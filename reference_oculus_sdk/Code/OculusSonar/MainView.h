#pragma once

#include <QMainWindow>
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <QTimer>

#include "ModeCtrls.h"
#include "OptionsCtrls.h"
#include "OnlineCtrls.h"
#include "ReviewCtrls.h"
#include "Settings.h"
#include "ToolsCtrls.h"
#include "InfoCtrls.h"
#include "CursorCtrls.h"

#include "TitleCtrls.h"

#include "SettingsForm.h"
#include "ConnectForm.h"
#include "DeviceForm.h"
#include "InfoForm.h"
#include "HelpForm.h"

#include "../RmGl/RmGlWidget.h"
#include "../RmGl/PalWidget.h"
#include "../Oculus/OsStatusRx.h"
#include "../Oculus/OsClientCtrl.h"
#include "../Oculus/OssDataWrapper.h"
#include "../RmUtil/RmLogger.h"
#include "../RmUtil/RmPlayer.h"


// forward definition for the sonar surface
class SonarSurface;

// Enumerate different display modes
enum eDisplayMode : int
{
  offline,
  online,
  review
};


#pragma pack(push, 1)
// ----------------------------------------------------------------------------
// Sonar data header with bearing table
class ApSonarDataHeader
{
public:
  float   range;        // range in m
  float   gain;         // The current gain value
  float   frequency;    // The frequency
  quint16 nRngs;        // number of range lines
  quint16 nBrgs;        // number of bearing lines
  qint16  pBrgs[1024];  // The bearing table (max 1024)
  quint16 size;         //
};
#pragma pack(pop)

// ----------------------------------------------------------------------------
// Structure holding information about various types of sonar supported by
// this software. Things like maximum range, etc can be used to adjust the
// various on-screen controls
struct SonarType {
	OculusPartNumberType partNumber;
	bool highFreq;
	double highFreqRange;
	bool lowFreq;
	double lowFreqRange;
};


// ----------------------------------------------------------------------------
// MainView - this is the main oculus sonar window

class MainView : public QMainWindow
{
  Q_OBJECT

public:

	static const uint32_t NAVIGATION_RANGE_COUNT = 16;
	static const uint32_t INSPECTION_RANGE_COUNT = 11;

	static double NAVIGATION_RANGES[NAVIGATION_RANGE_COUNT];
	static double INSPECTION_RANGES[INSPECTION_RANGE_COUNT];

public:

  explicit MainView(QWidget *parent = 0);
  ~MainView();

  // Overrides
  void resizeEvent(QResizeEvent* event)  Q_DECL_OVERRIDE;
  void keyPressEvent(QKeyEvent* event) Q_DECL_OVERRIDE;

  // Methods
  void LayoutCtrls();
  void SetDisplayMode(eDisplayMode displayMode);
  void ReadSettings();
  void WriteSettings();

  bool Snapshot();

  void FlipX(bool flip);
  void FlipY(bool flip);


  void StartLog();
  void StopLog();

  void SetTheme(QString theme);

  void SetMeasureMode(bool enable);

  void RebuildSonarList();

	void SetSonarInfo(OculusStatusMsg msg);

  void MouseLeaveFan();
  void MouseEnterFan();

  void SetTitleLogFile(QString string);

	void ShowLogEditor();

  // Controls
  TitleCtrls    m_titleCtrls;

  ModeCtrls     m_modeCtrls;
  OptionsCtrls  m_optionsCtrls;
  OnlineCtrls   m_onlineCtrls;
  ReviewCtrls   m_reviewCtrls;
  ToolsCtrls    m_toolsCtrls;
  PalWidget     m_palSelect;
  InfoCtrls     m_infoCtrls;
  CursorCtrls   m_cursorCtrls;

  InfoForm		m_infoForm;
  HelpForm		m_helpForm;

  // Forms
  SettingsForm  m_settings;
  ConnectForm   m_connectForm;
  DeviceForm    m_deviceForm;

  // Data
  RmGlWidget    m_fanDisplay;       // The fan display widget
  SonarSurface* m_pSonarSurface;    // The sonar surface
  OsClientCtrl  m_oculusClient;     // The network client control
  OsStatusRx    m_oculusStatus;     // The status receive UDP socket
  RmLogger      m_logger;           // The logging object
  RmPlayer      m_player;           // The player object
  QLabel        m_info;             // Information string display

	QString									m_themeName;

	bool										m_measureMode;
	bool		m_reconnect;

	bool	m_timeout;

	OculusVersionInfo				m_versionInfo;
	OculusPartNumberType		m_partNumber;


  // UI Data
  // mode 1 - 750kHz, 256 beams, 120º aperture
  // mode 2 - 1.5MHz, 128 beams, 60º aperture
  // mode 3 - 1.8MHz, 256 beams, 50º aperture

  //QStringList   m_sonarList;        // List of sonars we can connect to
  QMap<uint32_t, OculusStatusMsg>      m_sonarList;
  QMap<uint32_t, QDateTime>            m_statusMessageTimes;
  QMutex                               m_statusMessageTimesLock;
  QMutex                               m_sonarLock;


  eDisplayMode  m_displayMode;      // What display type to use
  QString       m_ipFromStatus;     // The IP of the sonar from the status message
  QString       m_ipDevStatus;

  uint32_t       m_bootVersion;
  uint32_t       m_armVersion;

  // Replay position data
  QString       m_replayFile;       // Name of the current file for replay
  quint64*      m_pEntries;         // File position entry table for the current reply file
  int           m_nEntries;         // Number of oculus entries found in the replay file
  int           m_index;            // Current entry index
  int			m_indexLower;
  int			m_indexUpper;		// Upper entry index
  OsBufferEntry m_entry;            // The last read oculus message
  QTimer        m_replay;           // The replay timer

  quint64*		m_pViewInfoEntries;
  int			m_nViewInfoEntries;

  // Buffer for sonar header data
  ApSonarDataHeader m_sonarReplay;
  bool              m_useRawSonar;

  QDateTime         m_payloadDateTime;

  int				m_playSpeed;


  // Current sonar information
  OculusInfo		*m_pSonarInfo;



  void UpdateLogFileName();

  void AbortReconnect();

  void UpdateSonarInfo(OculusPartNumberType pn);

signals:
  void NewSonarDetected();
  void SonarClientStateChanged();

public slots:
  void NewStatusMsg(OculusStatusMsg osm, quint16 valid, quint16 invalid);
  void NewReturnFire(OsBufferEntry* pEntry);
  void NewUserConfig(UserConfig config);
  void FireSonar();
  void SpawnOculusWebView();
  void PalSelected(int pal);
  void ConnectionFailed(QString error);
  void OnNewPayload(unsigned short type, unsigned short version, double time, unsigned payloadSize, quint8* pPayload);
  void ReviewEntryChanged(int enrr);
  void ReviewLowerEntryChanged(int entry);
  void ReviewUpperEntryChanged(int entry);
  void StartReplay();
  void StopReplay();
  void PlayNext();
  void StyleChanged(QString name);
  void MouseInfo(float dist, float angle, float x, float y);
  void MonitorAvailableSonars();

  void SocketTimeout();
  void SocketReconnecting();
  void SocketDisconnected();
};

