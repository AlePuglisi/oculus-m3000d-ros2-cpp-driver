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

#include "InfoCtrls.h"
#include "ui_InfoCtrls.h"

#include <QDebug>

#include "MainView.h"

//------------------------------------------------------------------------------
InfoCtrls::InfoCtrls(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::InfoCtrls)
{
    ui->setupUi(this);

	m_pMainView = (MainView*) parent;

	m_lastModel = OculusPartNumberType::partNumberUndefined;

	ui->serialNo->setVisible(false);
	ui->ipAddr->setVisible(false);
	ui->ipMask->setVisible(false);
	ui->revision->setVisible(false);
    ui->modelNo->setVisible(false);

	ui->error->setVisible(false);

	m_lastFreq = 0.0;

}

//------------------------------------------------------------------------------
InfoCtrls::~InfoCtrls()
{
    delete ui;
}

//------------------------------------------------------------------------------
// Update the Oculus information
void InfoCtrls::UpdateInfo(QString ip, QString mask, QString serialNo)
{
	// Hide the model number
	//ui->modelNo->setVisible(false);
	ui->serialNo->setVisible(true);
    ui->serialNo->setText("Oculus: " + serialNo);
	ui->ipAddr->setVisible(true);
	ui->ipAddr->setText("IP: " + ip);
	ui->ipMask->setVisible(true);
	ui->ipMask->setText("Mask: " + mask);
}

//------------------------------------------------------------------------------
void InfoCtrls::UpdateFrequency(double frequency) {
	if (m_lastFreq != frequency) {
		QString str;

		if (frequency == 1200.0) {
			str = "1.2MHz";
		}
		else {
			str = QString::number(frequency, 4, 0) + "kHz";
		}

		ui->modelNo->setText("Frequency: " + str);
		m_lastFreq = frequency;
		ui->modelNo->setVisible(true);
	}
}

//------------------------------------------------------------------------------
void InfoCtrls::UpdateModel(uint16_t model) {

	// Hide the info labels
	//HideInfo();

	// Capture the part
	OculusPartNumberType modelType = (OculusPartNumberType)model;

	if (modelType == OculusPartNumberType::partNumberUndefined) {
		ui->modelNo->setVisible(false);
		//qDebug() << "Unknown part number";
		m_lastModel = modelType;
		return;
	}

	if (modelType == m_lastModel) return;

	QString str;

	// Show the model number
	ui->modelNo->setVisible(true);

	QString mdl(m_pMainView->m_pSonarInfo->model);

	qDebug() << m_pMainView->m_pSonarInfo->model;

	ui->modelNo->setText("Model: " + mdl);

	m_lastModel = modelType;
}

// -----------------------------------------------------------------------------
void InfoCtrls::UpdateError(uint32_t status) {

	bool show = false;
	QString error;
	QColor color;
	bool checkPause = false;

	//qDebug() << "Status: " << status;

	//ui->error->setVisible(true);
	// Master status style
	OculusMasterStatusType mst = (OculusMasterStatusType)(status & 0x07);

	//qDebug() << "Master Status Type: " << mst;
	if (mst == oculusMasterStatusSsblBoot) {
		error = "Error: SSBL Bootloader";
		color = QColor("yellow");
		show = true;
		checkPause = true;
	}
	else if (mst == oculusMasterStatusSsblRun) {
		error = "Error: SSBL Run";
		color = QColor("yellow");
		show = true;
		checkPause = true;

	}

	// Check the pause reason
	if (checkPause) {
		// Pause reason
		OculusPauseReasonType prt = (OculusPauseReasonType)((status & 0x38) >> 3);

		if (prt == oculusPauseMagSwitch) {
			error = "Halt: Mag Switch Detected";
			color = QColor("yellow");
			show = true;
		}
		else if (prt == oculusPauseBootFromMain) {
			error = "Halt: Boot From Main";
			color = QColor("yellow");
			show = true;
		}
		else if (prt == oculusPauseFlashError) {
			error = "Halt: Flash Error. Update firmware";
			color = QColor("yellow");
			show = true;
		}
		else if (prt == oculusPauseJtagLoad) {
			error = "Halt: JTAG Load";
			color = QColor("yellow");
			show = true;
		}
	}


	bool overTempShutdown = (status & (1 << 15));

	if (overTempShutdown) {
		error = "Warning: High Temp - Ping Rate Stopped";
		color = QColor("orange");
		show = true;
	}

	bool transmitError = (status & (1 << 16));

	if (transmitError) {
		error = "Critical: Transmit Circuit Failure";
		color = QColor("red");
		show = true;
	}

	ui->error->setVisible(show);
	ui->error->setText(error);
	ui->error->setStyleSheet("QLabel { color: " + color.name() + "; }");
}

// -----------------------------------------------------------------------------
void InfoCtrls::HideInfo() {
	// Set all the labels to invisible
	ui->serialNo->setVisible(false);
	ui->ipAddr->setVisible(false);
	ui->ipMask->setVisible(false);
	ui->revision->setVisible(false);
	ui->modelNo->setVisible(false);
	ui->error->setVisible(false);
}

void InfoCtrls::ClearModel() {
	m_lastModel = OculusPartNumberType::partNumberUndefined;
}
