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

#include <QMatrix4x4>

// ----------------------------------------------------------------------------
// RmGl2dOrtho - adds in some aspect controls for 2d ortho windows

enum eAspect
{
  none,
  maintainX,
  maintainY,
  maintainXY,
};

class RmGl2dOrtho
{
public:
  RmGl2dOrtho();

  // Methods
  void rmgl2dViewport(int w, int h);
  void rmgl2dProjection(float left, float right, float bottom, float top, eAspect aspect = maintainXY);

  void XyToScreen(float x, float y, float& sx, float& sy);
  void ScreenToXy(float sx, float sy, float& x, float& y);

  // Data
  QMatrix4x4   m_projection;           // Current display projection
  float        m_originX;              // Origin - all coords have this removed durig rendering
  float        m_originY;              // Oriign y value

  eAspect      m_aspect;               // How to aspect control

  float        m_vpLeft;               // Viewport left    - demanded
  float        m_vpRight;              // Viewport right   - demanded
  float        m_vpTop;                // Viewport top     - demanded
  float        m_vpBottom;             // Viewport bottom  - demanded
  float        m_rotation;             // Viewport rotation

  float        m_vpLeftActual;         // Viewport left    - after aspect control
  float        m_vpRightActual;        // Viewport right   - after aspect control
  float        m_vpTopActual;          // Viewport top     - after aspect control
  float        m_vpBottomActual;       // Viewport bottom  - after aspect control

  int          m_width;                // Width of this ortho in pixels
  int          m_height;               // Height of this ortho in pixels
  float        m_ppu;                  // Pixels per unit
  float        m_ppuY;                 // Pixels per unit Y (for non aspect display)
};

