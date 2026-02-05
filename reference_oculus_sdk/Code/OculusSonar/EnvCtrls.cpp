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

#include <QDebug>
#include <QDoubleValidator>
#include <QSettings>

#include "EnvCtrls.h"
#include "ui_EnvCtrls.h"

EnvCtrls::EnvCtrls(QWidget *pParent) :
  QWidget(pParent),
  ui(new Ui::EnvCtrls)
{
  ui->setupUi(this);
  ui->speedOfSound->setValidator(new QDoubleValidator(this));

  m_svType       = freshWater;
  m_speedOfSound = 1500.0;
  m_salinity     = 0;

  UpdateCtrls();
}

EnvCtrls::~EnvCtrls()
{
  delete ui;
}

// ----------------------------------------------------------------------------
void EnvCtrls::ReadSettings()
{
  QSettings settings;

  m_svType       = (eSvType)settings.value("SvType", freshWater).toInt();
  m_speedOfSound = settings.value("SOS", 1510.0).toDouble();
  m_salinity     = settings.value("Salinity", 0.0).toDouble();

  //qDebug() << "Read setting " + QString::number(m_speedOfSound);

  UpdateCtrls();
}

// ----------------------------------------------------------------------------
void EnvCtrls::WriteSettings()
{
  QSettings settings;

  settings.setValue("SvType",   m_svType);
  settings.setValue("SOS",      m_speedOfSound);
  settings.setValue("Salinity", m_salinity);
}

// ----------------------------------------------------------------------------
// Update the UI with the settings
void EnvCtrls::UpdateCtrls()
{
  switch (m_svType)
  {
    case freshWater:
      ui->freshWater->setChecked(true);
      ui->saltWater->setChecked(false);
      ui->useVelocity->setChecked(false);
      break;

    case saltWater:
      ui->freshWater->setChecked(false);
      ui->saltWater->setChecked(true);
      ui->useVelocity->setChecked(false);
      break;

    case fixedValue:
      ui->freshWater->setChecked(false);
      ui->saltWater->setChecked(false);
      ui->useVelocity->setChecked(true);
      break;
  }

  ui->speedOfSound->setText(QString::number(m_speedOfSound));
}



void EnvCtrls::on_freshWater_clicked()
{
  m_svTypeTemp = freshWater;
}

void EnvCtrls::on_saltWater_clicked()
{
  m_svTypeTemp = saltWater;
}

void EnvCtrls::on_useVelocity_clicked()
{
  m_svTypeTemp = fixedValue;
}

void EnvCtrls::on_speedOfSound_editingFinished()
{
  m_speedOfSoundTemp = ui->speedOfSound->text().toDouble();
}
