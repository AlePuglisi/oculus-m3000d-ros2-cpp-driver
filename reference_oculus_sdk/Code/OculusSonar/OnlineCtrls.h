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

#ifndef ONLINECTRLS_H
#define ONLINECTRLS_H

#include "CtrlWidget.h"

#include "../Oculus/Oculus.h"

class MainView;

namespace Ui {
class OnlineCtrls;
}

// Enumerate different sonar modes
enum eSonarMode : int
{
  navigation = 1,
  identification = 2,
};

class OnlineCtrls : public CtrlWidget
{
  Q_OBJECT

public:
  explicit OnlineCtrls(QWidget *pParent = 0);
  ~OnlineCtrls();

  // Methods
  void ReadSettings();
  void WriteSettings();

  void UpdateLogFileName();

  void ToogleFrequency();
  void IncreaseGain();
  void DecreaseGain();
  void IncreaseRange();
  void DecreaseRange();
  void ToggleRecord();

  void CancelRecord();

  // Data
  MainView* m_pMainView;

  eSonarMode   m_demandMode;       // The demand mode sent to the oculus 1, 2, 3
  int          m_demandGain;       // The demanad gain sent to the oculus (0 - 100%)
	double       m_demandRange;      // The demand range sent to the oculs (0 - 100%)
  double       m_actualRange;      // The actual range
	uint16_t	m_lastPartNo;

  int						m_rangeIndex;
  double*        m_ranges;
  int           m_rangeCount;

  int			m_lfIndex;
  int			m_hfIndex;

  void InitRangeSlider(eSonarMode mode);

  void UpdateRangeSlider(uint16_t pn);

  void UpdateControls(OculusPartNumberType pn);


private:
  Ui::OnlineCtrls *ui;

public slots:
  void GainChanged(int value);
  void RangeChanged(int value);
  void FreqChanged(int value);
  void RecordChanged(bool checked);
  //void SetNavigationMode();
  //void SetInspectionMode();

  void SetLowFreq();
  void SetHighFreq();
};

#endif // ONLINECTRLS_H
