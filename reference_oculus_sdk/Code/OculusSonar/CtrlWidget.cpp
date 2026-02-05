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

#include "CtrlWidget.h"

#include <QPainter>


QFont  CtrlWidget::s_titleFont   = QFont("Calibri", 12, QFont::Normal, true);
QBrush CtrlWidget::s_bpBlueBrush = QBrush(QColor(0, 158, 223));
QPen   CtrlWidget::s_bpBluePen   = QPen(QColor(0, 158, 223));
QBrush CtrlWidget::s_bpGreyBrush = QBrush(QColor(66, 66, 66));
QPen   CtrlWidget::s_bpGreyPen   = QPen(QColor(66, 66, 66));

CtrlWidget::CtrlWidget(QWidget *pParent) :
  QWidget(pParent)
{
}

// ----------------------------------------------------------------------------
// The object is basically transparent but wee need a fill for windows text
void CtrlWidget::paintEvent(QPaintEvent* pEvent)
{
  Q_UNUSED(pEvent);

  // Get the window area
  QRect r = this->rect();
  QRect all = this->rect();

  QPainter painter;

  painter.begin(this);

  if (m_ctrlTitle.length() > 0)
  {
    painter.setPen(s_bpGreyPen);
    painter.setBrush(s_bpGreyBrush);

    // Draw the title text
    painter.setFont(s_titleFont);
    painter.setPen(s_bpBluePen);

    QTextOption o;
    o.setAlignment(Qt::AlignLeft | Qt::AlignTop);
    r.setTop(2);
    r.setLeft(4);
    painter.drawText(r, m_ctrlTitle, o);
  }
  else
  {
    painter.setPen(s_bpGreyPen);
    painter.setBrush(s_bpGreyBrush);
  }


  painter.end();
}

