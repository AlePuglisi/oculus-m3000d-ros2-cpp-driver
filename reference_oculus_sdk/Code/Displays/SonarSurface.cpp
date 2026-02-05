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

#include "SonarSurface.h"

#include <QtMath>

#include "../RmUtil/RmUtil.h"

#include <QMouseEvent>


// ============================================================================
// SonarSurface - displays sonar data in a fan display

SonarSurface::SonarSurface()
{
  m_textureId    = 0;           // The image texture id
  m_nBrgs        = 0;
  m_nRngs        = 0;
  m_nBits        = 8;
  m_range        = 0.0;
  m_dwGrid       = true;        // The grid display toggle
  m_dwGridText   = false;
  m_nArcs        = 0;
  m_newImgData   = false;

  m_pImgVbo      = nullptr;     // Vertex buffer object for the image
  m_pGridVbo     = nullptr;     // Vertex buffer objectc for the grid
  m_pBrgs        = nullptr;     // The bearing table
  m_pData        = nullptr;     // Buffer of the last data image

  m_flipX        = false;
  m_flipY        = false;
  m_headDown     = true;

  m_pRgbData     = nullptr;
  m_useRgb       = false;

  m_disconnected = true;

	m_freqChange	 = true;

  m_measuring = false;
  m_showLastMeasurement = false;

  m_measureEnable = false;

  m_wasInsideFan = false;

  m_showGrid = true;
}

SonarSurface::~SonarSurface()
{
  if (m_pImgVbo)
    delete m_pImgVbo;

  if (m_pGridVbo)
    delete m_pGridVbo;

  if (m_pBrgs)
    delete m_pBrgs;

  if (m_pData)
    delete m_pData;

  if (m_pRgbData)
    delete m_pRgbData;
}

// ----------------------------------------------------------------------------
// Render the scene
void SonarSurface::Render()
{
  glClearColor(m_clearColour.redF(), m_clearColour.greenF(), m_clearColour.blueF(), m_clearColour.alphaF());
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // If we have a background then render the image
  if (m_disconnected)
  {
    RenderBackground();
    m_showLastMeasurement = false;
  }
  else
  {
      // Draw the fan image
      if (m_useRgb)
        RenderImgRgb();
      else
        RenderImg();
      // Draw the grid display
      RenderGrid();

	  // Render the branding
	  RenderBranding();
  }

  if ((m_measuring) || ((!m_measuring) && (m_showLastMeasurement)))
  {
      if ((m_measureEndX == -1.0) && (m_measureEndY == -1.0))
          return;

      RenderMeasureLine();
  }

}

// ----------------------------------------------------------------------------
// Initialise the OpenGL environment
void SonarSurface::OnCreate()
{
  RmGlSurface::OnCreate();

  InitTextures();
}

// ----------------------------------------------------------------------------
// Update the project to display the sonar fan
void SonarSurface::OnResize(int w, int h)
{
  m_width = w;
  m_height = h;

  //qDebug() << "Width: " << m_width << ", Height: " << m_height;

  //RmGl2dOrtho::rmgl2dViewport(w, h);
  RmGl2dOrtho::rmgl2dViewport(m_width, m_height);

  if (m_headDown)
    RmGl2dOrtho::rmgl2dProjection(-m_range * 0.9, m_range * 0.9,  m_range * 1.02, -m_range * 0.02, maintainXY);
  else
    RmGl2dOrtho::rmgl2dProjection(-m_range * 0.9, m_range * 0.9, -m_range * 0.02, m_range * 1.02, maintainXY);

  CalcGrid();
}

// ----------------------------------------------------------------------------
// Create a blank sonar image
void SonarSurface::InitTextures()
{
  // Generate a texture for the sonar data
  glGenTextures(1, &m_textureId);

  int nBrgs = 256;
  int nRngs = 256;

  // Put a simple test image into the sonar display
  uchar* pData = (uchar*) malloc (nBrgs * nRngs);
  uchar* pPtr = pData;

  for (int j = 0; j < nRngs; j++)
  {
    for (int i = 0; i < nBrgs; i++)
    {
      *pPtr++ = j;
    }
  }

  UpdateImg(nRngs, nBrgs, pData);

  // Put a simple test image into the sonar display
  m_pRgbData = (uchar*) malloc (nBrgs * nRngs * 3);
  pPtr = m_pRgbData;

  for (int j = 0; j < nRngs; j++)
  {
    for (int i = 0; i < nBrgs; i++)
    {
      *pPtr++ = j;
      *pPtr++ = 0;
      *pPtr++ = 0;
    }
  }

  // Create a somple +/- 64 degree bearing table
  m_pBrgs = (short*) malloc (nBrgs * sizeof(short));
  m_nBrgs = nBrgs;

  for (int b = 0; b < nBrgs; b++)
    m_pBrgs[b] = ((short) (-6400.0 + (12800.0 * (double) b / (double)nBrgs)));

  // Default 10m range
  UpdateFan(10.0, nBrgs, m_pBrgs);

  // Clean up the tempprary data structures
  delete pData;
}

// ----------------------------------------------------------------------------
// Render a background graphic
void SonarSurface::RenderBackground()
{
    const float MAX_WIDTH   = 640.0;
    const float MAX_HEIGHT  = 480.0;

    float ratio = MAX_HEIGHT / MAX_WIDTH;
    float width = m_width / 2;
    float height = width * ratio;

    if (width > MAX_WIDTH)
        width = MAX_WIDTH;

    if (height > MAX_HEIGHT)
        height = MAX_HEIGHT;

    // Draw the background in the centre of the surface
    QRect rect(0 + (m_width / 2) - (width / 2), 0 + (m_height / 2) - (height / 2), width, height);
    RmglRenderBackground(rect);
}

void SonarSurface::RenderBranding()
{
	if (m_showBranding) {
		qDebug() << "Rendering branding";
		RmglRenderLeftBranding();
		RmglRenderRightBranding();
	}
}


void SonarSurface::SetLeftBranding(QString file)
{
	RmglSetLeftBranding(file);
}

void SonarSurface::SetRightBranding(QString file)
{
	RmglSetRightBranding(file);
}

// ----------------------------------------------------------------------------
// Render the sonar image part if the fan display
void SonarSurface::RenderImg()
{
  RmglSetPgm(pgmPalette);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, m_textureId);

  if (m_newImgData)
    AddDataToImg();

  // Setup vertex
  int aPosition = glGetAttribLocation(m_pgmId, "aPosition");
  glVertexAttribPointer(aPosition, 2, GL_FLOAT, false, 4 * sizeof(float), m_pImgVbo);
  glEnableVertexAttribArray(aPosition);

  // Setup texture
  int aTexCoord = glGetAttribLocation(m_pgmId, "aTexCoords");
  glVertexAttribPointer(aTexCoord, 2, GL_FLOAT, false, 4 * sizeof(float), &m_pImgVbo[2]);
  glEnableVertexAttribArray(aTexCoord);

  // Draw Fan
  glDrawArrays(GL_TRIANGLE_STRIP, 0, m_nBrgs * 2);
}

// ----------------------------------------------------------------------------
// Render the image using pre-made RGB data
void SonarSurface::RenderImgRgb()
{
  RmglSetPgm(pgmTexture);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, m_textureId);

  if (m_newImgData)
    AddRgbDataToImg();

  // Setup vertex
  int aPosition = glGetAttribLocation(m_pgmId, "aPosition");
  glVertexAttribPointer(aPosition, 2, GL_FLOAT, false, 4 * sizeof(float), m_pImgVbo);
  glEnableVertexAttribArray(aPosition);

  // Setup texture
  int aTexCoord = glGetAttribLocation(m_pgmId, "aTexCoords");
  glVertexAttribPointer(aTexCoord, 2, GL_FLOAT, false, 4 * sizeof(float), &m_pImgVbo[2]);
  glEnableVertexAttribArray(aTexCoord);

  // Draw Fan
  glDrawArrays(GL_TRIANGLE_STRIP, 0, m_nBrgs * 2);

}

// ----------------------------------------------------------------------------
// Draw the grid lines
void SonarSurface::RenderGrid()
{
  if (!m_pGridVbo)
  {
	//qDebug() << "OsFanGL::RenderGrid, cannot draw grid, no vertex info";
    return;
  }

  glDisable(GL_DEPTH_TEST);

  // Bind the solid colour program
  RmglSetColour(Qt::gray);
  RmglSetPgm(pgmSolid);

	if (m_showGrid) {

	  // Setup vertex
	  int aPosition = glGetAttribLocation(m_pgmId, "aPosition");
	  glVertexAttribPointer(aPosition, 2, GL_FLOAT, false, 2 * sizeof(float), m_pGridVbo);
	  glEnableVertexAttribArray(aPosition);

	  glDrawArrays(GL_LINES, 0, 5 * 2);


	  // Draw the arcs
	  for (int a = 0; a < m_nArcs; a++)
	  {
		glVertexAttribPointer(aPosition, 2, GL_FLOAT, false, 2 * sizeof(float), &m_pGridVbo[10 * 2 + a * m_nBrgs * 2]);
		glEnableVertexAttribArray(aPosition);

		glDrawArrays(GL_LINE_STRIP, 0, m_nBrgs);
	  }
  }

  RmglRenderText(m_gridText);
}

void SonarSurface::RenderMeasureLine()
{
    //qDebug() << "Render line";

    glDisable(GL_DEPTH_TEST);

    // Bind the solid colour program
    RmglSetColour(Qt::red);
    RmglSetPgm(pgmSolid);

    RmglRenderLine(m_measureStartX, m_measureStartY, m_measureEndX, m_measureEndY, QColor(255, 255, 0));

    RmglRenderText(m_measureText);
}

// ----------------------------------------------------------------------------
// Calulate the grid line vertex array based upon the given range and bearing array
void SonarSurface::CalcGrid()
{
  // Make sure we have something sensible
  if (!m_pBrgs || m_nBrgs < 5)
  {
	//qDebug() << "OsFanGL::CalcGrid, Cannot calculate grid, insufficient bearing info.";
    return;
  }

  m_gridText.ClearBuffer();

  // 5 radials left, right, centre and half
  // 5 arcs of nbrgs each
  int nv = 2 * (5 * 2 + 10 * m_nBrgs);

  m_pGridVbo = (float*)realloc (m_pGridVbo, nv * sizeof(float));
  memset(m_pGridVbo, 0, nv * sizeof(float));

  // I'm going to assume symmetry for now
  m_left  = qDegreesToRadians((float)m_pBrgs[0] / 100.0f);
  m_right = qDegreesToRadians((float)m_pBrgs[m_nBrgs - 1] / 100.0f);

  float* pV = m_pGridVbo;

  // Left radial
  *pV++ = 0.0;
  *pV++ = 0.0;
  *pV++ = m_range * sin(m_left);
  *pV++ = m_range * cos(m_left);
  // Left radial  1/2
  *pV++ = 0.0;
  *pV++ = 0.0;
  *pV++ = m_range * sin(m_left / 2.0);
  *pV++ = m_range * cos(m_left / 2.0);
  // Centre radial
  *pV++ = 0.0;
  *pV++ = 0.0;
  *pV++ = 0.0;
  *pV++ = m_range;
  // Right radial 1/2
  *pV++ = 0.0;
  *pV++ = 0.0;
  *pV++ = m_range * sin(m_right / 2.0);
  *pV++ = m_range * cos(m_right / 2.0);
  // Right radial
  *pV++ = 0.0;
  *pV++ = 0.0;
  *pV++ = m_range * sin(m_right);
  *pV++ = m_range * cos(m_right);

  // Get a good marker frequency for the range
  m_mkr = (float) RmUtil::NearestMkr(m_range / 5.0);
  float arc = m_mkr;

  m_gridText.ClearBuffer();

  int a = 0;
  for (a = 0; a < 7; a ++)
  {
    // Force last arc to be at range
    if (arc > m_range)
        arc = m_range;

    for (unsigned v = 0; v < m_nBrgs; v++)
    {
      float t = qDegreesToRadians((float)m_pBrgs[v] / 100.0);

      *pV++ = arc * sin(t);
      *pV++ = arc * cos(t);
    }

    // Add in the text markers
    if (m_dwGridText)
    {
      if (m_headDown)
      {
        RmglAddText(m_gridText, arc * sin(m_left), arc * cos(m_left), 0.0, QString::number(arc, 'g', 3) + "m", taRight | taTop | taFlipY);
        RmglAddText(m_gridText, arc * sin(m_right), arc * cos(m_right), 0.0, QString::number(arc, 'g', 3) + "m", taLeft | taTop | taFlipY);
      }
      else
      {
        RmglAddText(m_gridText, arc * sin(m_left), arc * cos(m_left), 0.0, QString::number(arc, 'g', 3) + "m", taRight | taTop);
        RmglAddText(m_gridText, arc * sin(m_right), arc * cos(m_right), 0.0, QString::number(arc, 'g', 3) + "m", taLeft | taTop);
      }
    }

    // If we've drawn the last range we dont need to continue
    if (arc == m_range)
      break;

    arc += m_mkr;
  }

  // How many arcs to render
  m_nArcs = a + 1;
}

// ----------------------------------------------------------------------------
// Recalculate the image based on the current buffered data
void SonarSurface::Recalculate()
{
  // Do we have an image?
  if (m_pData && m_nBrgs && m_pBrgs && m_nRngs)
  {
    UpdateFan(m_range, m_nBrgs, m_pBrgs, true);
    m_newImgData = true;

    emit Update();
  }
}

// ----------------------------------------------------------------------------
void SonarSurface::AddDataToImg()
{
  if (m_nRngs && m_nBrgs && m_pData)
  {
    glBindTexture(GL_TEXTURE_2D, m_textureId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, m_nBrgs, m_nRngs, 0, GL_ALPHA, GL_UNSIGNED_BYTE, m_pData);

    // Reset the new image data flag
    m_newImgData = false;
  }
}

// ----------------------------------------------------------------------------
// Update the image unsign RGB data
void SonarSurface::AddRgbDataToImg()
{
  if (m_nRngs && m_nBrgs && m_pRgbData)
  {
    glBindTexture(GL_TEXTURE_2D, m_textureId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_nBrgs, m_nRngs, 0, GL_RGB, GL_UNSIGNED_BYTE, m_pRgbData);

    // Reset the new image data flag
    m_newImgData = false;
  }
}


// ----------------------------------------------------------------------------
// Update the fan VBO
void SonarSurface::UpdateFan(double rng, int nBrgs, short* pBrgs, bool updateProjection)
{
  // Update the vertex list
  m_pImgVbo  = (float*) realloc (m_pImgVbo, nBrgs * sizeof(float) * 5 * 2);
  m_nBrgs = nBrgs;

  // If the bearing data is a new (external table) copy into the local buffer
  if (pBrgs != m_pBrgs)
  {
     m_pBrgs = (short*) realloc (m_pBrgs, m_nBrgs * sizeof(short));
     memcpy(m_pBrgs, pBrgs, m_nBrgs * sizeof(short));
  }

  for (int b = 0; b < nBrgs; b++)
  {
    float* pE = &m_pImgVbo[b * 8];

    float brg = qDegreesToRadians((float)pBrgs[b] / 100.0f);

    pE[0] = 0;
    pE[1] = 0;
    pE[2] = (b + 0.5) / (float)nBrgs;
    pE[3] = 0.0;

    pE[4] = rng * sin(brg);
    pE[5] = rng * cos(brg);
    pE[6] = (b + 0.5) / (float)nBrgs;
    pE[7] = 1.0;

    if (m_flipX)
    {
      pE[2] = 1.0 - pE[2];
      pE[6] = 1.0 - pE[6];
    }

    if (m_flipY)
    {
      pE[3] = 1.0 - pE[3];
      pE[7] = 1.0 - pE[7];
    }
  }

  // If the range has changed then we need to adjust the projection and the fans lines
	if (updateProjection || rng != m_range || !m_pGridVbo)
  {
    // Recalculate the projection
    m_range = rng;

    if (m_headDown)
      rmgl2dProjection(-rng * 0.9, rng * 0.9,  rng * 1.02, -rng * 0.02, maintainXY);
    else
      rmgl2dProjection(-rng * 0.9, rng * 0.9, -rng * 0.02, rng * 1.02, maintainXY);

    // Recalculate the gridlines
    CalcGrid();

    // Clear the last measurement shown (if any)
	m_showLastMeasurement = false;
  }
}

// ----------------------------------------------------------------------------
// Update the contents of the image texture based on the incomming data
void SonarSurface::UpdateImg(int nRngs, int nBrgs, uchar* pData)
{
  // If this is noyt already out buffer copy the contents into the data buffer
  if (pData != m_pData)
  {
    m_pData = (uchar*) realloc (m_pData, nRngs * nBrgs);

    m_nRngs = nRngs;
    m_nBrgs = nBrgs;
    m_nBits = 8;

    memcpy(m_pData, pData, nRngs * nBrgs);
  }

  m_newImgData = true;
}

// ----------------------------------------------------------------------------
// Update the contents of the image texture based on the incomming data
void SonarSurface::UpdateImg16(int nRngs, int nBrgs, quint16* pData)
{
  m_pData = (uchar*) realloc (m_pData, nRngs * nBrgs);

  m_nRngs = nRngs;
  m_nBrgs = nBrgs;
  m_nBits = 8;

  quint16* pSrc = pData;
  uchar*   pDest = m_pData;

  for (int i = 0; i < nRngs * nBrgs; i++)
        *pDest++ = (uchar)((*pSrc++ & 0xff00) >> 8);

  m_newImgData = true;
}

bool SonarSurface::AreClockwise(QPoint ct, float radius, float angle, QPoint pt)
{
    float x = (ct.x() + radius) * cos(angle);
    float y = (ct.y() + radius) * sin(angle);

    //qDebug() << "CCX: " << x << ", CCY: " << y;

    return -x * pt.y() + y * pt.x() > 0;
}

bool SonarSurface::IsInsideSector(QPoint pt, QPoint ct, float radius, float angle1, float angle2)
{
    float x;
    float y;

    // Subtract the centre point from the mouse point
    if (m_headDown)
    {
        x = ct.x() - pt.x();
        y = ct.y() - pt.y();

    }
    else {
        x = pt.x() - ct.x();
        y = pt.y() - ct.y();
    }

    if (x < 0)
        x = -x;

    //qDebug() << "X:" << x << ", Y:" << y;

    bool s1 = !AreClockwise(ct, radius, angle1, QPoint(x, y));
    bool s2 = AreClockwise(ct, radius, angle2, QPoint(x, y));
    bool s3 = (x * x + y * y <= radius * radius);

	//qDebug() << (x * x);
	//qDebug() << (y * y);
	//qDebug() << (radius * radius);

	//qDebug() << "S1: " << s1 << ", S2: " << s2 << ", S3: " << s3;

    //return false;

    return s1 && s2 && s3;

    //return !AreClockwise(ct, rd, angle1, QPoint(x,y)) && AreClockwise(ct, rd, angle2, QPoint(x, y)) && (x * x + y * y <= (rd * rd));
}

bool SonarSurface::IsInsideFan(QPoint pos) {
    // Store the fan sector

    // Left and right angles in degrees (subtract 90 as right is 0 degs)
    float leftDegs = qRadiansToDegrees(m_left) - 90;
    float rightDegs = qRadiansToDegrees(m_right) - 90;
    // Left and right angles in radians
    float leftRads = qDegreesToRadians(leftDegs);
    float rightRads = qDegreesToRadians(rightDegs);
    // Radius in metres
    float radiusMetres = m_range;
    // Radius in pixels
    float radiusPixels = (m_height * 0.9625);

    // Fan display centre position
    float ctx, cty;
    XyToScreen(0.0, 0.0, ctx, cty);

    //qDebug() << "Fan Start X: " << ctx << ", Y: " << cty;

    int rx = (pos.x() - ctx);
    int ry = 0;

    if (m_headDown) {
        ry = (cty - pos.y());
    }
    else {
        ry = (pos.y() - cty);
    }

    //qDebug() << "RX: " << rx << ", RY: " << ry;

    bool s1 = !AreClockwise(QPoint(ctx, cty), radiusPixels, leftRads, QPoint(rx, ry));
    bool s2 = AreClockwise(QPoint(ctx, cty), radiusPixels, rightRads, QPoint(rx, ry));
    bool s3 = (rx * rx + ry * ry <= radiusPixels * radiusPixels);

    return (s1 && s2 && s3);
}

// -----------------------------------------------------------------------------
void SonarSurface::mouseMoveEvent(QMouseEvent* pEvent)
{
    // Need to determine whether the current mouse position is inside the fan

    // Capture the current mouse position
    QPoint pos = pEvent->pos();

    // Determine if the cursor is inside the fan display
    bool insideFan = IsInsideFan(pos) && (!m_disconnected);

    // Check to see if the mouse has moved in or out of the fan display
    if (insideFan && !m_wasInsideFan) {
        emit MouseEnter();
    }
    else if (m_wasInsideFan && !insideFan) {
		emit MouseLeave();
    }

    // Store the last state
    m_wasInsideFan = insideFan;


	if ((m_measureEnable) && (IsInsideFan(pos)))
		this->SetCursor(Qt::CrossCursor);
	else
		this->SetCursor(Qt::ArrowCursor);

	if ((m_measureEnable) && ((m_measuring) && (IsInsideFan(pos))))
    {

        //QPoint pos = pEvent->pos();
        float x, y;

        // Get the X,Y of the mouse coordinates
        ScreenToXy(pos.x(), pos.y(), x, y);

        float sx = m_measureStartX;
        float sy = m_measureStartY;
        float dx = x - sx;
        float dy = y - sy;

        float dist = sqrt((dx * dx) + (dy * dy));

        if (dist > m_range)
            return;

        //qDebug() << "Distance: " + QString::number(dist);

        float rads = atan2((y - sy), (x - sx));
        float angle = (rads * (180 / 3.141592654)) - 90;

        if (angle < -180)
            angle += 360;
        else if (angle > 180)
            angle -= 360;


        //qDebug() << "Angle: " + QString::number(angle);

        m_measureEndX = x;
        m_measureEndY = y;

		m_measureText.ClearBuffer();
		if (m_headDown)
		{
			RmglAddText(m_measureText, m_measureEndX, m_measureEndY, 0.0, "Dist: " + QString::number(dist, 3, 2) + "m, Angle: " + QString::number(angle, 3, 1) + ", X: " + QString::number(x, 3, 2) + ", Y: " + QString::number(y, 3, 2), taFlipY);
		}
		else
		{
			RmglAddText(m_measureText, m_measureEndX, m_measureEndY, 0.0, "Dist: " + QString::number(dist, 3, 2) + "m, Angle: " + QString::number(angle, 3, 1) + ", X: " + QString::number(x, 3, 2) + ", Y: " + QString::number(y, 3, 2));
		}

        emit Update();

    }

    // The mouse is inside the fan display
    if (insideFan && m_wasInsideFan) {

        float x, y;

        // Get the X,Y of the mouse coordinates
        ScreenToXy(pos.x(), pos.y(), x, y);

        // Work out the distance
        float dist = sqrt((x * x) + (y * y));

        float rads = atan2((x), (y));
        float angle = (rads * (180 / 3.141592654));// - 90;

        if (angle < -180)
            angle += 360;
        else if (angle > 180)
            angle -= 360;


        emit MouseInfo(dist, angle, x, y);

    }


}

// -----------------------------------------------------------------------------
void SonarSurface::mousePressEvent(QMouseEvent* pEvent)
{
	qDebug() << "Measure enable" << m_measureEnable;

    // Capture the measure point
    if ((pEvent->button() == Qt::LeftButton) && (m_measureEnable))
    {
        QPoint pos = pEvent->pos();

        if (IsInsideFan(pos)) {

            ScreenToXy(pos.x(), pos.y(), m_measureStartX, m_measureStartY);

            m_measureEndX = -1.0;
            m_measureEndY = -1.0;

			m_showLastMeasurement = false;

            m_measuring = true;
        }
    }
}

// -----------------------------------------------------------------------------
void SonarSurface::mouseReleaseEvent(QMouseEvent* pEvent)
{

    // Mouse release
    if (pEvent->button() == Qt::LeftButton)
    {
		m_measureStartX = -1.0;
		m_measureEndX = -1.0;
        m_measuring = false;
		m_measureEndX = -1.0;
		m_measureEndY = -1.0;

        m_showLastMeasurement = true;

        emit Update();
    }
}

void SonarSurface::ToggleGridLines() {
	m_showGrid = !m_showGrid;
}
