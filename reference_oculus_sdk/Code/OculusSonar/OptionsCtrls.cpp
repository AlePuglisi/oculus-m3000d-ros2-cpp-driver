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

#include "OptionsCtrls.h"
#include "ui_OptionsCtrls.h"

#include "MainView.h"

#include <QMessageBox>

// -----------------------------------------------------------------------------
OptionsCtrls::OptionsCtrls(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OptionsCtrls)
{
    m_pMainWnd = (MainView*)parent;

    ui->setupUi(this);

    connect(ui->oculusLogo, &QPushButton::clicked, this, &OptionsCtrls::ShowOculusPage);
    connect(ui->bpLogo,     &QPushButton::clicked, this, &OptionsCtrls::ShowOculusPage);
}

// -----------------------------------------------------------------------------
OptionsCtrls::~OptionsCtrls()
{
    delete ui;
}

// -----------------------------------------------------------------------------
void OptionsCtrls::ShowOculusPage()
{
    m_pMainWnd->SpawnOculusWebView();
}

// -----------------------------------------------------------------------------
void OptionsCtrls::ShowSettings()
{
    m_pMainWnd->m_settings.exec();
}

// -----------------------------------------------------------------------------
void OptionsCtrls::CloseApp()
{
    QMessageBox msg;
    msg.setIcon(QMessageBox::Question);
    msg.setText("Are you sure you want to quit?");
    msg.setStandardButtons(QMessageBox::Yes | QMessageBox::No);

    int result = msg.exec();

    if (result == QMessageBox::Yes) {
        QApplication::quit();
    }
}
