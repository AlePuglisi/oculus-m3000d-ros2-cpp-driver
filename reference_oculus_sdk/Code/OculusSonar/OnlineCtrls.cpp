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

#include <QSettings>

#include "OnlineCtrls.h"
#include "ui_OnlineCtrls.h"

#include "MainView.h"
#include "../Displays/SonarSurface.h"

// ----------------------------------------------------------------------------
//
OnlineCtrls::OnlineCtrls(QWidget *pParent) :
  CtrlWidget(pParent),
  ui(new Ui::OnlineCtrls)
{
	m_pMainView = (MainView*) pParent;

	// The default settings
	m_demandMode  = navigation;
	m_demandGain  = 50;    // (%)
	m_demandRange = 10.0;    // (%)
	m_actualRange = 10.0;
	m_lastPartNo = 0xFFFF;

	m_lfIndex = 4;
	m_hfIndex = 5;

	ui->setupUi(this);

	if (m_pMainView) {
		// Set up the range controls
		ui->rangeValue->setText(QString::number(m_demandRange));
		ui->rangeSlider->setRange(0, 100);
		ui->rangeSlider->setValue(m_demandRange);
		// Set up the gain controls
		ui->gainValue->setText(QString::number(m_demandGain) + "%");
		ui->gainSlider->setRange(0, 100);
		ui->gainSlider->setValue(m_demandGain);
		// Setup the frequency slider
		ui->freqSlider->setRange(1, 2);
		ui->freqSlider->setValue((m_demandMode == navigation ? 1 : 2));

		// Connect up the slots/signals
		connect(ui->gainSlider, &QSlider::valueChanged, this, &OnlineCtrls::GainChanged);
		connect(ui->rangeSlider, &QSlider::valueChanged, this, &OnlineCtrls::RangeChanged);
		connect(ui->freqSlider, &QSlider::valueChanged, this, &OnlineCtrls::FreqChanged);
		connect(ui->record, &QPushButton::toggled, this, &OnlineCtrls::RecordChanged);

		connect(ui->low, &QPushButton::clicked, this, &OnlineCtrls::SetLowFreq);
		connect(ui->high, &QPushButton::clicked, this, &OnlineCtrls::SetHighFreq);
	}
}

// ----------------------------------------------------------------------------
//
OnlineCtrls::~OnlineCtrls()
{
  delete ui;
}

// ----------------------------------------------------------------------------
//
void OnlineCtrls::ReadSettings()
{
  QSettings settings;

  // Update the range, gain and mode values
  m_demandGain  = settings.value("DemandGain", 50).toInt();
  m_demandRange = settings.value("DemandRange", 50).toInt();
  //m_demandMode  = (eSonarMode)settings.value("DemandMode", 1).toInt();
  m_demandMode = eSonarMode::navigation;

  InitRangeSlider(m_demandMode);

  // Update the range and gain controls
  ui->gainValue->setText(QString::number(m_demandGain) + "%");
  ui->rangeValue->setText(QString::number(m_demandRange) + "m");

  ui->rangeSlider->blockSignals(true);
  ui->rangeSlider->setValue(0);
  // find the range index
  for (int i = 1; i < m_rangeCount; i++) {
      if (m_ranges[i] > m_demandRange) {
          ui->rangeSlider->setValue(i-1);
            break;
      }
  }
  ui->rangeSlider->blockSignals(false);

  ui->gainSlider->setValue(m_demandGain);
  // Update the frequency slider
  ui->freqSlider->setValue(m_demandMode);

  if (m_demandMode == navigation) {
	  ui->freq->setText("750kHz");
  }
  else {
	  ui->freq->setText("1.2MHz");
  }
}

// ----------------------------------------------------------------------------
//
void OnlineCtrls::WriteSettings()
{
  QSettings settings;

  settings.setValue("DemandGain",  m_demandGain);
  settings.setValue("DemandRange", m_demandRange);
  settings.setValue("DemandMode",  m_demandMode);
}

// ----------------------------------------------------------------------------
//
void OnlineCtrls::GainChanged(int value)
{
  m_demandGain = value;
  ui->gainValue->setText(QString::number(m_demandGain) + "%");
}

// ----------------------------------------------------------------------------
//
void OnlineCtrls::InitRangeSlider(eSonarMode mode) {

    int lastCount = m_rangeCount;

	// TODO Set the maximum ranges based on the part number


    // Update the range slider for the specified frequency
    if (mode == navigation) {
        m_ranges = MainView::NAVIGATION_RANGES;
		m_rangeCount = MainView::NAVIGATION_RANGE_COUNT;
    }
    else {
        m_ranges = MainView::INSPECTION_RANGES;
		m_rangeCount = MainView::INSPECTION_RANGE_COUNT;
    }

    ui->rangeSlider->blockSignals(true);
    ui->rangeSlider->setMaximum(m_rangeCount - 1);

	UpdateRangeSlider(m_lastPartNo);

	m_rangeIndex = 0;

	ui->rangeSlider->blockSignals(false);

	if (m_demandMode == navigation) {
		RangeChanged(m_lfIndex);
	}
	else {
		RangeChanged(m_hfIndex);
	}

}

void OnlineCtrls::UpdateControls(OculusPartNumberType pn) {

	//

}

// -----------------------------------------------------------------------------
void OnlineCtrls::UpdateRangeSlider(uint16_t pn)
{
    // return;
    OculusPartNumberType partNo = (OculusPartNumberType) pn;

    ui->freqSlider->setVisible(true);
    ui->high->setVisible(true);
    ui->low->setVisible(true);
    ui->freq->setVisible(true);

    if ((partNo == partNumberM370s) || (partNo == partNumberMT370s) ||
        (partNo == partNumberMD370s) || (partNo == partNumberMD370s_Burton) ||
        (partNo == partNumberMD370s_Impulse)) {
        ui->freqSlider->setVisible(false);
        ui->high->setVisible(false);
        ui->low->setVisible(false);
        ui->freq->setVisible(false);
    }

    m_pMainView->m_deviceForm.setPartNo(partNo);

    if ((m_demandMode == identification) &&
        ((partNo == partNumberM370s) || (partNo == partNumberMT370s) ||
         (partNo == partNumberMD370s) || (partNo == partNumberMD370s_Burton) ||
         (partNo == partNumberMD370s_Impulse))) {
        return;
    }

    // if (m_lastPartNo == pn) return;

    ui->rangeSlider->blockSignals(true);

    // Handle the different maximum ranges
    double maxRange = 120;

    switch (partNo) {
        case partNumberM370s:
        case partNumberMT370s:
        case partNumberMD370s:
        case partNumberMD370s_Burton:
        case partNumberMD370s_Impulse:
            maxRange = 200;
            break;
        case partNumberC550d:
            if (m_demandMode == identification) {
                ui->freq->setText("820kHz");
                maxRange = 30;
            } else {
                ui->freq->setText("550kHz");
                maxRange = 100;
            }
            break;
        case partNumberM750d:
        case partNumberMT750d:
        case partNumberMD750d:
        case partNumberMD750d_Burton:
        case partNumberMD750d_Impulse:
            if (m_demandMode == identification) {
                ui->freq->setText("1.2MHz");
                maxRange = 40;
            } else {
                ui->freq->setText("750kHz");
                maxRange = 120;
            }
            break;
        case partNumberM1200d:
        case partNumberMT1200d:
        case partNumberMD1200d:
        case partNumberMD1200d_Burton:
        case partNumberMD1200d_Impulse:
            if (m_demandMode == identification) {
                ui->freq->setText("2.1MHz");
                maxRange = 10;
            } else {
                ui->freq->setText("1.2MHz");
                maxRange = 40;
            }
            break;
        case partNumberM3000d:
        case partNumberMT3000d:
        case partNumberMD3000d_Burton:
        case partNumberMD3000d_Impulse:
            if (m_demandMode == identification) {
                ui->freq->setText("3.0MHz");
                maxRange = 5;
            } else {
                ui->freq->setText("1.2MHz");
                maxRange = 30;
            }
            break;
        case partNumberUndefined:
        case partNumberEnd:
        default:
            if (m_demandMode == identification) {
                ui->freq->setText("-");
                maxRange = 40;
            } else {
                ui->freq->setText("-");
                maxRange = 120;
            }
            break;
    }

    int idx = 0;

    for (idx = 0; idx < m_rangeCount; idx++) {
        if (m_ranges[idx] > maxRange) {
            break;
        }
    }

    // Shift the maximu
    ui->rangeSlider->setMaximum(idx - 1);
    ui->rangeSlider->blockSignals(false);

    m_lastPartNo = pn;
}

// ----------------------------------------------------------------------------
//
void OnlineCtrls::RangeChanged(int value)
{
	// Store the range index
		m_rangeIndex = value;

    // Get the range from the array
    m_demandRange = m_ranges[value];
    ui->rangeValue->setText(QString::number(m_demandRange) + "m");

	if (m_demandMode == navigation) {
		m_lfIndex = value;
	}
	else {
		m_hfIndex = value;
	}

}

// ----------------------------------------------------------------------------
//
void OnlineCtrls::FreqChanged(int value)
{
    // Update the frequency mode
    m_demandMode = (eSonarMode)value;

    // Update the frequency text label
    if (m_demandMode == navigation) {
        if ((m_lastPartNo == partNumberM1200d) ||
            (m_lastPartNo == partNumberMT1200d) ||
            (m_lastPartNo == partNumberMD1200d)) {
            ui->freq->setText("1.2MHz");
        } else {
            ui->freq->setText("750kHz");
        }
    } else {
        if ((m_lastPartNo == partNumberM1200d) ||
            (m_lastPartNo == partNumberMT1200d) ||
            (m_lastPartNo == partNumberMD1200d)) {
            ui->freq->setText("2.1MHz");
        } else if (
            (m_lastPartNo == partNumberM3000d) ||
            (m_lastPartNo == partNumberMT3000d)) {
            ui->freq->setText("3.0MHz");
        } else {
            ui->freq->setText("1.2MHz");
        }
    }

    // Update the range slider with ranges for the specified frequency
    InitRangeSlider(m_demandMode);

    // Update the range
		//m_pMainView->m_pSonarSurface->CalcGrid();
		//m_pMainView->m_fanDisplay.update();

	UpdateRangeSlider(m_lastPartNo);

	// Set the last range
	if (m_demandMode == navigation) {
		ui->rangeSlider->setValue(m_lfIndex);
	}
	else {
		ui->rangeSlider->setValue(m_hfIndex);
	}
}

// ----------------------------------------------------------------------------
void OnlineCtrls::UpdateLogFileName()
{
    ui->fileName->setText(QString(QFile(m_pMainView->m_logger.m_fileName).fileName()).split("/").last());
}

void OnlineCtrls::ToogleFrequency()
{
    // Ignore this if the sonar is a LF only sonar
    if ((m_pMainView->m_pSonarInfo->partNumber == partNumberM370s) ||
        (m_pMainView->m_pSonarInfo->partNumber == partNumberMT370s) ||
        (m_pMainView->m_pSonarInfo->partNumber == partNumberMD370s) ||
        (m_pMainView->m_pSonarInfo->partNumber == partNumberMD370s_Burton) ||
        (m_pMainView->m_pSonarInfo->partNumber == partNumberMD370s_Impulse)) {
        qDebug() << "Ignoring";
        return;
    }

    int value = (m_demandMode == navigation ? 2 : 1);
    this->FreqChanged(value);
    ui->freqSlider->setValue(value);
}

void OnlineCtrls::IncreaseGain() {
	int gain = m_demandGain;

	if (gain < 100) {
		gain++;
		GainChanged(gain);
		ui->gainSlider->setValue(gain);
	}
}

void OnlineCtrls::DecreaseGain() {
	int gain = m_demandGain;

	if (gain > 0) {
		gain--;
		GainChanged(gain);
		ui->gainSlider->setValue(gain);
	}
}

void OnlineCtrls::IncreaseRange() {
	int index = m_rangeIndex;
	if (index < ui->rangeSlider->maximum()) {
		index++;
		RangeChanged(index);
		ui->rangeSlider->setValue(index);

		if (m_demandMode == navigation) {
			m_lfIndex = index;
		}
		else {
			m_hfIndex = index;
		}
	}
}

void OnlineCtrls::DecreaseRange() {
	int index = m_rangeIndex;
	if (index > 0) {
		index--;
		RangeChanged(index);
		ui->rangeSlider->setValue(index);

		if (m_demandMode == navigation) {
			m_lfIndex = index;
		}
		else {
			m_hfIndex = index;
		}
	}
}

void OnlineCtrls::ToggleRecord() {
	bool recording = (m_pMainView->m_logger.m_state == logging ? false : true);
	//this->RecordChanged(recording);
	ui->record->setChecked(recording);
}

// ----------------------------------------------------------------------------
//
void OnlineCtrls::RecordChanged(bool checked)
{
    // Check the "checked" state
    if (checked) {
        // Start logging
        m_pMainView->StartLog();
        UpdateLogFileName();
    }
    else {
        // Stop logging
        ui->fileName->setText("");
        m_pMainView->StopLog();
    }

}

void OnlineCtrls::CancelRecord() {
	ui->record->setChecked(false);
}


void OnlineCtrls::SetLowFreq() {
	FreqChanged(eSonarMode::navigation);

	ui->freqSlider->blockSignals(true);
	{
		ui->freqSlider->setValue(1);
	}
	ui->freqSlider->blockSignals(false);
}

void OnlineCtrls::SetHighFreq() {
	FreqChanged(eSonarMode::identification);

	ui->freqSlider->blockSignals(true);
	{
		ui->freqSlider->setValue(2);
	}
	ui->freqSlider->blockSignals(false);
}
