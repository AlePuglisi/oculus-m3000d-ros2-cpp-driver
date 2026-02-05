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

#pragma once

#include <QWidget>

// ----------------------------------------------------------------------------
// Draws the palette used by RmglSurfaces - allows user to select

class PalWidget : public QWidget
{
  Q_OBJECT
public:
  explicit PalWidget(QWidget *parent = 0);

  // Overrides
  void paintEvent(QPaintEvent *pEvent)          Q_DECL_OVERRIDE;
  void mouseMoveEvent(QMouseEvent* pEvent)      Q_DECL_OVERRIDE;
  void mousePressEvent(QMouseEvent* pEvent)     Q_DECL_OVERRIDE;
  void leaveEvent(QEvent *pEvent)               Q_DECL_OVERRIDE;

  // Data
  int    m_palette;              // Current selected palette
  int    m_over;                 // palette that the mouse is over
  int    m_nPalettes;            // Number of palettes on display
  QImage m_palImage;             // The palette image

signals:
  void PalSelected(int pal);

public slots:
};

