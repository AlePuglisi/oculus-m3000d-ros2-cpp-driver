/******************************************************************************
 * (c) Copyright 2017 Blueprint Subsea.
 * This file is part of Oculus Viewer
 *
 * Oculus Viewer is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Oculus Viewer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

#include "ModeCtrls.h"
#include "ui_ModeCtrls.h"

#include "MainView.h"

#include <QFileDialog>
#include <QSettings>
#include <QListWidgetItem>
#include <QDesktopWidget>

#include <QMessageBox>
#include <QWaitCondition>

#include <QNetworkInterface>

#include <QVideoEncoderSettingsControl>

#include "DeviceForm.h"

#include "../RmUtil/RmUtil.h"
#include "../Displays/SonarSurface.h"

// -----------------------------------------------------------------------------
ModeCtrls::ModeCtrls(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ModeCtrls)
{
    m_pMainWnd = (MainView*) parent;

    ui->setupUi(this);

    // Connect up the signals/slots
    connect(ui->connect,    &QPushButton::toggled, this, &ModeCtrls::Connect);
    connect(ui->open,       &QPushButton::toggled, this, &ModeCtrls::OpenFile);
    connect(ui->config,     &QPushButton::clicked, this, &ModeCtrls::ShowDeviceConfig);

    // Hide the edit controls
    ui->config->setVisible(false);

	EnableConnect(false);

	m_lastDir = "";
}

// -----------------------------------------------------------------------------
ModeCtrls::~ModeCtrls()
{
    delete ui;
}

// -----------------------------------------------------------------------------
void ModeCtrls::setDisplayMode(int mode)
{
  if (mode == 2)
  {
    // Review mode

    ui->open->blockSignals(true);
        ui->open->setChecked(true);
    ui->open->blockSignals(false);
    // Disable the device setup
    ui->config->setVisible(false);
  }
  else {
    // Not review mode

    ui->open->blockSignals(true);
        ui->open->setChecked(false);
    ui->open->blockSignals(false);

    ui->config->setVisible(false);

    // offline
    if (mode == 0) {
        ui->connect->blockSignals(true);
		ui->connect->setChecked(false);
        ui->connect->blockSignals(false);

    }
    else {
        ui->config->setVisible(true);
    }
  }
}

void ModeCtrls::ToggleConnect() {
	bool check = ui->connect->isChecked();
	this->Connect(!check);
	ui->connect->setChecked(!check);
}

void ModeCtrls::ToggleOpen() {
	bool check = ui->open->isChecked();
	this->OpenFile(!check);
	ui->open->setChecked(!check);
}


// -----------------------------------------------------------------------------
void ModeCtrls::Connect(bool checked) {

	// Clear the last part number
	m_pMainWnd->m_partNumber = partNumberUndefined;

	// Disconnect from the sonar
    if (!checked) {
        // Disconnect
        this->Disconnect();
		// Clear the part number
		m_pMainWnd->m_infoCtrls.HideInfo();
		m_pMainWnd->m_pSonarInfo = NULL;
        return;
    }


	// Build a list of subnet masks for the ethernet interfaces on this PC
	QList<QNetworkInterface> ifs = QNetworkInterface::allInterfaces();
	QList<QNetworkAddressEntry> masks;

	foreach (QNetworkInterface inf, ifs) {
		foreach (QNetworkAddressEntry entry, inf.addressEntries()) {
			masks.append(entry);

			//qDebug() << entry.netmask().toString();
		}
	}


    int index = 0;
    QString ipAddress = "";
    QString ipMask = "";
    QString serialNo = "";

    uint32_t appVersion = 0;
    uint32_t bitVersion = 0;

	// Count the number of sonars
    int count = m_pMainWnd->m_connectForm.sonarCount();
	m_pMainWnd->m_titleCtrls.SetTitle("");

    // Check for the number of connected sonars
    if (count == 0) {
        // Nothing to connect to, abort
        ui->connect->blockSignals(true);
		ui->connect->setChecked(false);
        ui->connect->blockSignals(false);
        return;
    }
    else if (count == 1) {
        OculusStatusMsg osm = m_pMainWnd->m_sonarList.first();

        if (osm.connectedIpAddr != 0) {

			QMessageBox msg;
			msg.setText("A client is already connected to Oculus");
            msg.setIcon(QMessageBox::Warning);
            msg.exec();

            this->Disconnect();

            return;

		}
		else {

#define CHECK_SUBNET

#ifdef CHECK_SUBNET
			// Check for matching subnets
			//QList<QHostAddress> masks;
			bool valid = false;
/*
			QHostAddress sip(osm.ipAddr);
			foreach (QNetworkAddressEntry m, masks) {
				if (m.ip().isInSubnet(sip, osm.ipMask)) {
					qDebug("Yes");
					valid = true;
				}
			}*/
/*
			if (!valid) {

				QMessageBox msg;
				QString sb;
				sb.append("The subnet mask of the Oculus does not match the local ethernet adapter.\r\n\r\n");
				sb.append("Please change the settings of your local ethernet adapter to suit:\r\n");
				sb.append("IP: ");
				sb.append(RmUtil::FormatIpAddress(osm.ipAddr));
				sb.append("\r\n");
				sb.append("Subnet: ");
				sb.append(RmUtil::FormatIpAddress(osm.ipMask));

				msg.setWindowTitle("Ethernet Subnet Mismatch");
				msg.setText(sb);
				msg.setIcon(QMessageBox::Warning);
				msg.exec();

				this->Disconnect();

				return;
			}
			*/
#endif
            // Connect to the first sonar
            ipAddress = RmUtil::FormatIpAddress(osm.ipAddr);
            ipMask = RmUtil::FormatIpAddress(osm.ipMask);
            serialNo = QString::number(osm.deviceId);
        }
    }
    else {

        // Show the connection window if needs be
        int result = m_pMainWnd->m_connectForm.exec();

        // Check for cancel
        if (result == 0) {
            ui->connect->blockSignals(true);
			ui->connect->setChecked(false);
            ui->connect->blockSignals(false);
            return;
        }

        // Check for a valid selection
        QListWidgetItem *item = m_pMainWnd->m_connectForm.getSelection();

        if (item == nullptr) {
            ui->connect->blockSignals(true);
                ui->connect->setChecked(false);
            ui->connect->blockSignals(false);
            return;
        }

		// Valid selection
        QString text = item->text();
        QStringList fields = text.split("\r\n");

        ipAddress = fields[1].split("\t").last();
        ipMask = fields[2].split("\t").last();
        serialNo = fields[3].split("\t").last();

#ifdef CHECK_SUBNET
    }
#endif

	// Get the IP address of the sonar we're connecting to
	uint32_t sip = QHostAddress(ipAddress).toIPv4Address();
	uint32_t snm = QHostAddress(ipMask).toIPv4Address();

	bool valid = false;
	bool badSubnet = true;
	bool addrConflict = false;

	// Find a netmask match for the sonar
	foreach (QNetworkAddressEntry m, masks) {
		// Host IP and subnet mask
		uint32_t hip = m.ip().toIPv4Address();
		uint32_t hnm = m.netmask().toIPv4Address();

		// Get the range of IP addresses for the subnet
		//uint32_t netstart = (hip & hnm);
		//uint32_t netend = (netstart | ~hnm);

		// We could also check via...

		if ((hip & hnm) == (sip & snm)) {
			valid = true;
			badSubnet = false;

			if (sip == hip) {
				addrConflict = true;
			}

			break;
		}
	}

	addrConflict = false;

	if ((!valid) || (addrConflict)) {
		QMessageBox msg;
		QString sb;
		sb.append("Unable to connect to Oculus.\r\n\r\n");
		if (addrConflict) {
			sb.append("The IP address of the sonar and the computer are the same:\r\n");
			sb.append("IP: ");
			sb.append(ipAddress);
		}
		else {
			sb.append("Network subnet mismatch:\r\n");
			sb.append("IP: ");
			sb.append(ipAddress);
			sb.append("\r\n");
			sb.append("Subnet: ");
			sb.append(ipMask);
		}
		sb.append("\r\n\r\n");
		sb.append("Please adjust your ethernet settings accordingly");

		msg.setWindowTitle("Ethernet Configuration Error");
		msg.setText(sb);
		msg.setIcon(QMessageBox::Warning);
		msg.exec();

		this->Disconnect();

		return;
	}

	// Connect to the sonar
	if (!m_pMainWnd->m_oculusClient.IsOpen())
	{
		m_pMainWnd->m_onlineCtrls.ReadSettings();


		// Set the IP and subnet mask
		m_pMainWnd->m_oculusClient.m_hostname = ipAddress;
		m_pMainWnd->m_oculusClient.m_mask = ipMask;
		// Connect the client
		m_pMainWnd->m_oculusClient.Connect();
		// Online display mode
		m_pMainWnd->SetDisplayMode(online);
		// Stop any replay in progress
		m_pMainWnd->StopReplay();

		// Request the user config
		m_pMainWnd->m_oculusClient.RequestUserConfig();
	}
}

void ModeCtrls::OpenFileEx(QString fileName) {

	RmPlayer player;

	if (fileName.length() > 0)
	{
		// Get the log file path and save it ready for opening another file
		m_lastDir = QFileInfo(fileName).absolutePath();
		// Set the window title text
		m_pMainWnd->SetTitleLogFile(fileName);

/*
		// Get the first View Info record
		m_pMainWnd->m_nViewInfoEntries = player.CreateTypeIndex(fileName, rt_ocViewInfo, &m_pMainWnd->m_pViewInfoEntries);
*/
		// Build an index of the sonar types
		m_pMainWnd->m_nEntries = player.CreateTypeIndex(fileName, rt_oculusSonar, &m_pMainWnd->m_pEntries);

		if (m_pMainWnd->m_nEntries == 0)
		{
			m_pMainWnd->m_info.setText(tr("No oculus entries in file, trying sonar header"));
			m_pMainWnd->m_nEntries = player.CreateTypeIndex(fileName, rt_apSonarHeader, &m_pMainWnd->m_pEntries);
			m_pMainWnd->m_useRawSonar = true;
		}
		else
			m_pMainWnd->m_useRawSonar = false;

	  if (m_pMainWnd->m_nEntries > 0)
	  {
		m_pMainWnd->m_info.setText(QString::number(m_pMainWnd->m_nEntries) + tr(" Oculus entries identified in file."));
		m_pMainWnd->m_replayFile = fileName;


		if (m_pMainWnd->m_oculusClient.IsOpen())
		{
		  m_pMainWnd->m_oculusClient.Disconnect();
		  ui->connect->blockSignals(true);
			ui->connect->setChecked(false);
		  ui->connect->blockSignals(false);
		  update();
		}

		m_pMainWnd->SetDisplayMode(review);

		QApplication::processEvents();

		m_pMainWnd->m_reviewCtrls.SetEntry(0);
		m_pMainWnd->ReviewEntryChanged(0);
	  }
	}
	else {
		ui->open->blockSignals(true);
		ui->open->setChecked(false);
		ui->open->blockSignals(false);
		update();
	}

/*
	if (m_pMainWnd->m_nViewInfoEntries > 0) {
		player.OpenFileAt(fileName, m_pMainWnd->m_pViewInfoEntries[0]);
		player.ReadNextItemOfType(rt_ocViewInfo);
		player.CloseFile();
	}
*/

	// Recalculate the grid and update the fan display
	m_pMainWnd->m_pSonarSurface->CalcGrid();
	m_pMainWnd->m_fanDisplay.update();
}

// -----------------------------------------------------------------------------
void ModeCtrls::OpenFile(bool checked)
{
    // Close the current log file
    if (!checked) {
        this->CloseFile();
        // Set the display to offline mode to hide all the online controls
        m_pMainWnd->SetDisplayMode(offline);
        m_pMainWnd->SetTitleLogFile("");
		// Hide the palette if open
		m_pMainWnd->m_palSelect.setVisible(false);
        return;
    }

	if ((m_lastDir == "") && (m_pMainWnd->m_logger.m_logDir != "")) {
		m_lastDir = m_pMainWnd->m_logger.m_logDir;
	}

	QString logFile = QFileDialog::getOpenFileName(&m_pMainWnd->m_fanDisplay, tr("Select log file to open"), m_lastDir, "Oculus Log Files (*.oculus);; ArtemisPRO Log Files (*.prodive);; Oculus Log Files (*.log)");

	this->OpenFileEx(logFile);

}

// -----------------------------------------------------------------------------
void ModeCtrls::CloseFile()
{

}

// -----------------------------------------------------------------------------
void ModeCtrls::Disconnect()
{
    if (m_pMainWnd->m_oculusClient.IsOpen())
      m_pMainWnd->m_oculusClient.Disconnect();

    m_pMainWnd->SetDisplayMode(offline);

	m_pMainWnd->m_infoCtrls.ClearModel();
	m_pMainWnd->m_palSelect.setVisible(false);

	free(m_pMainWnd->m_pSonarInfo);
	m_pMainWnd->m_pSonarInfo = NULL;

	m_pMainWnd->m_infoCtrls.UpdateModel(partNumberUndefined);

}

// -----------------------------------------------------------------------------
bool ModeCtrls::ExecDeviceConfig() {
	m_pMainWnd->m_oculusClient.m_wait.wakeAll();

	// Request the user config and wait for a response
	//qDebug() << "Request User Config";
	bool ok = m_pMainWnd->m_oculusClient.RequestUserConfig();

	// Ensure that the user config was received ok
	if (!ok) {
		qDebug() << "Failed to request user config";
		return false;
	}

	// Execute the device configuration dialog
	int result = m_pMainWnd->m_deviceForm.exec();

	// Check if the user clicked, ok
	if (result) {

		QMessageBox msg;
		msg.setIcon(QMessageBox::Information);
		msg.setStandardButtons(QMessageBox::Ok);
		QString str = "New settings have been successfully applied.\r\n\r\n";
		str += "Please power cycle Oculus and reconnect the software";
		msg.setText(str);
		msg.setWindowTitle("Power Cycle");
		msg.exec();

		this->Disconnect();
	}

	return true;
}

// -----------------------------------------------------------------------------
void ModeCtrls::ShowDeviceConfig()
{
	// Try and execute the device config window
	bool ok = ExecDeviceConfig();

	if (!ok) {
		// Retry
		ok = ExecDeviceConfig();

		if (!ok) {
			//qDebug() << "Failed to show device window";

			QMessageBox msg;
			msg.setIcon(QMessageBox::Warning);
			msg.setStandardButtons(QMessageBox::Ok);
			QString str = "Timeout waiting for a reply from Oculus.\r\n\r\n";
			str += "Please try again.";
			msg.setText(str);
			msg.setWindowTitle("Settings Request Timeout");
			msg.exec();
		}

	}
}

// -----------------------------------------------------------------------------
void ModeCtrls::EnableConnect(bool enable)
{
	// Set the "enabled" state of the connect button
	ui->connect->setEnabled(enable);
}
