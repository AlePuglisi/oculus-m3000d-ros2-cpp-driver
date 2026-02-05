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

#include <QOpenGLFunctions>
#include <QMatrix4x4>
#include <QVector2D>
#include <QOpenGLShaderProgram>
#include <QDateTime>
#include <QPainter>
#include <QFont>
#include <QWindow>

#include "RmGlOrtho.h"



// List of built in shader programs
enum eProgram : int
{
  pgmSolid,
  pgmTexture,
  pgmPalette,
  pgmAlpha,
  pgmLuminance,
  pgmKey
};

// Text alignment options
enum eTextAlign : unsigned
{
  taLeft     = 0x01,
  taHCentre  = 0x02,
  taRight    = 0x04,
  taTop      = 0x08,
  taVCentre  = 0x10,
  taBottom   = 0x20,
  taFlipX    = 0x40,
  taFlipY    = 0x80
};

// ----------------------------------------------------------------------------
// Store individual character information about the font atlas
class GlyphInfo
{
public:
  char  c;       // The character
  int   width;   // The width
  int   height;  // The width
  float s0;      // Texture s0 coord withing the atlas
  float s1;      // Texture s1 coord withing the atlas
  float t0;      // Texture t0 coord withing the atlas
  float t1;      // Texture t1 coord withing the atlas
};

#define GLYPH_BASE 32
#define N_GLYPHS   94


// ----------------------------------------------------------------------------
// RmGlTextBuffer - a buffer to store text information - this can be added to and
// rendered by the RmGlSurface
class RmGlTextBuffer
{
public:
  RmGlTextBuffer();
  ~RmGlTextBuffer();

  // Methods
  void ClearBuffer();

  // Data
  float* m_pVbo;     // VBO for text
  int    m_nChars;   // Number of characters in the buffer
};


// ----------------------------------------------------------------------------
// RmGlSurface - an abstract opengl object used as a base class for opengl surfaces
// This class allows applications to choose between QWindow and QWidget design plans
// but reuse opengl code

class RmGlSurface : public QObject, public QOpenGLFunctions, public RmGl2dOrtho
{
  Q_OBJECT

public:
  RmGlSurface();
  virtual ~RmGlSurface();

  // Overridables
  virtual void Render() = 0;
  virtual void OnResize(int w, int h) = 0;
  virtual void OnCreate();

  // These virtual functions allow widget based systems to pass through mouse events
  virtual void mouseMoveEvent(QMouseEvent* pEvent)         { Q_UNUSED(pEvent) }
  virtual void mousePressEvent(QMouseEvent* pEvent)        { Q_UNUSED(pEvent) }
  virtual void mouseReleaseEvent(QMouseEvent* pEvent)      { Q_UNUSED(pEvent) }
  virtual void mouseDoubleClickEvent(QMouseEvent* pEvent)  { Q_UNUSED(pEvent) }
  virtual void wheelEvent(QWheelEvent *pEvent)             { Q_UNUSED(pEvent) }

  // Methods
  int  RmglCompileShader(int type, QString shaderSource);
  int  RmglLinkProgram(int vertex, int fragment);
  int  RmglBuildProgram(QString vertexSource, QString fragmnetSource);
  void RmglInitShaders();
  void RmglInitPalette();
  void RmglInitFont();
  void RmglInitMarkers();
  void RmglSetFont(QFont font);
  void RmglSetBackground(QString background);
  void RmglSetColour(QColor colour);
  void RmglSetPgm(eProgram pgm);
  void RmglRenderVbo(float* pVbo, unsigned texId, eProgram pgm);
  void RmglAddText(RmGlTextBuffer& buffer, float x, float y, float rot, QString text, unsigned ta = (taLeft | taBottom));
  void RmglRenderText(RmGlTextBuffer& buffer);
  void RmglRenderBackground(QRect rect);
  void RmglRenderOverlay();
  bool RmglLoadImageToTexture(QString imgName, unsigned& texId);
  void RmglRenderMarker(float x, float y, float rot, float scale, unsigned marker, QColor colour, QString text);
  void RmglRenderLine(float x0, float y0, float x1, float y1, QColor colour);
  void RmglRenderPolyline(float* pPts, int nPts, QColor colour);
  void RmglRenderPixelRect(int x0, int y0, int x1, int y1, QColor colour);

	void RmglRenderLeftBranding();
	void RmglRenderRightBranding();
	void RmglSetLeftBranding(QString image);
	void RmglSetRightBranding(QString image);
	void RmglShowBranding(bool enable);

  // Data
  QColor    m_clearColour;             // Clear Colour to be used
  QColor    m_colour;                  // Current solid colour (used by Solid and Alpha)
  QColor    m_colKey;                  // Current key colour (used by Key)
  QFont     m_font;                    // Font to use for background text
  QString   m_background;              // Name of the background resource
  int       m_palIndex;                // Palette index used by the palette texture
  unsigned  m_palTexId;                // Palette texture id
  unsigned  m_backgroundId;            // The background texture id
  unsigned  m_overlayId;               // The overlay texture id
  int       m_nPalettes;               // Number of palettes (derived from image height / 10)
  int       m_pgmId;                   // Id of the currently selected program

	QString		m_leftBranding;
	unsigned	m_leftBrandingId;
	QString		m_rightBranding;
	unsigned	m_rightBrandingId;

	QRect			m_leftRect;
	QRect			m_rightRect;

	bool			m_showBranding;

  // Text generation
  unsigned  m_fontTexId;               // Palette texture id
  GlyphInfo m_fontInfo[N_GLYPHS];      // Font atlas information

  // Shaders
  int m_pgmTexture;                    // Basic Texture
  int m_pgmPalette;                    // Texture is palettised using pal index
  int m_pgmSolid;                      // Solid colour render
  int m_pgmAlpha;                      // Texture is used as alpha value (used by fonts)
  int m_pgmLuminance;                  // Texture (alpha is set to the luminance)
  int m_pgmKey;                        // colour key

  // Markers
  unsigned m_arrowId;                  // Arrow marker
  unsigned m_squareId;                 // Square marker
  unsigned m_circleId;                 // Circle marker
  unsigned m_diamondId;                // Diamond marker

signals:
  void Update();
  void SetCursor(const QCursor& cursor);
};


