#pragma once

#include <QWidget>
#include <QImage>

// forward definition for the main window
class MainView;

// Enumerate the types of buttons possible on the toolbar
enum eButtonType
{
  pushButton,
  toggleButton,
  radioButton,
  attemptToggle
};

// ----------------------------------------------------------------------------
// NpToolButton - A graphical toolbar button
class NpToolButton : public QObject
{
  Q_OBJECT

public:
  NpToolButton();

  // Methods
  bool   Inside(int x, int y);
  bool   Load(QString normalImg, QString pressedImg, QString text);
  QImage AddTextToImage(QImage image, QString text, QColor textColour);
  void   Draw(QPainter* pPainter);
  void   OnPress();
  void   OnRelease();
  void   OnUncheck();                  // Called by radio button functions

  void   setVisible(bool visible);

  // Button Type
  eButtonType  m_type;

  // Data
  bool    m_isPressed;                 // Is this button in a pressed state
  bool    m_isChecked;                 // Is this button in a checked state
  bool    m_isHighlighted;             // Is this button currentky highlighted
  QRect   m_native;                    // The native size for the button image and text
  QRect   m_rect;                      // The draw area of the button on the toolbar
  QString m_text;                      // The text associated with this button

  // Artwork
  QImage  m_normal;                    // The image for the normal state
  QImage  m_pressed;                   // The image for the pressed state
  QImage  m_highlight;                 // The image for the highlighted state

  bool    m_isVisible;                 // Is the button visible

signals:
  void UpdateToolbar();                // Called when the button needs redraw
  void Pressed();                      // Called when the button has been pressed
  void Released();                     // Called when the button has been released
  void Checked();                      // Called when the button has been checked
  void Unchecked();                    // Called when the button has been unchecked
  bool Attempt();                      // Try to connect
};

// -----------------------------------------------------------------------------
// NpToolSlider - A graphical toolbar slider
class NpToolSlider : public QObject
{
  Q_OBJECT

public:
  NpToolSlider();

  // Methods
  void  Draw(QPainter* pPainter);
  void  OnPress();
  void  OnSlide();
  void  OnRelease();

  bool m_isPressed;                     // Is this slider in a pressed state
  bool m_isSliding;                     // Is this slider in a sliding state

signals:
  void Pressed();
  void Slide();
};

// ----------------------------------------------------------------------------
// NpToolbar - The Navpoint toolbar control

class NpToolbar : public QWidget
{
  Q_OBJECT

public:
  NpToolbar(QWidget* pParent = 0);

  // Overrides
  void paintEvent(QPaintEvent *pEvent)          Q_DECL_OVERRIDE;
  void mouseMoveEvent(QMouseEvent* pEvent)      Q_DECL_OVERRIDE;
  void mousePressEvent(QMouseEvent* pEvent)     Q_DECL_OVERRIDE;
  void mouseReleaseEvent(QMouseEvent* pEvent)   Q_DECL_OVERRIDE;
  void resizeEvent(QResizeEvent* pEvent)        Q_DECL_OVERRIDE;
  void leaveEvent(QEvent *pEvent)               Q_DECL_OVERRIDE;
  bool event(QEvent *pEvent)                    Q_DECL_OVERRIDE;


  // Methods
  void AddToLeft(NpToolButton* pButton, eButtonType type = pushButton);
  void AddToRight(NpToolButton* pButton, eButtonType type = pushButton);


  void ForceResize();


  NpToolButton* HitTest(QPoint point);

  // Data
  MainView*      m_pMainWnd;           // Pointer to the main window

  // Button Lists
  NpToolButton** m_ppLeft;             // Array of buttons to appear on the left of the toolbar
  unsigned       m_nLeft;              // Number of left buttons

  NpToolButton** m_ppRight;            // Array of buttons to appear on the right of the toolbar
  unsigned       m_nRight;             // Number of right buttons

public slots:
  void Redraw();
};

// ----------------------------------------------------------------------------
// The main toolbar for the application

class MainToolbar : public NpToolbar
{
  Q_OBJECT

public:
  MainToolbar(QWidget* pParent = 0);

  void ReadSettings();
  void WriteSettings();

  // Buttons
  NpToolButton m_connect;            // The open files page select button
  NpToolButton m_settings;           // The mission plan page select button
  NpToolButton m_record;             // The dive log page select button
  NpToolButton m_openFile;           // The settings form toggle button
  NpToolButton m_exportLog;          // The log file exporter button
  NpToolButton m_editLog;            // The log file editor button
  NpToolButton m_setup;              // The device setup button

  // KW
  NpToolButton m_snapshot;           // The snapshot image button

  NpToolButton m_upDown;             // The information banner toggle button
  NpToolButton m_leftRight;          // The artemis logo button
  NpToolButton m_palette;            // The palette widget toggle button

public slots:
  void Connect();

  void ShowSettings();
  void HideSettings();
  void ShowPalette();
  void HidePalette();
  void SetToLeft();
  void SetToRight();
  void SetToUp();
  void SetToDown();
  void StartLog();
  void StopLog();

  void OpenFile();
  void CloseFile();

  void Snapshot();

  void Disconnect();
};

