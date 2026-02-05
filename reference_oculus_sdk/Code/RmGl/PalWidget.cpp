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

#include "PalWidget.h"

#include <QPainter>
#include <QDebug>
#include <QMouseEvent>

PalWidget::PalWidget(QWidget *parent) : QWidget(parent)
{
  m_palette   = 0;        // Current selected palette
  m_over      = -1;       // palette that the mouse is over
  m_nPalettes = 1;        // Number of palettes on display

  // Try to load the image from the resource
  if (m_palImage.load(":/RmGl/Media/palettes.bmp"))
  {
    // The palette is assumed to have 10 pixels per palette in height
    m_nPalettes = m_palImage.height() / 10;
  }
  else
  // Create a greyscale image
  {
    qDebug() << "PalWidget, Cannot load palettes bitmap from resource, using default greyscale";

    // Create a default greyscale image
    unsigned char pal[256 * 10 * 4];

    for (int j = 0; j < 10; j++)
    {
      for (int i = 0; i < 256; i++)
      {
        pal[i + j * 256 + 0] = i;
        pal[i + j * 256 + 1] = i;
        pal[i + j * 256 + 2] = i;
        pal[i + j * 256 + 3] = 255;
      }
    }

    m_palImage = QImage(pal, 256, 10, QImage::Format_ARGB32);
  }

  // Constrain the widget to be the same size as the palette bitmap
  setMinimumSize(m_palImage.size());
  setMaximumSize(m_palImage.size());

  // Get access to mouse tracking information
  setMouseTracking(true);
}

// ----------------------------------------------------------------------------
// Draw the image, highlight the selected image and the palette under the mouse
void PalWidget::paintEvent(QPaintEvent* pEvent)
{
  Q_UNUSED(pEvent);

  // Get the dimensions of the toolbar
  QRect r = rect();

  QPainter painter;
  painter.begin(this);

  painter.drawImage(r, m_palImage);

  QRect selected;

  selected.setLeft(0);
  selected.setRight(r.width() - 1);
  selected.setTop(r.height() * m_palette / m_nPalettes);
  selected.setBottom(r.height() * (m_palette + 1) / m_nPalettes);

  painter.setPen(QPen(Qt::white, 2));

  painter.drawRoundedRect(selected, 2, 2);

  if (m_over >= 0)
  {
    QRect over;

    over.setLeft(0);
    over.setRight(r.width() - 1);
    over.setTop(r.height() * m_over / m_nPalettes);
    over.setBottom(r.height() * (m_over + 1) / m_nPalettes);

    painter.setPen(QPen(Qt::black, 2));

    painter.drawRoundedRect(over, 2, 2);
  }


  painter.end();
}

// ----------------------------------------------------------------------------
// Track the mouse over the palette selection
void PalWidget::mouseMoveEvent(QMouseEvent* pEvent)
{
  int pal = m_nPalettes * pEvent->pos().y() / height();
  int over = -1;

  if (pal < m_nPalettes)
    over = pal;

  if (over != m_over)
  {
    m_over = over;
    update();
  }
}

// ----------------------------------------------------------------------------
// Select the palette if it has changed
void PalWidget::mousePressEvent(QMouseEvent* pEvent)
{
  int pal = m_nPalettes * pEvent->pos().y() / height();
  int over = -1;

  if (0 <= pal && pal < m_nPalettes)
  {
    m_palette = pal;
    update();

    emit PalSelected(m_palette);
  }
}

// ----------------------------------------------------------------------------
// Clear the selection
void PalWidget::leaveEvent(QEvent* pEvent)
{
  if (m_over >= 0)
  {
    m_over = -1;
    update();
  }
}
