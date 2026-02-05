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

#include "DeviceForm.h"
#include "ui_DeviceForm.h"

#include "MainView.h"

#include "../RmUtil/RmUtil.h"

#include <QFileDialog>
#include <QString>
#include <QMessageBox>
#include <QTemporaryDir>
#include <QStringList>


DeviceForm::DeviceForm(QWidget *parent) :
    QDialog(parent),
		ui(new Ui::DeviceForm)
{
    m_pMainView = (MainView*) parent;

    ui->setupUi(this);

    // Masks for the IP addresses
    ui->ipAddr->setInputMask("000.000.000.000");
    ui->ipMask->setInputMask("000.000.000.000");

    connect(ui->dhcpEnable, &QSlider::valueChanged, this, &DeviceForm::DhcpChanged);

    connect(ui->gainAssist, &QSlider::valueChanged, this, &DeviceForm::GainAssistChanged);
    connect(ui->gammaControl, &QSlider::valueChanged, this, &DeviceForm::GammaCorrectionChanged);
    connect(ui->netSpeedLimitControl, &QSlider::valueChanged, this, &DeviceForm::NetSpeedLimitChanged);

	connect(ui->ipAddr, &QLineEdit::editingFinished, this, &DeviceForm::ValidateIpAddress);
	connect(ui->ipMask, &QLineEdit::editingFinished, this, &DeviceForm::ValidateNetmask);

	m_partNo = partNumberUndefined;
}

DeviceForm::~DeviceForm()
{
    delete ui;
}

// -----------------------------------------------------------------------------
void DeviceForm::UpdateControls()
{
	uint32_t ipAddr = m_pMainView->m_oculusClient.m_config.m_ipAddr;
	uint32_t ipMask = m_pMainView->m_oculusClient.m_config.m_ipMask;

	QString ipStr = QString::fromLatin1("%1.%2.%3.%4")
					.arg(((ipAddr & 0xFF000000) >> 24))
					.arg(((ipAddr & 0xFF0000) >> 16))
					.arg(((ipAddr & 0xFF00) >> 8))
					.arg(((ipAddr & 0xFF) ));


	QString ipMaskStr = QString::fromLatin1("%1.%2.%3.%4")
					.arg(((ipMask & 0xff000000) >> 24))
					.arg(((ipMask & 0xff0000) >> 16))
					.arg(((ipMask & 0xff00) >> 8))
					.arg(((ipMask & 0xff) ));

    uint32_t dhcp = m_pMainView->m_oculusClient.m_config.m_bDhcpEnable;

	ui->ipAddr->setText(ipStr);
	ui->ipMask->setText(ipMaskStr);
    ui->dhcpEnable->setValue(dhcp);

    ui->gainAssist->setValue(m_gainAssist ? 1 : 0);
    ui->gammaControl->setValue(m_gammaCorrection);
    ui->netSpeedLimitControl->setValue(m_netSpeedLimit);

    this->DhcpChanged(dhcp);
}

// -----------------------------------------------------------------------------
void DeviceForm::showEvent(QShowEvent *event)
{
	ui->tabWidget->setCurrentIndex(0);

    this->UpdateControls();
}

// -----------------------------------------------------------------------------
void DeviceForm::accept()
{
    // Capture the new user config settings
    QStringList ip = ui->ipAddr->text().split(".");
    QStringList mask = ui->ipMask->text().split(".");
    uint32_t dhcp = ui->dhcpEnable->value();

	// KW - 23/10/2017
	// Reverse the IP and subnet byte order to match the new IP change command
	uint32_t ipAddress  = (ip[0].toInt() << 24) + (ip[1].toInt() << 16) + (ip[2].toInt() << 8) + ip[3].toInt();
	uint32_t ipMask     = (mask[0].toInt() << 24) + (mask[1].toInt() << 16) + (mask[2].toInt() << 8) + mask[3].toInt();
	//uint32_t ipAddress  = (ip[3].toInt() << 24) + (ip[2].toInt() << 16) + (ip[1].toInt() << 8) + ip[0].toInt();
	//uint32_t ipMask     = (mask[3].toInt() << 24) + (mask[2].toInt() << 16) + (mask[1].toInt() << 8) + mask[0].toInt();

    bool changed = false;

	// Update the variables with the new values
	m_gainAssist = m_gainAssistNew;
	m_gammaCorrection = m_gammaCorrectionNew;
	m_netSpeedLimit = m_netSpeedLimitNew;

    if ((ipAddress != m_pMainView->m_oculusClient.m_config.m_ipAddr) || (ipMask != m_pMainView->m_oculusClient.m_config.m_ipMask) || (dhcp != m_pMainView->m_oculusClient.m_config.m_bDhcpEnable))
        changed = true;

    if (changed) {
		//qDebug() << "Writing user config...";

        // Write the changes to the device
        m_pMainView->m_oculusClient.WriteUserConfig(ipAddress, ipMask, dhcp);

		// Read the config back
		m_pMainView->m_oculusClient.RequestUserConfig();

		// Validate the changes
		bool ok = false;

		// Validate the changes
		uint32_t ip = m_pMainView->m_oculusClient.m_config.m_ipAddr;
		uint32_t mask = m_pMainView->m_oculusClient.m_config.m_ipMask;
		uint32_t dhcpEn = m_pMainView->m_oculusClient.m_config.m_bDhcpEnable;

		if ((ip == ipAddress) && (mask == ipMask) && (dhcpEn == dhcp)) {
			ok = true;
		}

		if (!ok) {
			QMessageBox msg;
			msg.setIcon(QMessageBox::Critical);
			msg.setStandardButtons(QMessageBox::Ok);
			msg.setText("Failed to write the Oculus configuration settings. Please retry");
			msg.setWindowTitle("Error");
			msg.exec();
			QDialog::reject();
		}
		else {

			QDialog::accept();
		}

		return;
    }
    else {
		qDebug() << "No changes";
        QDialog::reject();
    }

}

void DeviceForm::setPartNo(OculusPartNumberType partNo)
{
	m_partNo = partNo;
}

// -----------------------------------------------------------------------------
void DeviceForm::DhcpChanged(int value)
{
    bool enable = (value == 0);

    ui->ipAddr->setEnabled(enable);
    ui->ipMask->setEnabled(enable);
}

// -----------------------------------------------------------------------------
void DeviceForm::GainAssistChanged(int value)
{
	m_gainAssistNew = (value == 0 ? false : true);
}

/*
void DeviceForm::AltFrequencyChanged(int value)
{
	m_altFreq = (value == 0 ? false : true);
}
*/
// -----------------------------------------------------------------------------
void DeviceForm::GammaCorrectionChanged(int value)
{
	m_gammaCorrectionNew = (uint8_t)value;

    // Update the gamma label
    float gamma = (1.0 / 255) * value;
    ui->gammaValue->setText(QString::number(gamma, 1, 2));
}

void DeviceForm::ValidateIpAddress() {

	bool error = false;
	// Get the IP address
	QString ip = ui->ipAddr->text();
	// Split the IP address up
	QStringList parts = ip.split('.');

	if (parts.length() != 4) {
		// Error
		error = true;
	}
	else {

		foreach (QString part, parts) {
			bool ok = false;
			int num = part.toInt(&ok);

			qDebug() << num;

			if (!ok) {
				error = true;
			}

			if ((num < 0) || (num > 255)) {
				// Error
				error = true;
				break;
			}
		}

	}

	// Check for error
	if (error) {
		QMessageBox msg(QMessageBox::Critical, "IP Address Error", "Invalid IP address format");
		msg.exec();
		ui->ipAddr->setFocus();
		ui->ipAddr->selectAll();
	}

}

void DeviceForm::ValidateNetmask() {
	bool error = false;
	// Get the IP address
	QString ip = ui->ipMask->text();
	// Split the IP address up
	QStringList parts = ip.split('.');

	if (parts.length() != 4) {
		// Error
		error = true;
	}
	else {

		foreach (QString part, parts) {
			bool ok = false;
			int num = part.toInt(&ok);

			qDebug() << num;

			if (!ok) {
				error = true;
			}

			if ((num < 0) || (num > 255)) {
				// Error
				error = true;
				break;
			}
		}

	}

	// Check for error
	if (error) {
		QMessageBox msg(QMessageBox::Critical, "Netmask Address Error", "Invalid netmask address format");
		msg.exec();
		ui->ipMask->setFocus();
		ui->ipMask->selectAll();
	}
}

// -----------------------------------------------------------------------------
void DeviceForm::NetSpeedLimitChanged(int value)
{
	m_netSpeedLimitNew = (uint8_t)value;
    // Update the gamma label
    ui->netSpeedLimitValue->setText(QString::number(value, 1, 0));
}


void DeviceForm::NewUserConfig() {
	qDebug() << "New User Config Received";

	this->UpdateControls();
}

void DeviceForm::NewVersionInfo() {
	qDebug() << "New Version Info Received";
}

