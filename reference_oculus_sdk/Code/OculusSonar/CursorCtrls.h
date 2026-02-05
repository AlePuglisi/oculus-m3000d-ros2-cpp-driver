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

#ifndef CURSORCTRLS_H
#define CURSORCTRLS_H

#include <QWidget>

namespace Ui {
class CursorCtrls;
}

class CursorCtrls : public QWidget
{
    Q_OBJECT

public:
    explicit CursorCtrls(QWidget *parent = 0);
    ~CursorCtrls();


    void UpdateMouseInfo(float dist, float angle, float x, float y);

private:
    Ui::CursorCtrls *ui;
};

#endif // CURSORCTRLS_H
