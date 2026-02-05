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

#include "AppCtrls.h"
#include "ui_AppCtrls.h"

#include <QFile>
#include <QString>
#include <QDir>
#include <QSettings>

#include "MainView.h"

AppCtrls::AppCtrls(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AppCtrls)
{
    m_pMainWnd = (MainView*) parent;

    ui->setupUi(this);

    connect(ui->dark, &QPushButton::clicked, this, &AppCtrls::SetDarkTheme);
    connect(ui->light, &QPushButton::clicked, this, &AppCtrls::SetLightTheme);
    connect(ui->navy, &QPushButton::clicked, this, &AppCtrls::SetNavyTheme);


}

AppCtrls::~AppCtrls()
{
    delete ui;
}

void AppCtrls::SetDarkTheme()
{
    emit StyleChanged("dark");
   // m_pMainWnd->SetTheme("dark");
}

void AppCtrls::SetLightTheme()
{
    emit StyleChanged("light");
}

void AppCtrls::SetNavyTheme()
{
    emit StyleChanged("navy");
}



void AppCtrls::ReadSettings()
{

}

void AppCtrls::WriteSettings()
{

}
