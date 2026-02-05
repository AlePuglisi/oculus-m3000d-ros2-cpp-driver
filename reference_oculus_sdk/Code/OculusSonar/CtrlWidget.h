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

#ifndef CTRLWIDGET_H
#define CTRLWIDGET_H

#include <QWidget>

class CtrlWidget : public QWidget
{
  Q_OBJECT

public:
  CtrlWidget(QWidget *pParent = 0);

  void paintEvent(QPaintEvent *pEvent)   Q_DECL_OVERRIDE;

  // Data
  QString m_ctrlTitle;

public:
  static QFont  s_titleFont;
  static QBrush s_bpBlueBrush;
  static QPen   s_bpBluePen;
  static QBrush s_bpGreyBrush;
  static QPen   s_bpGreyPen;

};

#endif // CTRLWIDGET_H
