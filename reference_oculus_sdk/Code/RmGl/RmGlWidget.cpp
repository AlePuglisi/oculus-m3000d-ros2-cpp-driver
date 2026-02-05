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

#include "rmglwidget.h"
#include "RmGlSurface.h"

#include <QApplication>
#include <QPaintEvent>

class SonarSurface;


RmGlWidget::RmGlWidget(QWidget* pParent)
  : QOpenGLWidget(pParent),
	m_version(this)
{
  m_pSurface = nullptr;

  m_version.setObjectName("version");
  m_version.setText("Version: " + QApplication::applicationVersion());

  setMouseTracking(true);

}

RmGlWidget::~RmGlWidget()
{
  makeCurrent();

  if (m_pSurface)
    delete m_pSurface;

	doneCurrent();

  m_pSurface = nullptr;
}

// -----------------------------------------------------------------------------
void RmGlWidget::paintEvent(QPaintEvent *event)
{
    QOpenGLWidget::paintEvent(event);

    QPainter painter;

    painter.begin(this);

    painter.end();

}

// Overrides
void RmGlWidget::paintGL()
{
  if (m_pSurface)
  {
    // Update the clear colour
    m_pSurface->m_clearColour = this->palette().background().color();
    m_pSurface->Render();

  }
}

void RmGlWidget::initializeGL()
{
  if (m_pSurface) {
    m_pSurface->OnCreate();

    m_pSurface->m_clearColour = this->palette().background().color();
  }
}

void RmGlWidget::resizeGL(int w, int h)
{
  m_version.setGeometry(20, h - 20, w, 20);

  if (m_pSurface)
    m_pSurface->OnResize(w, h);
}

void RmGlWidget::mouseMoveEvent(QMouseEvent* pEvent)
{
  if (m_pSurface)
    m_pSurface->mouseMoveEvent(pEvent);
}

void RmGlWidget::mousePressEvent(QMouseEvent* pEvent)
{
  if (m_pSurface)
    m_pSurface->mousePressEvent(pEvent);
}

void RmGlWidget::mouseReleaseEvent(QMouseEvent* pEvent)
{
  if (m_pSurface)
    m_pSurface->mouseReleaseEvent(pEvent);
}

void RmGlWidget::mouseDoubleClickEvent(QMouseEvent* pEvent)
{
  if (m_pSurface)
    m_pSurface->mouseDoubleClickEvent(pEvent);
}

void RmGlWidget::wheelEvent(QWheelEvent *pEvent)
{
  if (m_pSurface)
    m_pSurface->wheelEvent(pEvent);
}

void RmGlWidget::RmglSetupSurface(RmGlSurface* pSurface)
{
  m_pSurface = pSurface;

  if (m_pSurface)
  {
    connect(m_pSurface, &RmGlSurface::Update, this, &RmGlWidget::OnUpdate);
    connect(m_pSurface, &RmGlSurface::SetCursor, this, &RmGlWidget::SetCursor);
  }
}

void RmGlWidget::OnUpdate()
{
  update();
}

void RmGlWidget::SetCursor(const QCursor &cursor)
{
  setCursor(cursor);
}

void RmGlWidget::showBranding(bool show) {
	if (m_pSurface) {
		qDebug() << "Show branding: " << show;
		m_pSurface->RmglShowBranding(show);
		update();
	}
}

