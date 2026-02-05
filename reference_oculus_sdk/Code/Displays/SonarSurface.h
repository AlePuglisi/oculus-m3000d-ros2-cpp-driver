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

#include "../RmGl/RmGlSurface.h"

// ----------------------------------------------------------------------------
// SonarSurface - displays sonar data in a fan display

class SonarSurface : public RmGlSurface
{
  Q_OBJECT

public:
  explicit SonarSurface();
  ~SonarSurface();

  // Overrides
  void Render()                                 Q_DECL_OVERRIDE;
  void OnCreate()                               Q_DECL_OVERRIDE;
  void OnResize(int w, int h)                   Q_DECL_OVERRIDE;

  void mouseMoveEvent(QMouseEvent* pEvent)      Q_DECL_OVERRIDE;
  void mousePressEvent(QMouseEvent* pEvent)     Q_DECL_OVERRIDE;
  void mouseReleaseEvent(QMouseEvent* pEvent)   Q_DECL_OVERRIDE;

  // Methods
  void InitTextures();

  void RenderBackground();
  void RenderImg();
  void RenderImgRgb();
  void RenderGrid();
  void RenderMeasureLine();
	void RenderBranding();
  void CalcGrid();
  void Recalculate();
  void AddDataToImg();
  void AddRgbDataToImg();

  bool AreClockwise(QPoint ct, float radius, float angle, QPoint pt);
  bool IsInsideSector(QPoint pt, QPoint ct, float radius, float angle1, float angle2);
  bool IsInsideFan(QPoint pos);

	void SetLeftBranding(QString file);
	void SetRightBranding(QString file);

	void ToggleGridLines();

public slots:
  void UpdateFan(double rng, int nBrgs, short* pBrgs, bool updateProjection = false);
  void UpdateImg(int nRngs, int nBrgs, uchar* pData);
  void UpdateImg16(int nRngs, int nBrgs, quint16* pData);

public:
  // Data
  unsigned m_textureId;    // The image texture id
  unsigned m_nBrgs;
  unsigned m_nRngs;
  quint8   m_nBits;        // Number of bits stored in the local image
  double   m_range;
	bool     m_freqChange;
  double   m_left;         // Angle of the left of the fan
  double   m_right;        // Angle of the right axis of the fan
  bool     m_dwGrid;       // The grid display toggle
  bool     m_dwGridText;   // The grid display toggle
  float    m_mkr;          // Maker frequency of the axis grid
  int      m_nArcs;        // Number of arcs to render
  bool     m_flipY;        // Flip the vertical axis (of the texture)
  bool     m_flipX;        // Flip the horizontal axis (of the texture)
  bool     m_headDown;     // Invert the direction of the fan display to point down
  bool     m_newImgData;   // Has the image been updated

  float*   m_pImgVbo;      // Vertex buffer object for the image
  float*   m_pGridVbo;     // Vertex buffer objectc for the grid
  short*   m_pBrgs;        // The bearing table
  uchar*   m_pData;        // The last data for this image
  uchar*   m_pRgbData;     // The RGB data to use for this image
  bool     m_useRgb;       // Use an RGB image rather than the luminance

  int      m_width;
  int      m_height;

  bool     m_disconnected;
  bool     m_measureEnable;
  bool     m_measuring;
  bool     m_showLastMeasurement;

  float    m_measureStartX;
  float    m_measureStartY;
  float    m_measureEndX;
  float    m_measureEndY;

  bool     m_wasInsideFan;

  bool		m_showGrid;

  // Text buffer to draw grid information into
  RmGlTextBuffer m_gridText;

  RmGlTextBuffer m_measureText;

signals:

  void MouseInfo(float dist, float angle, float x, float y);

  void MouseEnter();
  void MouseLeave();

};

