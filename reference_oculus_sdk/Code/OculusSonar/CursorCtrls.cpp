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

#include "CursorCtrls.h"
#include "ui_CursorCtrls.h"

// -----------------------------------------------------------------------------
CursorCtrls::CursorCtrls(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CursorCtrls)
{
    ui->setupUi(this);

    //ui->dist->setText("test");
    //ui->angle->setText("test");
}

// -----------------------------------------------------------------------------
CursorCtrls::~CursorCtrls()
{
    delete ui;
}


// -----------------------------------------------------------------------------
void CursorCtrls::UpdateMouseInfo(float dist, float angle, float x, float y)
{
    // Update the X, Y cartesian coordinates
    ui->cursorInfo->setText("X: " + QString::number(x, 3, 2) + "m, Y: " + QString::number(y, 3, 2) + "m");
    // Update the range measurement
    ui->rangeInfo->setText("Range: " + QString::number(dist, 3, 2) + "m");
    // Update the bearing measurement
    ui->brgInfo->setText("Bearing: " + QString::number(angle, 3, 1));

}
