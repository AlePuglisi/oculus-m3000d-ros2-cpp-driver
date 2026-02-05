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

#ifndef INFOCTRLS_H
#define INFOCTRLS_H

#include <QWidget>


#include "../Oculus/Oculus.h"

class MainView;

namespace Ui {
class InfoCtrls;
}

class InfoCtrls : public QWidget
{
    Q_OBJECT

public:
    explicit InfoCtrls(QWidget *parent = 0);
    ~InfoCtrls();

	void UpdateInfo(QString ip, QString mask, QString serialNo);
	void UpdateModel(uint16_t model);
	void UpdateError(uint32_t status);
	void UpdateFrequency(double frequency);

	void HideInfo();
	void ClearModel();

	OculusPartNumberType m_lastModel;

	double m_lastFreq;

	MainView *m_pMainView;

private:
    Ui::InfoCtrls *ui;
};

#endif // INFOCTRLS_H
