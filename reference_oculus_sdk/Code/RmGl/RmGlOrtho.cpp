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

#include "RmGlOrtho.h"
#include <QtMath>
#include "../RmUtil/RmUtil.h"

RmGl2dOrtho::RmGl2dOrtho()
{
  m_ppu      = 1.0;   // Pixels per unit
  m_ppuY     = 1.0;   // Pixels per unit (Y Axis)
  m_width    = 100;
  m_height   = 100;
  m_rotation = 0.0;
  m_originX  = 0.0;
  m_originY  = 0.0;
}

// ----------------------------------------------------------------------------
// Setup the projection for this display
void RmGl2dOrtho::rmgl2dProjection(float left, float right, float bottom, float top, eAspect aspect)
{
  m_vpLeft   = left;
  m_vpRight  = right;
  m_vpTop    = top;
  m_vpBottom = bottom;
  m_aspect   = aspect;

  rmgl2dViewport(m_width, m_height);
}

// ----------------------------------------------------------------------------
// Configure the projection based upon the required space
void RmGl2dOrtho::rmgl2dViewport(int w, int h)
{
  float vpLeft   = m_vpLeft;
  float vpRight  = m_vpRight;
  float vpTop    = m_vpTop;
  float vpBottom = m_vpBottom;

  m_width  = w;
  m_height = h;
  m_ppu    = fabs((vpRight - vpLeft) / (float)w);
  m_ppuY   = fabs((vpTop - vpBottom) / (float)h);

  eAspect aspect = m_aspect;

  // If we need to fit the whole screen in then check for the largest required ppu
  if (m_aspect == maintainXY)
  {
    if (m_ppu < m_ppuY)
      aspect = maintainY;
    else
      aspect = maintainX;
  }

  // Fix one axis based on the other
  switch (aspect)
  {
  // Fit the required X viewport to the display viewport, fix Y accordingly
  case maintainX:
    {
      m_ppuY = m_ppu;

      float cY = (vpBottom + vpTop) / 2.0f;

      if (m_vpTop > m_vpBottom)
      {
        vpBottom = cY - m_ppuY * (float)h / 2.0f;
        vpTop    = cY + m_ppuY * (float)h / 2.0f;
      }
      else
      {
        vpBottom = cY + m_ppuY * (float)h / 2.0f;
        vpTop    = cY - m_ppuY * (float)h / 2.0f;
      }
    }
    break;

  // Fit the required Y viewport to the display viewport, fix X accordingly
  case maintainY:
    {
      m_ppu = m_ppuY;

      float cX = (vpLeft + vpRight) / 2.0f;

      if (m_vpRight > m_vpLeft)
      {
        vpLeft  = cX - m_ppu * (float)w / 2.0f;
        vpRight = cX + m_ppu * (float)w / 2.0f;
      }
      else
      {
        vpLeft  = cX + m_ppu * (float)w / 2.0f;
        vpRight = cX - m_ppu * (float)w / 2.0f;
      }
    }
    break;

  // Avoid warnings
  case none:
  case maintainXY:
    break;
  }

  m_projection.setToIdentity();
  m_projection.ortho(vpLeft, vpRight, vpBottom, vpTop, -1.0, 1.0);
	m_projection.rotate(m_rotation, 0.0, 0.0, 1.0);
	/*
	m_projection.ortho(vpTop, vpBottom, vpRight, vpLeft, -1.0, 1.0);
	m_projection.rotate(-90.0, 0.0, 0.0, 1.0);
*/
  // Record the results of the aspect control in the actuals
  m_vpLeftActual   = vpLeft;
  m_vpRightActual  = vpRight;
  m_vpTopActual    = vpTop;
  m_vpBottomActual = vpBottom;

//  qDebug() << "RmGlWidget::rmglViewport w = " + QString::number(w) + ", h = " + QString::number(h) + " Aspect:" + QString::number(aspect)
//              + " Left:"   + QString::number(vpLeft) + " Right:" + QString::number(vpRight)
//              + " Bottom:" + QString::number(vpBottom) + " Top:" + QString::number(vpTop);
}

// ----------------------------------------------------------------------------
// Convert XY to screen coordinates based on current projection
void RmGl2dOrtho::XyToScreen(float x, float y, float& sx, float& sy)
{
  sx = (float)RmUtil::Interpolate(m_vpLeftActual, m_vpRightActual, x, 0, m_width);
  sy = (float)RmUtil::Interpolate(m_vpTopActual, m_vpBottomActual, y, 0, m_height);
}

// ----------------------------------------------------------------------------
// Convert screen coordinates to XY based on current projection
void RmGl2dOrtho::ScreenToXy(float sx, float sy, float& x, float& y)
{
  x = (float)RmUtil::Interpolate(0, m_width, sx, m_vpLeftActual, m_vpRightActual);
  y = (float)RmUtil::Interpolate(0, m_height, sy, m_vpTopActual, m_vpBottomActual);
}


