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

#include "RmGlSurface.h"

#include <QMouseEvent>
#include <QtMath>
#include <QFile>

#include "../RmUtil/RmUtil.h"

// ============================================================================
// RmGlTextBuffer - a buffer to store text information - this can be added to and
// rendered by the RmGlSurface

RmGlTextBuffer::RmGlTextBuffer()
{
  m_pVbo   = nullptr;
  m_nChars = 0;
}

RmGlTextBuffer::~RmGlTextBuffer()
{
  ClearBuffer();
}

// ----------------------------------------------------------------------------
// Clear the buffer of any data
void RmGlTextBuffer::ClearBuffer()
{
  // Delete any text buffer contents
  if (m_pVbo)
    delete m_pVbo;

  m_pVbo   = nullptr;
  m_nChars = 0;
}


// ============================================================================
// RmGlSurface - an abstract opengl object used as a base class for opengl surfaces
// This class allows applications to choose between QWindow and QWidget design plans
// but reuse opengl code


RmGlSurface::RmGlSurface()
{
  m_palIndex       = 1;     // Palette index used by the palette texture (0.0 - 1.0)
  m_palTexId       = 0;     // Palette texture id
  m_nPalettes      = 1;     // Number of palettes (derived by palette image height / 10)
  m_fontTexId      = 0;     // Font texture id
  m_pgmId          = 0;     // Current program id
  m_backgroundId   = 0;
	m_leftBrandingId = 0;
	m_rightBrandingId = 0;
  m_overlayId      = 0;
  m_showBranding = false;
  m_clearColour.setRgb(0, 0, 0);


}

RmGlSurface::~RmGlSurface()
{
  // Delete the palette texture
  if (m_palTexId)
    glDeleteTextures(1, &m_palTexId);

  // Delete the font texture
  if (m_fontTexId)
    glDeleteTextures(1, &m_fontTexId);

  // Delete the background texture
  if (m_backgroundId)
    glDeleteTextures(1, &m_backgroundId);

  // Delete the overlay texture
  if (m_overlayId)
		glDeleteTextures(1, &m_overlayId);

	// Delete the branding texture
	if (m_leftBrandingId)
		glDeleteTextures(1, &m_leftBrandingId);

	if (m_rightBrandingId)
		glDeleteTextures(1, &m_rightBrandingId);
}

// ----------------------------------------------------------------------------
// Setup the ogl stuff
void RmGlSurface::OnCreate()
{
  initializeOpenGLFunctions();

  RmglInitShaders();
  RmglInitPalette();
  RmglInitFont();
  RmglInitMarkers();
  RmglSetBackground(m_background);

  // Default branding
  RmglSetLeftBranding(":/Logo_Blueprint_256.png");
  RmglSetRightBranding(":/Logo_Oculus_256.png");
}

// ----------------------------------------------------------------------------
// Compile a shader from the give string resource
int RmGlSurface::RmglCompileShader(int type, QString shaderSource)
{
  // Create a new shader object.
  int shaderId = glCreateShader(type);

  // Creation failed
  if (shaderId == 0)
    return 0;

  // Read in the shader data
  QFile shaderFile(shaderSource);

  if (!shaderFile.open(QIODevice::ReadOnly))
    return 0;

  QByteArray ba = shaderFile.readAll();

  shaderFile.close();

  const char* data[]    = { ba.data()   };
  const int   lengths[] = { ba.length() };

  // Pass in the shader source.
  glShaderSource(shaderId, 1, data, lengths);

  // Compile the shader.
  glCompileShader(shaderId);

  // Get the compilation status.
  int compileStatus;
  glGetShaderiv(shaderId, GL_COMPILE_STATUS, &compileStatus);

  // Verify the compile status - If it failed, delete the shader object.
  if (compileStatus == 0)
  {
    glDeleteShader(shaderId);
    return 0;
  }

  // Return the shader object ID.
  return shaderId;
}

// --------------------------------------------------------------------------
// Link a vertex shader and a fragment shader into a program and return the id
int RmGlSurface::RmglLinkProgram(int vertex, int fragment)
{
  // Create a new program object.
  int program = glCreateProgram();

  if (program == 0)
    return 0;

  // Attach the vertex shader to the program.
  glAttachShader(program, vertex);

  // Attach the fragment shader to the program.
  glAttachShader(program, fragment);

  // Link the shaders into a program.
  glLinkProgram(program);

  // Get the link status.
  int status;
  glGetProgramiv(program, GL_LINK_STATUS, &status);

  // Verify the link status, delete if failed
  if (status == 0)
  {
    // If it failed, delete the program object.
    glDeleteProgram(program);
    return 0;
  }

  // Return the program object ID.
  return program;
}

// ----------------------------------------------------------------------------
// Build a shader program and return the id (0 on error)
int RmGlSurface::RmglBuildProgram(QString vertexSource, QString fragmentSource)
{
  int program = 0;

  // Compile the shaders.
  int vertex   = RmglCompileShader(GL_VERTEX_SHADER,   vertexSource);
  int fragment = RmglCompileShader(GL_FRAGMENT_SHADER, fragmentSource);

  // Link them into a shader program.
  if (vertex && fragment)
    program = RmglLinkProgram(vertex, fragment);

  return program;
}


// ----------------------------------------------------------------------------
// Create the internal shaders
void RmGlSurface::RmglInitShaders()
{
  m_pgmSolid     = RmglBuildProgram(":/RmGl/Shaders/solid.vsh",   ":/RmGl/Shaders/solid.fsh");
  m_pgmTexture   = RmglBuildProgram(":/RmGl/Shaders/texture.vsh", ":/RmGl/Shaders/texture.fsh");
  m_pgmAlpha     = RmglBuildProgram(":/RmGl/Shaders/texture.vsh", ":/RmGl/Shaders/alpha.fsh");
  m_pgmPalette   = RmglBuildProgram(":/RmGl/Shaders/texture.vsh", ":/RmGl/Shaders/palette.fsh");
  m_pgmLuminance = RmglBuildProgram(":/RmGl/Shaders/texture.vsh", ":/RmGl/Shaders/luminance.fsh");
  m_pgmKey       = RmglBuildProgram(":/RmGl/Shaders/texture.vsh", ":/RmGl/Shaders/colourkey.fsh");
}



// ----------------------------------------------------------------------------
// Load and setup the texture for the font renderer
void RmGlSurface::RmglSetFont(QFont font)
{
  // Get font metrics
  QFontMetrics fm(font);

  int width  = 0;
  int height = fm.height();

  for (int c = 0; c < N_GLYPHS; c++ )
  {
    m_fontInfo[c].c = c + GLYPH_BASE;
    m_fontInfo[c].height = height;
    m_fontInfo[c].width  = fm.width(m_fontInfo[c].c);

    width  += m_fontInfo[c].width + 2;
  }

  // Square off the image
  if (width > 0 && height > 0)
  {
    while (width > height * 2)
    {
      width  /= 2;
      height *= 2;

      // Lets have them in power of 2
      width  = RmUtil::Pow2Up(width);
      height = RmUtil::Pow2Up(height);
    }
  }

  QImage img(width, height, QImage::Format_ARGB32);
  img.fill(0x00000000);

  QPainter painter;
  QRect    r(0, 0, width, height);

  painter.begin(&img);
  painter.setRenderHints(QPainter::HighQualityAntialiasing | QPainter::TextAntialiasing);
  painter.setFont(font);

  QPen pen(QColor("#ffffffff"));
  painter.setPen(pen);


  // Render the fonts into the bitmap
  int x = 0;
  int y = 0;

  for (int c = 0; c < N_GLYPHS; c++ )
  {
    QRect r;

    if (x + m_fontInfo[c].width > width)
    {
      x  = 0;
      y += fm.height();
    }

    painter.drawText(x, y, m_fontInfo[c].width, m_fontInfo[c].height, 0, QString(m_fontInfo[c].c));

    m_fontInfo[c].s0    = (float)x / (float)width;
    m_fontInfo[c].s1    = m_fontInfo[c].s0 + (float)m_fontInfo[c].width / (float)width;
    m_fontInfo[c].t0    = (float)y / (float)height;
    m_fontInfo[c].t1    = m_fontInfo[c].t0 + (float)m_fontInfo[c].height / (float) height;

    x += m_fontInfo[c].width;
  }

  painter.end();

  // Bind the image into the font texture
  glBindTexture(GL_TEXTURE_2D, m_fontTexId);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  glPixelStorei  (GL_UNPACK_ALIGNMENT, 1);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.width(), img.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, img.bits());
}

// ----------------------------------------------------------------------------
// Setup a background texture
void RmGlSurface::RmglSetBackground(QString background)
{
  if (background.length() > 0)
  {
    QImage img;

    if (img.load(background))
    {
      img = img.rgbSwapped();

      // Generate a texture for the bacxkground data (if one doesn't already exist)
      if (!m_backgroundId)
        glGenTextures(1, &m_backgroundId);

      glBindTexture(GL_TEXTURE_2D, m_backgroundId);


      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.width(), img.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, img.bits());
    }
    else
      qDebug() << "Background image load failed: " + background;

  }
}

void RmGlSurface::RmglSetLeftBranding(QString image) {

	if (image.length() > 0) {

		QImage img;

		if (img.load(image)) {
			m_leftRect = img.rect();

			img = img.rgbSwapped();

			// Generate a texture
			if (!m_leftBrandingId)
				glGenTextures(1, &m_leftBrandingId);

			glBindTexture(GL_TEXTURE_2D, m_leftBrandingId);

			//qDebug() << "Set left branding: " << image;

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.width(), img.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, img.bits());
		}
		else
			qDebug() << "Branding image load failed: " + image;

	}


}


void RmGlSurface::RmglSetRightBranding(QString image) {

	if (image.length() > 0) {

		QImage img;

		if (img.load(image)) {
			m_rightRect = img.rect();

			img = img.rgbSwapped();

			// Generate a texture
			if (!m_rightBrandingId)
				glGenTextures(1, &m_rightBrandingId);

			glBindTexture(GL_TEXTURE_2D, m_rightBrandingId);

			//qDebug() << "Set right branding: " << image;

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.width(), img.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, img.bits());
		}
		else
			qDebug() << "Branding image load failed: " + image;

	}


}

void RmGlSurface::RmglShowBranding(bool enable) {
	m_showBranding = enable;
}

// ----------------------------------------------------------------------------
// Create the font texture and setup a default font
void RmGlSurface::RmglInitFont()
{
  // Generate a texture for the text data
  glGenTextures(1, &m_fontTexId);

  // KW
  // Increase the font size used for the range labels
  m_font.setFamily("Verdana");
  m_font.setPointSize(16);
  // END KW

  RmglSetFont(m_font);
}

// ----------------------------------------------------------------------------
// Create the marker textures and load from the standard resources
void RmGlSurface::RmglInitMarkers()
{
  RmglLoadImageToTexture(":/RmGl/Media/arrow.png",   m_arrowId);
  RmglLoadImageToTexture(":/RmGl/Media/circle.png",  m_circleId);
  RmglLoadImageToTexture(":/RmGl/Media/square.png",  m_squareId);
  RmglLoadImageToTexture(":/RmGl/Media/diamond.png", m_diamondId);
}

// ----------------------------------------------------------------------------
// Load and setup the palette texture for the palette shader
void RmGlSurface::RmglInitPalette()
{
  glGenTextures(1, &m_palTexId);
  glBindTexture(GL_TEXTURE_2D, m_palTexId);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  QImage img;

  if (img.load(":/RmGl/Media/palettes.bmp"))
  {
    img = img.rgbSwapped();

    // The palette is assumed to have 10 pixels per palette in height
    m_nPalettes = img.height() / 10;

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.width(), img.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, img.bits());
  }
  else
  {
    qDebug() << "RmGlWidget::rmglInitPalette, Cannot load palettes bitmap from resource, using default greyscale";

    // Create a default greyscale image
    unsigned char pal[256 * 256 * 4];

    for (int j = 0; j < 256; j++)
    {
      for (int i = 0; i < 256; i++)
      {
        pal[i + j * 256 + 0] = i;
        pal[i + j * 256 + 1] = i;
        pal[i + j * 256 + 2] = i;
        pal[i + j * 256 + 3] = 255;
      }
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, pal);
  }
}

// ----------------------------------------------------------------------------
// Configure the uniform colour for appropriate shaders
void RmGlSurface::RmglSetColour(QColor colour)
{
  m_colour = colour;
}

// ----------------------------------------------------------------------------
// Set the current program to the given program (update any needed uniforms)
void RmGlSurface::RmglSetPgm(eProgram pgm)
{
  switch (pgm)
  {
    case pgmSolid:
      m_pgmId = m_pgmSolid;
      glUseProgram(m_pgmId);
      glUniform4f(glGetUniformLocation(m_pgmId, "uColour"), m_colour.redF(), m_colour.greenF(), m_colour.blueF(), m_colour.alphaF());
      break;

    case pgmTexture:
      m_pgmId = m_pgmTexture;
      glUseProgram(m_pgmId);
      glUniform1i(glGetUniformLocation(m_pgmId, "uTexUnit"), 0);
      break;

    case pgmPalette:
      m_pgmId = m_pgmPalette;
      glUseProgram(m_pgmId);
      glUniform1i(glGetUniformLocation(m_pgmId, "uTexUnit"), 0);
      glUniform1f(glGetUniformLocation(m_pgmId, "uPalIndex"), ((float)m_palIndex + 0.5f) / (float)m_nPalettes);
      glUniform1i(glGetUniformLocation(m_pgmId, "uPalUnit"), 1);
      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, m_palTexId);
      break;

    case pgmAlpha:
      m_pgmId = m_pgmAlpha;
      glUseProgram(m_pgmId);
      glUniform1i(glGetUniformLocation(m_pgmId, "uTexUnit"), 0);
      glUniform4f(glGetUniformLocation(m_pgmId, "uColour"), m_colour.redF(), m_colour.greenF(), m_colour.blueF(), m_colour.alphaF());
      break;

    case pgmLuminance:
      m_pgmId = m_pgmLuminance;
      glUseProgram(m_pgmId);
      glUniform1i(glGetUniformLocation(m_pgmId, "uTexUnit"), 0);
      break;

    case pgmKey:
      m_pgmId = m_pgmKey;
      glUseProgram(m_pgmId);
      glUniform1i(glGetUniformLocation(m_pgmId, "uTexUnit"), 0);
      glUniform1i(glGetUniformLocation(m_pgmId, "uColKey"), m_colKey.rgba());
      break;
  }

  glUniformMatrix4fv(glGetUniformLocation(m_pgmId, "uMatrix"), 1, false, m_projection.data());
  glUniform1f(glGetUniformLocation(m_pgmId, "uOriginX"), m_originX);
  glUniform1f(glGetUniformLocation(m_pgmId, "uOriginY"), m_originY);
}

// ----------------------------------------------------------------------------
// Render the billbaord (with texture corrdinates) using texId and program = pgm
void RmGlSurface::RmglRenderVbo(float *pVbo, unsigned texId, eProgram pgm)
{
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texId);

  RmglSetPgm(pgm);

  // Setup vertex
  int aPosition = glGetAttribLocation(m_pgmId, "aPosition");
  glVertexAttribPointer(aPosition, 2, GL_FLOAT, false, 4 * sizeof(float), pVbo);
  glEnableVertexAttribArray(aPosition);

  // Setup texture
  int aTexCoord = glGetAttribLocation(m_pgmId, "aTexCoords");
  glVertexAttribPointer(aTexCoord, 2, GL_FLOAT, false, 4 * sizeof(float), &pVbo[2]);
  glEnableVertexAttribArray(aTexCoord);

  // Draw Billboard
  if (pgm == pgmLuminance || pgm == pgmKey || pgm == pgmTexture || pgm == pgmAlpha)
  {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }

  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glDisable(GL_BLEND);
}

// ----------------------------------------------------------------------------
// Add text to the text buffer
void RmGlSurface::RmglAddText(RmGlTextBuffer& buffer, float x, float y, float rot, QString text, unsigned ta)
{
  int nChars = text.length();
  int c;

  // Kick out if there is nothing to do
  if (nChars < 1)
    return;

  // Add space into the VBO for the new text
  buffer.m_pVbo = (float*) realloc (buffer.m_pVbo, (buffer.m_nChars + nChars) * sizeof(float) * 24);

  // In case of daft strings
  if (!buffer.m_pVbo)
    return;

  // Setup a matrix that defines the position and rotation of the text
  QMatrix4x4 mat;
  mat.translate(x, y);
  mat.rotate(rot, 0.0, 0.0, 1.0);

  int strWidth  = 0;
  int strHeight = 0;

  // Get the size of the string in pixels
  for (c = 0; c < nChars; c++)
  {
    int index = text.at(c).cell() - GLYPH_BASE;

    if (0 <= index && index < N_GLYPHS)
    {
      strWidth += m_fontInfo[index].width;
      strHeight = (int)MAX(strHeight, m_fontInfo[index].height);
    }
  }

  // Convert pixel widths into screen coordinates
  float dx = (float)strWidth  * m_ppu;
  float dy = (float)strHeight * m_ppuY;

  float* pV = &buffer.m_pVbo[buffer.m_nChars * 24];

  float cx = 0;
  float cy = 0;

  if ((ta & taHCentre) == taHCentre)
    cx -= dx / 2.0;
  else
  if ((ta & taRight) == taRight)
    cx -= dx;

  if ((ta & taVCentre) == taVCentre)
    cy -= dy / 2.0;
  else
  if ((ta & taTop) == taTop)
    cy -= dy;

  // For each chacter we need to define the quad and the texture coordinates
  for (c = 0; c < nChars; c++)
  {
    GlyphInfo* pGlyph = &m_fontInfo[text.at(c).cell() - GLYPH_BASE];

    float dx = (float)pGlyph->width  * m_ppu;
    float dy = (float)pGlyph->height * m_ppuY;

    float s0 = pGlyph->s0;
    float s1 = pGlyph->s1;
    float t0 = pGlyph->t0;
    float t1 = pGlyph->t1;

    if ((ta & taFlipX) == taFlipX)
    {
      s0 = pGlyph->s1;
      s1 = pGlyph->s0;
    }

    if ((ta & taFlipY) == taFlipY)
    {
      t0 = pGlyph->t1;
      t1 = pGlyph->t0;
    }

    QVector3D v0(cx,       cy, 1);         // BL
    QVector3D v1(cx + dx,  cy, 1);         // BR
    QVector3D v2(cx,       cy + dy, 1);    // TL
    QVector3D v3(cx + dx,  cy + dy, 1);    // TR

    v0 = mat * v0;
    v1 = mat * v1;
    v2 = mat * v2;
    v3 = mat * v3;

    cx += dx;

    *pV++ = v0.x();         // Bottom Left X
    *pV++ = v0.y();         // Bottom Left Y
    *pV++ = s0; // Bottom Left S
    *pV++ = t1; // Bottom Left T

    *pV++ = v2.x();         // Top Left X
    *pV++ = v2.y();         // Top Left Y
    *pV++ = s0; // Top Left S
    *pV++ = t0; // Top Left T

    *pV++ = v1.x();         // Bottom Right X
    *pV++ = v1.y();         // Bottom Right Y
    *pV++ = s1; // Bottom Right S
    *pV++ = t1; // Bottom Right T

    *pV++ = v1.x();         // Bottom Right X
    *pV++ = v1.y();         // Bottom Right Y
    *pV++ = s1; // Bottom Right S
    *pV++ = t1; // Bottom Right T

    *pV++ = v3.x();         // Top Right X
    *pV++ = v3.y();         // Top Right Y
    *pV++ = s1; // Top Right S
    *pV++ = t0; // Top Right T

    *pV++ = v2.x();         // Top Left X
    *pV++ = v2.y();         // Top Left Y
    *pV++ = s0; // Top Left S
    *pV++ = t0; // Top Left T

  }

  buffer.m_nChars += nChars;
}

// ----------------------------------------------------------------------------
// Render the contents (if any) of the text buffer
void RmGlSurface::RmglRenderText(RmGlTextBuffer& buffer)
{
  if (buffer.m_pVbo && buffer.m_nChars > 0)
  {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_fontTexId);

    RmglSetPgm(pgmTexture);

    // Setup vertex
    int aPosition = glGetAttribLocation(m_pgmId, "aPosition");
    glVertexAttribPointer(aPosition, 2, GL_FLOAT, false, 4 * sizeof(float), buffer.m_pVbo);
    glEnableVertexAttribArray(aPosition);

    // Setup texture
    int aTexCoord = glGetAttribLocation(m_pgmId, "aTexCoords");
    glVertexAttribPointer(aTexCoord, 2, GL_FLOAT, false, 4 * sizeof(float), &buffer.m_pVbo[2]);
    glEnableVertexAttribArray(aTexCoord);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDrawArrays(GL_TRIANGLES, 0, 6 * buffer.m_nChars);
    glDisable(GL_BLEND);
  }
}

// ----------------------------------------------------------------------------
// Render a solid rectangle to the given pixel area
void RmGlSurface::RmglRenderPixelRect(int x0, int y0, int x1, int y1, QColor colour)
{
  float vbo[8];

  if (m_vpBottomActual > m_vpTopActual)
  {
    vbo[0] = m_vpLeftActual + (float)x0 * m_ppu; vbo[1] = m_vpTopActual + (float)y1 * m_ppuY;
    vbo[2] = m_vpLeftActual + (float)x0 * m_ppu; vbo[3] = m_vpTopActual + (float)y0 * m_ppuY;
    vbo[4] = m_vpLeftActual + (float)x1 * m_ppu; vbo[5] = m_vpTopActual + (float)y1 * m_ppuY;
    vbo[6] = m_vpLeftActual + (float)x1 * m_ppu; vbo[7] = m_vpTopActual + (float)y0 * m_ppuY;
  }
  else
  {
    vbo[0] = m_vpLeftActual + (float)x0 * m_ppu; vbo[1] = m_vpTopActual - (float)y1 * m_ppuY;
    vbo[2] = m_vpLeftActual + (float)x0 * m_ppu; vbo[3] = m_vpTopActual - (float)y0 * m_ppuY;
    vbo[4] = m_vpLeftActual + (float)x1 * m_ppu; vbo[5] = m_vpTopActual - (float)y1 * m_ppuY;
    vbo[6] = m_vpLeftActual + (float)x1 * m_ppu; vbo[7] = m_vpTopActual - (float)y0 * m_ppuY;
  }

  m_colour = colour;
  RmglSetPgm(pgmSolid);

  // Setup vertex
  int aPosition = glGetAttribLocation(m_pgmId, "aPosition");
  glVertexAttribPointer(aPosition, 2, GL_FLOAT, false, 2 * sizeof(float), vbo);
  glEnableVertexAttribArray(aPosition);

  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}



// ----------------------------------------------------------------------------
// Draw the background image (if available)
void RmGlSurface::RmglRenderBackground(QRect rect)
{
  if (m_backgroundId)
  {
    float vbo[16];

    if (m_vpBottomActual > m_vpTopActual)
    {
        vbo[ 0] = m_vpLeftActual + (float)rect.left() * m_ppu;  vbo[ 1] = m_vpTopActual + (float)rect.bottom() * m_ppuY; vbo[ 2] = 0.0; vbo[ 3] = 1.0;
        vbo[ 4] = m_vpLeftActual + (float)rect.left() * m_ppu;  vbo[ 5] = m_vpTopActual + (float)rect.top() * m_ppuY;    vbo[ 6] = 0.0; vbo[ 7] = 0.0;
        vbo[ 8] = m_vpLeftActual + (float)rect.right() * m_ppu; vbo[ 9] = m_vpTopActual + (float)rect.bottom() * m_ppuY; vbo[10] = 1.0; vbo[11] = 1.0;
        vbo[12] = m_vpLeftActual + (float)rect.right() * m_ppu; vbo[13] = m_vpTopActual + (float)rect.top() * m_ppuY;    vbo[14] = 1.0; vbo[15] = 0.0;

    }
    else
    {
        vbo[ 0] = m_vpLeftActual + (float)rect.left() * m_ppu;  vbo[ 1] = m_vpTopActual - (float)rect.bottom() * m_ppuY; vbo[ 2] = 0.0; vbo[ 3] = 1.0;
        vbo[ 4] = m_vpLeftActual + (float)rect.left() * m_ppu;  vbo[ 5] = m_vpTopActual - (float)rect.top() * m_ppuY;    vbo[ 6] = 0.0; vbo[ 7] = 0.0;
        vbo[ 8] = m_vpLeftActual + (float)rect.right() * m_ppu; vbo[ 9] = m_vpTopActual - (float)rect.bottom() * m_ppuY; vbo[10] = 1.0; vbo[11] = 1.0;
        vbo[12] = m_vpLeftActual + (float)rect.right() * m_ppu; vbo[13] = m_vpTopActual - (float)rect.top() * m_ppuY;    vbo[14] = 1.0; vbo[15] = 0.0;

    }

    RmglRenderVbo(vbo, m_backgroundId, pgmTexture);
  }
}


void RmGlSurface::RmglRenderLeftBranding() {

	if (m_leftBrandingId) {

		// Scale the image to fit nicely
		QRect rect = m_leftRect;
		double aspect = ((double)rect.height() / (double)rect.width());
		int width = 256;
		rect.setTopLeft(QPoint(30, 30));
		rect.setWidth(width);
		rect.setHeight(width * aspect);

		float vbo[16];


		if (m_vpBottomActual > m_vpTopActual) {


			vbo[ 0] = m_vpLeftActual + (float)rect.left() * m_ppu;  vbo[ 1] = m_vpTopActual + (float)rect.bottom() * m_ppuY; vbo[ 2] = 0.0; vbo[ 3] = 1.0;
			vbo[ 4] = m_vpLeftActual + (float)rect.left() * m_ppu;  vbo[ 5] = m_vpTopActual + (float)rect.top() * m_ppuY;    vbo[ 6] = 0.0; vbo[ 7] = 0.0;
			vbo[ 8] = m_vpLeftActual + (float)rect.right() * m_ppu; vbo[ 9] = m_vpTopActual + (float)rect.bottom() * m_ppuY; vbo[10] = 1.0; vbo[11] = 1.0;
			vbo[12] = m_vpLeftActual + (float)rect.right() * m_ppu; vbo[13] = m_vpTopActual + (float)rect.top() * m_ppuY;    vbo[14] = 1.0; vbo[15] = 0.0;
		}
		else {

			vbo[ 0] = m_vpLeftActual + (float)rect.left() * m_ppu;  vbo[ 1] = m_vpTopActual - (float)rect.bottom() * m_ppuY; vbo[ 2] = 0.0; vbo[ 3] = 1.0;
			vbo[ 4] = m_vpLeftActual + (float)rect.left() * m_ppu;  vbo[ 5] = m_vpTopActual - (float)rect.top() * m_ppuY;    vbo[ 6] = 0.0; vbo[ 7] = 0.0;
			vbo[ 8] = m_vpLeftActual + (float)rect.right() * m_ppu; vbo[ 9] = m_vpTopActual - (float)rect.bottom() * m_ppuY; vbo[10] = 1.0; vbo[11] = 1.0;
			vbo[12] = m_vpLeftActual + (float)rect.right() * m_ppu; vbo[13] = m_vpTopActual - (float)rect.top() * m_ppuY;    vbo[14] = 1.0; vbo[15] = 0.0;

		}

		RmglRenderVbo(vbo, m_leftBrandingId, pgmTexture);
	}

}

void RmGlSurface::RmglRenderRightBranding() {

	if (m_rightBrandingId) {

		// Scale the image to fit nicely
		QRect rect = m_rightRect;
		double aspect = ((double)rect.height() / (double)rect.width());
		int width = 256;
		rect.setTopLeft(QPoint(m_width - width - 30, 30));
		rect.setWidth(width);
		rect.setHeight(width * aspect);

		float vbo[16];

		if (m_vpBottomActual > m_vpTopActual) {


			vbo[ 0] = m_vpLeftActual + (float)rect.left() * m_ppu;  vbo[ 1] = m_vpTopActual + (float)rect.bottom() * m_ppuY; vbo[ 2] = 0.0; vbo[ 3] = 1.0;
			vbo[ 4] = m_vpLeftActual + (float)rect.left() * m_ppu;  vbo[ 5] = m_vpTopActual + (float)rect.top() * m_ppuY;    vbo[ 6] = 0.0; vbo[ 7] = 0.0;
			vbo[ 8] = m_vpLeftActual + (float)rect.right() * m_ppu; vbo[ 9] = m_vpTopActual + (float)rect.bottom() * m_ppuY; vbo[10] = 1.0; vbo[11] = 1.0;
			vbo[12] = m_vpLeftActual + (float)rect.right() * m_ppu; vbo[13] = m_vpTopActual + (float)rect.top() * m_ppuY;    vbo[14] = 1.0; vbo[15] = 0.0;


		}
		else {


			vbo[ 0] = m_vpLeftActual + (float)rect.left() * m_ppu;  vbo[ 1] = m_vpTopActual - (float)rect.bottom() * m_ppuY; vbo[ 2] = 0.0; vbo[ 3] = 1.0;
			vbo[ 4] = m_vpLeftActual + (float)rect.left() * m_ppu;  vbo[ 5] = m_vpTopActual - (float)rect.top() * m_ppuY;    vbo[ 6] = 0.0; vbo[ 7] = 0.0;
			vbo[ 8] = m_vpLeftActual + (float)rect.right() * m_ppu; vbo[ 9] = m_vpTopActual - (float)rect.bottom() * m_ppuY; vbo[10] = 1.0; vbo[11] = 1.0;
			vbo[12] = m_vpLeftActual + (float)rect.right() * m_ppu; vbo[13] = m_vpTopActual - (float)rect.top() * m_ppuY;    vbo[14] = 1.0; vbo[15] = 0.0;

		}

		RmglRenderVbo(vbo, m_rightBrandingId, pgmTexture);
	}

}

// ----------------------------------------------------------------------------
// Draw the overlay image (if available)
void RmGlSurface::RmglRenderOverlay()
{
  if (m_overlayId)
  {
    float vbo[16];

    vbo[ 0] = m_vpLeftActual;  vbo[ 1] = m_vpBottomActual; vbo[ 2] = 0.0; vbo[ 3] = 1.0;
    vbo[ 4] = m_vpLeftActual;  vbo[ 5] = m_vpTopActual;    vbo[ 6] = 0.0; vbo[ 7] = 0.0;
    vbo[ 8] = m_vpRightActual; vbo[ 9] = m_vpBottomActual; vbo[10] = 1.0; vbo[11] = 1.0;
    vbo[12] = m_vpRightActual; vbo[13] = m_vpTopActual;    vbo[14] = 1.0; vbo[15] = 0.0;

    QMatrix4x4 saved = m_projection;

    m_projection.setToIdentity();
    m_projection.ortho(m_vpLeftActual, m_vpRightActual, m_vpBottomActual, m_vpTopActual, -1.0, 1.0);

    RmglRenderVbo(vbo, m_overlayId, pgmTexture);

    m_projection = saved;
  }
}

// ----------------------------------------------------------------------------
// Load the given image resoure into the texture
bool RmGlSurface::RmglLoadImageToTexture(QString imgName, unsigned &texId)
{
  QImage img;

  if (img.load(imgName))
  {
    img = img.rgbSwapped();

    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.width(), img.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, img.bits());

    return true;
  }

  return false;
}

// ----------------------------------------------------------------------------
// Render a marker to the screen
void RmGlSurface::RmglRenderMarker(float x, float y, float rot, float scale, unsigned marker, QColor colour, QString text)
{
  Q_UNUSED(text)

  if (marker)
  {
    float vbo[16];
    float xScale = scale * 12.0 * m_ppu;
    float yScale = scale * 12.0 * m_ppuY;

    double t = RmUtil::D2R(rot);

    vbo[ 0] = x + RmUtil::root2 * cos(-3.0 * RmUtil::pi_4 + t) * xScale;
    vbo[ 1] = y + RmUtil::root2 * sin(-3.0 * RmUtil::pi_4 + t) * yScale;

    vbo[ 4] = x + RmUtil::root2 * cos(3.0 * RmUtil::pi_4 + t) * xScale;
    vbo[ 5] = y + RmUtil::root2 * sin(3.0 * RmUtil::pi_4 + t) * yScale;

    vbo[ 8] = x + RmUtil::root2 * cos(-RmUtil::pi_4 + t) * xScale;
    vbo[ 9] = y + RmUtil::root2 * sin(-RmUtil::pi_4 + t) * yScale;

    vbo[12] = x + RmUtil::root2 * cos(RmUtil::pi_4 + t) * xScale;
    vbo[13] = y + RmUtil::root2 * sin(RmUtil::pi_4 + t) * yScale;

    // Texture coordinates
    vbo[ 2] = 0.0; vbo[ 3] = 1.0;
    vbo[ 6] = 0.0; vbo[ 7] = 0.0;
    vbo[10] = 1.0; vbo[11] = 1.0;
    vbo[14] = 1.0; vbo[15] = 0.0;

    m_colour = colour;
    RmglRenderVbo(vbo, marker, pgmAlpha);
  }
}

// ----------------------------------------------------------------------------
// Render a solid line
void RmGlSurface::RmglRenderLine(float x0, float y0, float x1, float y1, QColor colour)
{
  float vbo[16];

  vbo[0] = x0;
  vbo[1] = y0;
  vbo[2] = x1;
  vbo[3] = y1;

  m_colour = colour;
  RmglSetPgm(pgmSolid);

  // Setup vertex
  int aPosition = glGetAttribLocation(m_pgmId, "aPosition");
  glVertexAttribPointer(aPosition, 2, GL_FLOAT, false, 2 * sizeof(float), vbo);
  glEnableVertexAttribArray(aPosition);

  glDrawArrays(GL_LINES, 0, 2);
}

// ----------------------------------------------------------------------------
// Render a solid polyline
void RmGlSurface::RmglRenderPolyline(float* pPts, int nPts, QColor colour)
{
  m_colour = colour;
  RmglSetPgm(pgmSolid);

  // Setup vertex
  int aPosition = glGetAttribLocation(m_pgmId, "aPosition");
  glVertexAttribPointer(aPosition, 2, GL_FLOAT, false, 2 * sizeof(float), pPts);
  glEnableVertexAttribArray(aPosition);

  glDrawArrays(GL_LINES, 0, nPts);
}
