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

#include <QObject>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QMatrix4x4>
#include <QVector2D>
#include <QOpenGLShaderProgram>
#include <QDateTime>
#include <QPainter>
#include <QFont>
#include <QWindow>
#include <QStylePainter>
#include <QLabel>

//#include "RmGlOrtho.h"

class RmGlSurface;

//enum eProgram : int
//{
//  pgmSolid,
//  pgmTexture,
//  pgmPalette,
//  pgmAlpha,
//  pgmLuminance,
//  pgmKey
//};

//enum eTextAlign : int
//{
//  taLeft,
//  taCentre,
//  taRight,
//  taTop,
//  taBottom
//};

//// Onscreen tools types
//#define RMGL_TOOL_BUTTON 0
//#define RMGL_TOOL_SLIDER 1



// ----------------------------------------------------------------------------
// Provides some shaders and a basic 2d ortho projection
class RmGlWidget : public QOpenGLWidget
{
  Q_OBJECT

public:
  RmGlWidget(QWidget* pParent = 0);
  virtual ~RmGlWidget();

  // Overrides
  void paintGL()                                            Q_DECL_OVERRIDE;
  void initializeGL()                                       Q_DECL_OVERRIDE;
  void resizeGL(int w, int h)                               Q_DECL_OVERRIDE;

  void mouseMoveEvent(QMouseEvent* pEvent)                  Q_DECL_OVERRIDE;
  void mousePressEvent(QMouseEvent* pEvent)                 Q_DECL_OVERRIDE;
  void mouseReleaseEvent(QMouseEvent* pEvent)               Q_DECL_OVERRIDE;
  void mouseDoubleClickEvent(QMouseEvent* pEvent)           Q_DECL_OVERRIDE;
  void wheelEvent(QWheelEvent* pEvent)                      Q_DECL_OVERRIDE;

  void paintEvent(QPaintEvent *event)                       Q_DECL_OVERRIDE;

  void RmglSetupSurface(RmGlSurface* pSurface);

	void showBranding(bool);


  //virtual int heightForWidth(int width) const;

  RmGlSurface* RmglGetSurface() { return m_pSurface; }

  RmGlSurface* m_pSurface;

  QLabel m_version;

public slots:
  void OnUpdate();
  void SetCursor(const QCursor& cursor);
};

//// ----------------------------------------------------------------------------
//// Provides some shaders and a basic 2d ortho projection
//class RmGlWidget : public QOpenGLWidget, public QOpenGLFunctions, public RmGl2dOrtho
//{
//public:
//  RmGlWidget(QWidget* pParent = 0);
//  ~RmGlWidget();

//  // Overrides
//  void initializeGL() Q_DECL_OVERRIDE;

//  // Methods
//  void rmglInitShaders();
//  void rmglInitPalette();
//  void rmglInitFont();
//  void rmglSetColour(QColor colour);
//  void rmglSetPgm(eProgram pgm);
//  void rmglText(float x, float y, float rot, QString text, eTextAlign horz = taLeft, eTextAlign vert = taBottom);
//  void rmglRenderBB(float* pBB, unsigned texId, eProgram pgm);

//  // Data
//  QVector2D    m_mouseDown;    // Last mouse down position
//  QColor       m_colour;       // Current solid colour (used by Solid and Alpha)
//  QColor       m_colKey;       // Current key colour (used by Key)
//  int          m_palIndex;     // Palette index used by the palette texture (0.0 - 1.0)
//  unsigned     m_palTexId;     // Palette texture id
//  int          m_nPalettes;    // Number of palettes (derived from image height / 10)
//  unsigned     m_fontTexId;    // Font renderer texture id
//  QPainter     m_fontPainter;  // The painter used to render qt fonts
//  QImage       m_fontImg;      // Current font image (max 4096x256)
//  QFont        m_font;         // Font used for rendering
//  char*        m_pFontImg;     // Image buffer (max 4096x256)
//  float        m_fontBB[16];   // Font billboard
//  int          m_pgmId;        // Id of the currently selected program

//  // Shaders
//  QOpenGLShaderProgram m_pgmTexture;   // Basic Texture
//  QOpenGLShaderProgram m_pgmPalette;   // Texture is palettised using pal index
//  QOpenGLShaderProgram m_pgmSolid;     // Solid colour render
//  QOpenGLShaderProgram m_pgmAlpha;     // Texture is used as alpha value (used by fonts)
//  QOpenGLShaderProgram m_pgmLuminance; // Texture (alpha is set to the lumionance)
//  QOpenGLShaderProgram m_pgmKey;       // colour key
//};
