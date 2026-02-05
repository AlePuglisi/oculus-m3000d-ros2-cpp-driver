#include <QPainter>
#include <QDebug>
#include <QDesktopServices>
#include <QUrl>
#include <QMouseEvent>
#include <QToolTip>
#include <QFileDialog>
#include <QSettings>
#include <QGuiApplication>
#include <QScreen>
#include <QPixmap>
#include <QGraphicsDropShadowEffect>

#include "../Oculus/OsClientCtrl.h"
#include "../Displays/SonarSurface.h"
#include "../RmUtil/RmPlayer.h"

#include "OsToolbar.h"
#include "MainView.h"

#include "KwAviFile.h"

// ============================================================================
// NpToolButton - A graphical toolbar button

NpToolButton::NpToolButton()
{
  // Default button state
  m_isPressed     = false;
  m_isHighlighted = false;
  m_isChecked     = false;
  m_type          = pushButton;
  m_isVisible     = true;
}

// ----------------------------------------------------------------------------
// Is the given coordinate within the button
bool NpToolButton::Inside(int x, int y)
{
  if (m_rect.contains(x, y))
    return true;
  else
    return false;
}

// ----------------------------------------------------------------------------
// Load the resources for the button
bool NpToolButton::Load(QString normalImg, QString pressedImg, QString text)
{
  m_normal  = QImage(normalImg);
  m_pressed = QImage(pressedImg);

  // Greyscale the highlighted - we need to get back to full colour again for the blue corners
  //m_highlight = m_normal.convertToFormat(QImage::Format_Grayscale8);
  //m_highlight = m_highlight.convertToFormat(m_normal.format());

  // Setup the size of the native image
 // m_native.setWidth(m_normal.width());
  //m_native.setHeight(m_normal.height());
  m_native.setWidth(64);
  m_native.setHeight(64);

  m_text = text;

  return true;
}

// ----------------------------------------------------------------------------
// Extend an image and add in the given text
QImage NpToolButton::AddTextToImage(QImage image, QString text, QColor textColour)
{
  // Setup the font based on the size of the image
  QFont font;
  font.setFamily("Calibri");
  font.setPixelSize(image.height() / 2);

  // Get the width of the string
  QFontMetrics fm(font);
  int width  = std::min(4094, fm.width(text));

  // Create a return image with room for the text
  QImage returnImage = QImage(image.width() + 14 + width, image.height(), image.format());

  QRect r = returnImage.rect();

  // Construct the new image
  QPainter painter;
  painter.begin(&returnImage);

  // Clean the back gorund using the corner colour of the given image
  painter.fillRect(r, image.pixel(1, 1));

  // Add in the given image to the left
  painter.drawImage(0, 0, image);

  // Adjust the rect not to linclude the image
  r.setLeft(image.width() + 4);

  // Paint in the text
  painter.setFont(font);

  QTextOption o;
  o.setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

  painter.end();

  return returnImage;
}


// ----------------------------------------------------------------------------
// Draw the button based upon its current position and state
void NpToolButton::Draw(QPainter* pPainter)
{
 /* // Enforce the images to be 64x64px
  QRect rect = m_rect;
  rect.setWidth(64);
  rect.setHeight(64);
*/
  pPainter->setRenderHint(QPainter::Antialiasing);
  pPainter->setRenderHint(QPainter::SmoothPixmapTransform);

  if (m_isHighlighted)
  {
    QRect r;
    //r = m_rect.adjusted(-2, -2, 2, 2);
    r = m_rect.adjusted(-8, -8, 8, 8);

    QPen pen = pPainter->pen();
    //pPainter->fillRect(r, QColor(30, 144, 255));
    pen.setColor(QColor(30, 144, 255));
    pen.setWidth(2);
    pPainter->setPen(pen);
    pPainter->setBrush(QColor(30, 144, 255));
    pPainter->drawRoundedRect(r, 5, 5);
  }

  if (m_isPressed || m_isChecked)
    pPainter->drawImage(m_rect, m_pressed);
  else
    pPainter->drawImage(m_rect, m_normal);

  /*
  if (m_isHighlighted)
  {
    QRect r;
    r = m_rect.adjusted(-2, -2, 2, 2);
    pPainter->drawRoundedRect(r, 5, 5);
  }*/
}

// ----------------------------------------------------------------------------
// The mouse has been pressed inside the button
void NpToolButton::OnPress()
{
  m_isPressed = true;

  switch (m_type)
  {
    case pushButton:
      break;

    case toggleButton:
      m_isChecked = !m_isChecked;

      qDebug() << "Toggled:" + QString::number(m_isChecked);

      if (m_isChecked)
        emit Checked();
      else
        emit Unchecked();
      break;

    case radioButton:
      if (!m_isChecked)
      {
        m_isChecked = true;
        emit Checked();
      }
      break;

    case attemptToggle:
      break;

  };

  emit Pressed();
}

// ----------------------------------------------------------------------------
// The mouse has been released inside the buttob
void NpToolButton::OnRelease()
{
  // Reset the unpress flag
  m_isPressed = false;

  emit Released();
}

// ----------------------------------------------------------------------------
//
void NpToolButton::OnUncheck()
{
  m_isChecked = false;
  emit Unchecked();
}


// ----------------------------------------------------------------------------
// Sets the visibility of the tool button
void NpToolButton::setVisible(bool visible) {

    if (visible == m_isVisible) return;

    m_isVisible = visible;
}


// ============================================================================
// NpToolbar - The toolbar control for the application

NpToolbar::NpToolbar(QWidget* pParent) :
  QWidget(pParent)
{
  // Attach the parent as a typed main window
  m_pMainWnd = (MainView*) pParent;

  // Initialise the button lists
  m_ppLeft  = nullptr;
  m_nLeft   = 0;
  m_ppRight = nullptr;
  m_nRight  = 0;

  // Get access to mouse tracking information
  setMouseTracking(true);
}

// ----------------------------------------------------------------------------
// Draw the toolbar artwork and the buttons
void NpToolbar::ForceResize()
{
    // Get the toolbar size
    QRect r = geometry();

    int x_space = 30;
    int y_space = 20;
    int x     = x_space;
    int st    = 1;
    int sb    = 1;

    // Configure the left hand button
    for (int left = 0; left < (int)m_nLeft; left++)
    {
        if (!m_ppLeft[left]->m_isVisible) continue;

      m_ppLeft[left]->m_rect.setTop(y_space);
      m_ppLeft[left]->m_rect.setBottom(y_space + m_ppLeft[left]->m_native.height() * st / sb);
      m_ppLeft[left]->m_rect.setLeft(x);
      x += m_ppLeft[left]->m_native.width() * st / sb;
      m_ppLeft[left]->m_rect.setRight(x);
      x += x_space;
    }

    x = r.right() - x_space;// * 2;

    // Configure the right hand buttons (right to left)
    for (int right = 0; right < (int)m_nRight; right++)
    {
        if (!m_ppRight[right]->m_isVisible) continue;

      m_ppRight[right]->m_rect.setTop(y_space);
      m_ppRight[right]->m_rect.setBottom(y_space + m_ppRight[right]->m_native.height() * st / sb);
      m_ppRight[right]->m_rect.setRight(x);
      x -= m_ppRight[right]->m_native.width() * st / sb;
      m_ppRight[right]->m_rect.setLeft(x);
      x -= x_space;
    }
}

// ----------------------------------------------------------------------------
// Draw the toolbar artwork and the buttons
void NpToolbar::paintEvent(QPaintEvent* pEvent)
{
  Q_UNUSED(pEvent);

  QPainter painter;
  painter.begin(this);


  // KW
  // Fill the background of the toolbar in Blueprint blue
  //painter.fillRect(pEvent->rect(), QColor(66, 66, 66));
  // END KW

  // Draw left buttons
  for (unsigned left = 0; left < m_nLeft; left++) {
    if (m_ppLeft[left]->m_isVisible) {
      m_ppLeft[left]->Draw(&painter);
    }
  }

  // Draw right buttons
  for (unsigned right = 0; right < m_nRight; right++) {
    if (m_ppRight[right]->m_isVisible) {
      m_ppRight[right]->Draw(&painter);
    }
  }

  painter.end();
}

// ----------------------------------------------------------------------------
// Respond to mouse movement events within the toolbar
void NpToolbar::mouseMoveEvent(QMouseEvent* pEvent)
{
  bool modified = false;

  // Test the left hand buttons
  for (int left = 0; left < (int)m_nLeft; left++)
  {
    if (m_ppLeft[left]->m_rect.contains(pEvent->pos()))
    {
      if (!m_ppLeft[left]->m_isHighlighted)
      {
        m_ppLeft[left]->m_isHighlighted = true;
        modified = true;
      }
    }
    else
    if (m_ppLeft[left]->m_isHighlighted)
    {
      m_ppLeft[left]->m_isHighlighted = false;
      modified = true;
    }
  }

  // Test the right hand buttons
  for (int right = 0; right < (int)m_nRight; right++)
  {
    if (m_ppRight[right]->m_rect.contains(pEvent->pos()))
    {
      if (!m_ppRight[right]->m_isHighlighted)
      {
        m_ppRight[right]->m_isHighlighted = true;
        modified = true;
      }
    }
    else
    if (m_ppRight[right]->m_isHighlighted)
    {
      m_ppRight[right]->m_isHighlighted = false;
      modified = true;
    }
  }

  if (modified)
    update();
}


// ----------------------------------------------------------------------------
// Respond to mouse click events within the toolbar
void NpToolbar::mousePressEvent(QMouseEvent* pEvent)
{
  // Test the left hand buttons
  for (int left = 0; left < (int)m_nLeft; left++)
  {
    if (m_ppLeft[left]->m_rect.contains(pEvent->pos()))
    {
      m_ppLeft[left]->OnPress();

      // Uncheck any other radio buttons in the group
      if (m_ppLeft[left]->m_type == radioButton)
      {
        for (int radio = 0; radio < (int)m_nLeft; radio++)
        {
          if (m_ppLeft[radio] != m_ppLeft[left] && m_ppLeft[radio]->m_type == radioButton)
            m_ppLeft[radio]->OnUncheck();
        }
      }
    }
  }

  // Test the right hand buttons
  for (int right = 0; right < (int)m_nRight; right++)
  {
    if (m_ppRight[right]->m_rect.contains(pEvent->pos()))
    {
      m_ppRight[right]->OnPress();

      // Uncheck any other radio buttons in the group
      if (m_ppRight[right]->m_type == radioButton)
      {
        for (int radio = 0; radio < (int)m_nRight; radio++)
        {
          if (m_ppRight[radio] != m_ppRight[right] && m_ppRight[radio]->m_type == radioButton)
            m_ppRight[radio]->OnUncheck();
        }
      }
    }
  }

  update();
}

// ----------------------------------------------------------------------------
// Respond to mouse release events within the toolbar
void NpToolbar::mouseReleaseEvent(QMouseEvent* pEvent)
{
  // Test the left hand buttons
  for (int left = 0; left < (int)m_nLeft; left++)
  {
    if (m_ppLeft[left]->m_rect.contains(pEvent->pos()))
    {
      if (m_ppLeft[left]->m_isPressed)
        m_ppLeft[left]->OnRelease();
    }
  }

  // Test the right hand buttons
  for (int right = 0; right < (int)m_nRight; right++)
  {
    if (m_ppRight[right]->m_rect.contains(pEvent->pos()))
    {
      if (m_ppRight[right]->m_isPressed)
        m_ppRight[right]->OnRelease();
    }
  }

  update();
}

// ----------------------------------------------------------------------------
// Position the buttons based on the new size
void NpToolbar::resizeEvent(QResizeEvent* pEvent)
{
  Q_UNUSED(pEvent);

  ForceResize();
}

// ----------------------------------------------------------------------------
// Make sure that none of the buttons are in track
void NpToolbar::leaveEvent(QEvent* pEvent)
{
  Q_UNUSED(pEvent);

  bool modified = false;

  // Configure the left hand button
  for (int left = 0; left < (int)m_nLeft; left++)
  {
    if (m_ppLeft[left]->m_isHighlighted)
    {
      m_ppLeft[left]->m_isHighlighted = false;
      modified = true;
    }
  }

  // Configure the right hand buttons (right to left)
  for (int right = 0; right < (int)m_nRight; right++)
  {
    if (m_ppRight[right]->m_isHighlighted)
    {
      m_ppRight[right]->m_isHighlighted = false;
      modified = true;
    }
  }

  // If we have changed the toolbar state then update the toolbar
  if (modified)
    update();
}

// ----------------------------------------------------------------------------
// Display the correct tooltip
bool NpToolbar::event(QEvent* pEvent)
{
  if (pEvent->type() == QEvent::ToolTip)
  {
    QHelpEvent *helpEvent = static_cast<QHelpEvent *>(pEvent);


    NpToolButton* pButton = HitTest(helpEvent->pos());

    if (pButton)
    {
      QToolTip::showText(helpEvent->globalPos(), pButton->m_text);
    }
    else
    {
      QToolTip::hideText();
      pEvent->ignore();
    }

    return true;
  }

  return QWidget::event(pEvent);
}

// ----------------------------------------------------------------------------
// Add button to left of the toolbar
void NpToolbar::AddToLeft(NpToolButton* pButton, eButtonType type)
{
  m_nLeft++;
  m_ppLeft = (NpToolButton**) realloc (m_ppLeft, m_nLeft * sizeof (NpToolButton*));
  m_ppLeft[m_nLeft - 1] = pButton;

  pButton->m_type = type;

  connect(pButton, &NpToolButton::UpdateToolbar, this, &NpToolbar::Redraw);
}

// ----------------------------------------------------------------------------
// Add button to the right of the toolbar
void NpToolbar::AddToRight(NpToolButton* pButton, eButtonType type)
{
  m_nRight++;
  m_ppRight = (NpToolButton**) realloc (m_ppRight, m_nRight * sizeof (NpToolButton*));
  m_ppRight[m_nRight - 1] = pButton;

  pButton->m_type = type;

  connect(pButton, &NpToolButton::UpdateToolbar, this, &NpToolbar::Redraw);
}

// ----------------------------------------------------------------------------
// Test the point for being within one of the buttons
NpToolButton*NpToolbar::HitTest(QPoint point)
{
  // Test the left hand buttons
  for (int left = 0; left < (int)m_nLeft; left++)
  {
    if (m_ppLeft[left]->m_rect.contains(point))
    {
      return m_ppLeft[left];
    }
  }

  // Test the right hand buttons
  for (int right = 0; right < (int)m_nRight; right++)
  {
    if (m_ppRight[right]->m_rect.contains(point))
    {
      return m_ppRight[right];
    }
  }

  return nullptr;
}

// ----------------------------------------------------------------------------
// (SLOT) A simple slot to call the update function
void NpToolbar::Redraw()
{
  update();
}



// ============================================================================
// The main toolbar for the application

MainToolbar::MainToolbar(QWidget* pParent) :
  NpToolbar(pParent)
{
  m_pMainWnd = nullptr;

  // Add the left hand buttons (drawn left to right)
  AddToLeft(&m_connect,     toggleButton);
  AddToLeft(&m_openFile,    toggleButton);
  AddToLeft(&m_editLog,     toggleButton);
  AddToLeft(&m_exportLog,   toggleButton);
  AddToLeft(&m_setup,       toggleButton);
  //AddToLeft(&m_record,   toggleButton);
  //AddToLeft(&m_snapshot, pushButton);

  // Add the right hand buttons (drawn right to left)
  AddToRight(&m_settings,   toggleButton);
  //AddToRight(&m_upDown,    toggleButton);
  //AddToRight(&m_leftRight, toggleButton);
  //AddToRight(&m_palette,    toggleButton);

  // Load the button resources
  m_connect.    Load(":/Media/Mode_Live_96.png",            ":/Media/Mode_Live_96.png",             tr("Connect to Sonar"));
  //m_connect.  Load(":/Media/ConnectOff.png",              ":/Media/ConnectOn.png",                tr("Connect to Sonar"));
  m_openFile.   Load(":/Media/Mode_Review_96.png",          ":/Media/Mode_Review_96.png",           tr("Open File"));
  m_setup.      Load(":/Media/Mode_Settings_Device_96",     ":/Media/Mode_Settings_Device_96",      tr("Sonar Setup"));
  m_editLog.    Load(":/Media/Review_Func_Edit_96.png",     ":/Media/Review_Func_Edit_96.png",      tr("Edit Log File"));
  m_exportLog.  Load(":/Media/Review_Func_Export_96.png",   ":/Media/Review_Func_Export_96.png",    tr("Edit Log File"));
  m_palette.    Load(":/Media/Palette.png",                 ":/Media/PaletteOff.png",               tr("Toggle Palette"));
  m_settings.   Load(":/Media/Mode_Settings_App_96.png",    ":/Media/Mode_Settings_App_96.png",     tr("Toggle Settings"));

  connect(&m_connect, &NpToolButton::Checked, this, &MainToolbar::Connect);
  connect(&m_connect, &NpToolButton::Unchecked, this, &MainToolbar::Disconnect);

  connect(&m_settings, &NpToolButton::Checked,   this, &MainToolbar::ShowSettings);
  connect(&m_settings, &NpToolButton::Unchecked, this, &MainToolbar::HideSettings);

  connect(&m_record, &NpToolButton::Checked,   this, &MainToolbar::StartLog);
  connect(&m_record, &NpToolButton::Unchecked, this, &MainToolbar::StopLog);

  connect(&m_openFile, &NpToolButton::Checked,   this, &MainToolbar::OpenFile);
  connect(&m_openFile, &NpToolButton::Unchecked, this, &MainToolbar::CloseFile);

  m_editLog.setVisible(false);
  m_exportLog.setVisible(false);
}

// ----------------------------------------------------------------------------
void MainToolbar::ReadSettings()
{
  QSettings settings;

  m_upDown.m_isChecked     = settings.value("UpDownState", 0).toBool();
  m_leftRight.m_isChecked  = settings.value("LeftRightState", 0).toBool();
  Redraw();
}

// ----------------------------------------------------------------------------
void MainToolbar::WriteSettings()
{
  QSettings settings;

  settings.setValue("UpDownState", m_upDown.m_isChecked);
  settings.setValue("LeftRightState", m_leftRight.m_isChecked);
}


// ----------------------------------------------------------------------------
void MainToolbar::ShowSettings()
{
  m_pMainWnd->m_settings.setVisible(true);
}

// ----------------------------------------------------------------------------
void MainToolbar::HideSettings()
{
  m_pMainWnd->m_settings.setVisible(false);
}

// ----------------------------------------------------------------------------
void MainToolbar::ShowPalette()
{
  m_pMainWnd->m_palSelect.setVisible(true);
}

// ----------------------------------------------------------------------------
void MainToolbar::HidePalette()
{
  m_pMainWnd->m_palSelect.setVisible(false);
}

// ----------------------------------------------------------------------------
// Set the sonar image to work  right to left
void MainToolbar::SetToLeft()
{
  SonarSurface* pSonar = (SonarSurface*) m_pMainWnd->m_fanDisplay.RmglGetSurface();

  pSonar->m_flipX = true;
  pSonar->Recalculate();
}

// ----------------------------------------------------------------------------
// Set the sonar image to work left to right
void MainToolbar::SetToRight()
{
  SonarSurface* pSonar = (SonarSurface*) m_pMainWnd->m_fanDisplay.RmglGetSurface();

  pSonar->m_flipX = false;
  pSonar->Recalculate();
}

// ----------------------------------------------------------------------------
// Set the sonar image to point up the screen
void MainToolbar::SetToUp()
{
  SonarSurface* pSonar = (SonarSurface*) m_pMainWnd->m_fanDisplay.RmglGetSurface();

  pSonar->m_headDown = false;
  pSonar->Recalculate();
}

// ----------------------------------------------------------------------------
// Set the sonar image to point down the screen
void MainToolbar::SetToDown()
{
  SonarSurface* pSonar = (SonarSurface*) m_pMainWnd->m_fanDisplay.RmglGetSurface();

  pSonar->m_headDown = true;
  pSonar->Recalculate();
}

// ----------------------------------------------------------------------------
// Try to connect to the sonar
void MainToolbar::Connect()
{/*
  // Count the number of sonar's in the list
  int count = m_pMainWnd->m_sonarList.count();

  qDebug() << count;

  // Check to see if more than one sonar has been detected
  if (count > 1) {
    // Show the connect form
    int result = m_pMainWnd->m_conn.exec();

    qDebug() << result;

    if (result == 1) {
        // Get the sonar from the list
    }
  }
*/
  m_pMainWnd->m_connectForm.exec();

  // Show the connection list
  //m_pMainWnd->m_connect.setVisible(true);


  if (m_pMainWnd->m_ipFromStatus.length() > 0)
  {
    if (!m_pMainWnd->m_oculusClient.IsOpen())
    {
      m_pMainWnd->m_oculusClient.m_hostname = m_pMainWnd->m_ipFromStatus;
      m_pMainWnd->m_oculusClient.Connect();
      m_pMainWnd->SetDisplayMode(online);
      m_pMainWnd->StopReplay();
    }
  }
}

// ----------------------------------------------------------------------------
// Disconnect from the sonar
void MainToolbar::Disconnect()
{
  if (m_pMainWnd->m_oculusClient.IsOpen())
    m_pMainWnd->m_oculusClient.Disconnect();

  m_pMainWnd->SetDisplayMode(offline);
}

// ----------------------------------------------------------------------------
// (SLOT) Start up the logger
void MainToolbar::StartLog()
{
  m_pMainWnd->m_logger.OpenLog();

  if (m_pMainWnd->m_logger.m_state == logging)
    m_pMainWnd->m_info.setText("Logging To: " + m_pMainWnd->m_logger.m_fileName);
  else
  {
    m_pMainWnd->m_info.setText("Logging Failed! Cannot Open: " + m_pMainWnd->m_logger.m_fileName);
    m_record.m_isChecked = false;
    update();
  }
}

// ----------------------------------------------------------------------------
// (SLOT) The connection to the sonar failed ...
void MainToolbar::StopLog()
{
  m_pMainWnd->m_logger.CloseLog();

  if (m_pMainWnd->m_logger.m_state == notLogging)
    m_pMainWnd->m_info.setText("");
}

// ----------------------------------------------------------------------------
void MainToolbar::OpenFile()
{
  RmPlayer player;

  QFileDialog fd;

  QString logFile = fd.getOpenFileName(this, tr("Select log file to open"), m_pMainWnd->m_logger.m_logDir, "*.log");

  if (logFile.length() > 0)
  {
    m_pMainWnd->m_nEntries = player.CreateTypeIndex(logFile, rt_oculusSonar, &m_pMainWnd->m_pEntries);

    if (m_pMainWnd->m_nEntries == 0)
    {
      m_pMainWnd->m_info.setText(tr("No oculus entries in file, trying sonar header"));
      m_pMainWnd->m_nEntries = player.CreateTypeIndex(logFile, rt_apSonarHeader, &m_pMainWnd->m_pEntries);
      m_pMainWnd->m_useRawSonar = true;
    }
    else
      m_pMainWnd->m_useRawSonar = false;

    if (m_pMainWnd->m_nEntries > 0)
    {
      m_pMainWnd->m_info.setText(QString::number(m_pMainWnd->m_nEntries) + tr(" Oculus entries identified in file."));
      m_pMainWnd->m_replayFile = logFile;
      m_pMainWnd->SetDisplayMode(review);

      if (m_pMainWnd->m_oculusClient.IsOpen())
      {
        m_pMainWnd->m_oculusClient.Disconnect();
        m_connect.m_isChecked = false;
        update();
      }
    }
  }
  else
  {
    m_openFile.m_isChecked = false;
    update();
  }
}

// ----------------------------------------------------------------------------
void MainToolbar::CloseFile()
{

}


// ----------------------------------------------------------------------------
// Takes a snapshot of the current window state and saves it as a PNG
void MainToolbar::Snapshot()
{
    // Get the primary screen
    QScreen* screen = QGuiApplication::primaryScreen();
    QPixmap pixmap = screen->grabWindow(0);

    // Grab the framebuffer
    //QImage snapshot = m_pMainWnd->m_fanDisplay.grabFramebuffer();

    // Create the output filename
    QDateTime dt = QDateTime::currentDateTime();
    QString filename = m_pMainWnd->m_logger.m_logDir + QDir::separator() + QString(m_pMainWnd->m_logger.s_source) + dt.toString("_yyyyMMdd_hhmmss.png");

    // Save the snapshot
    //snapshot.save(filename, "PNG");

    pixmap.save(filename, "PNG");
}
