#include <QDesktopServices>
#include <QUrl>
#include <QSettings>
#include <QStandardPaths>
#include <QGuiApplication>
#include <QScreen>
#include <QPixmap>
#include <QDir>
#include <QApplication>
#include <QStatusBar>
#include <QtWinExtras/qwinfunctions.h>
#include <QKeyEvent>
#include <QMessageBox>

#include "MainView.h"
#include "../Displays/SonarSurface.h"
#include "ConnectForm.h"
#include "ModeCtrls.h"

#define WIN32_LEAN_AND_MEAN
#include "Windows.h"
#include "qt_windows.h"

double MainView::NAVIGATION_RANGES[] = { 1, 2, 5, 7.5, 10, 20, 30, 40, 50, 75, 100, 120, 140, 160, 180, 200 };
double MainView::INSPECTION_RANGES[] = { 0.3, 0.5, 1, 2, 3, 4, 5, 7.5, 10, 20, 40 };

MainView::MainView(QWidget *parent) :
  QMainWindow(parent),
  m_titleCtrls(this),
  m_modeCtrls(this),
  m_optionsCtrls(this),
  m_onlineCtrls(this),
  m_reviewCtrls(this),
  m_toolsCtrls(this),
  m_settings(this),
  m_palSelect(this),
  m_info(this),
  m_fanDisplay(this),
  m_connectForm(this),
  m_deviceForm(this),
  m_infoCtrls(this),
	m_cursorCtrls(this),
	m_infoForm(this),
	m_timeout(false),
	m_reconnect(false),
	m_helpForm(this)
{    
  // Allow the size grip to resize the form when in "normal" mode
  statusBar()->setSizeGripEnabled(true);

  //m_titleCtrls.setObjectName("TitleBar");

  m_playSpeed = 1;

	m_partNumber = partNumberUndefined;

	m_indexLower = 0;

  // Default settings mode
  m_displayMode = offline;

  // Make sure the fan display is on the bottom
  m_fanDisplay.lower();

  // Add a sonar surface to the fan display widget
  m_pSonarSurface = new SonarSurface;
  // Apply background image to surface display here
  m_pSonarSurface->m_background = ":/Background.png";
  m_pSonarSurface->m_dwGridText = true;
  //m_pSonarSurface->m_clearColour.setRgb(66, 66, 66, 66);
	m_pSonarSurface->m_clearColour.setRgb(81, 81, 81);
  m_fanDisplay.RmglSetupSurface(m_pSonarSurface);

  // Diagnostic info label
  //m_modeCtrls.setInfo("hhhiuhiufegfiqgfeq");
  m_info.setText("hhhiuhiufegfiqgfeq");
  m_info.setStyleSheet("QLabel { color : gray; }");
  m_info.setAlignment(Qt::AlignHCenter);
  m_info.setVisible(false);

  // Settings are hidden by default
  //m_settings.setVisible(false);

  //m_infoCtrls.setVisible(false);

  m_cursorCtrls.setVisible(false);

  // Palette selection is hidden by default
  m_palSelect.setVisible(false);
  m_palSelect.m_palette = m_pSonarSurface->m_palIndex;

  // Replay index initialisation
  m_nEntries = 0;
  m_pEntries = nullptr;
  m_useRawSonar = false;

  m_nViewInfoEntries = 0;
  m_pViewInfoEntries = nullptr;

  m_sonarList.clear();
  m_statusMessageTimes.clear();

  // Set the default playback speed
  m_replay.setInterval(100); // 10Hz ?

  m_measureMode = false;

  // NULL the sonar information
  m_pSonarInfo = NULL;


  // Connect up the signals and slots

  // Update the fan when the palette is selected
  connect(&m_palSelect, &PalWidget::PalSelected, this, &MainView::PalSelected);

  // Connect the status output from the oculus status recieve
  connect(&m_oculusStatus, &OsStatusRx::NewStatusMsg, this, &MainView::NewStatusMsg);

  // Connect a connection failure to clear the connect button
  connect(&m_oculusClient.m_readData, &OsReadThread::NotifyConnectionFailed, this, &MainView::ConnectionFailed);

  // Connect a successful read to push the data into the system
  connect(&m_oculusClient.m_readData, &OsReadThread::NewReturnFire, this, &MainView::NewReturnFire);

  // Connect updated log directory to the logger
  connect(&m_settings.m_settingsCtrls, &SettingsCtrls::NewLogDirectory, &m_logger, &RmLogger::SetLogDirectory);
  connect(&m_settings.m_settingsCtrls, &SettingsCtrls::MaxLogSize, &m_logger, &RmLogger::SetMaxLogSize);

  connect(&m_settings.m_appCtrls, &AppCtrls::StyleChanged, this, &MainView::StyleChanged);

  // Link the player output to the payload slot
  connect(&m_player, &RmPlayer::NewPayload, this, &MainView::OnNewPayload);

  // Link the review slider with the show entry
  connect(&m_reviewCtrls, &ReviewCtrls::EntryChanged, this, &MainView::ReviewEntryChanged);
  connect(&m_reviewCtrls, &ReviewCtrls::LowerEntryChanged, this, &MainView::ReviewLowerEntryChanged);
  connect(&m_reviewCtrls, &ReviewCtrls::UpperEntryChanged, this, &MainView::ReviewUpperEntryChanged);
  connect(&m_reviewCtrls, &ReviewCtrls::OnPlay, this, &MainView::StartReplay);
  connect(&m_reviewCtrls, &ReviewCtrls::OnStop, this, &MainView::StopReplay);

  // Link the replay next item
  connect(&m_replay, &QTimer::timeout, this, &MainView::PlayNext);
  connect(&m_player, &RmPlayer::EndOfFile, this, &MainView::StopReplay);


  connect(&m_oculusClient.m_readData, &OsReadThread::socketTimeout, this, &MainView::SocketTimeout);
  connect(&m_oculusClient.m_readData, &OsReadThread::socketReconnected, this, &MainView::SocketReconnecting);
  connect(&m_oculusClient.m_readData, &OsReadThread::socketDisconnect, this, &MainView::SocketDisconnected);


  // Connect a user config
  connect(&m_oculusClient, &OsClientCtrl::NewUserConfig, &m_deviceForm, &DeviceForm::NewUserConfig);

  connect(&m_connectForm, &ConnectForm::RebuildSonarList, this, &MainView::RebuildSonarList);

  QTimer *timer = new QTimer(this);
  connect(timer, &QTimer::timeout, this, &MainView::MonitorAvailableSonars);
  timer->start(1000);

  connect(m_pSonarSurface, &SonarSurface::MouseInfo, this, &MainView::MouseInfo);
  connect(m_pSonarSurface, &SonarSurface::MouseEnter, this, &MainView::MouseEnterFan);
  connect(m_pSonarSurface, &SonarSurface::MouseLeave, this, &MainView::MouseLeaveFan);

  connect(&m_infoForm, &InfoForm::AbortReconnect, this, &MainView::AbortReconnect);

  ReadSettings();

}

MainView::~MainView()
{
  WriteSettings();

  // If there is an entry table then clena up
  if (m_pEntries)
  {
    delete m_pEntries;
    m_nEntries = 0;
  }

  if (m_pViewInfoEntries) {
	  delete m_pViewInfoEntries;
	  m_nViewInfoEntries = 0;
  }
}

// ----------------------------------------------------------------------------
void MainView::UpdateLogFileName()
{
    m_onlineCtrls.UpdateLogFileName();
}

// ----------------------------------------------------------------------------
// Fit the fan display widget just inside the window
void MainView::resizeEvent(QResizeEvent* event)
{
  Q_UNUSED(event);

  QRect r = rect();
  r.adjust(0, 0, 0, -100);
  r.translate(0, 60);

  if ((r.width() % 2) != 0)
	  r.setWidth(r.width()+1);

  if ((r.height() % 2) != 0)
	  r.setHeight(r.height()+1);

  m_fanDisplay.setGeometry(r);

  // Position teh control panels
  LayoutCtrls();
}

// ----------------------------------------------------------------------------
void MainView::keyPressEvent(QKeyEvent* event) {
	char key = (char)event->key();

	qDebug() << key;

	if (key == 'C') {
		// Connect/Disconnect
		m_modeCtrls.ToggleConnect();
	}
	else if (key == 'O') {
		// Open/Close
		m_modeCtrls.ToggleOpen();
	}
	else if ((key >= '1') && (key <= '6')) {
		int index = key - (int)('1');
		// Change the palette
		this->PalSelected(index);
		m_palSelect.m_palette = index;
		m_palSelect.update();
	}

	if (m_displayMode == online) {
		if (key == 'Q') {
			//qDebug() << "Change frequency";
			m_onlineCtrls.ToogleFrequency();
		}
		else if (key == 'W') {
			//qDebug() << "Increase range";
			m_onlineCtrls.IncreaseRange();
		}
		else if (key == 'S') {
			qDebug() << "Decrease range";
			m_onlineCtrls.DecreaseRange();
		}
		else if (key == 'A') {
			//qDebug() << "Reduce gain";
			m_onlineCtrls.DecreaseGain();
		}
		else if (key == 'D') {
			//qDebug() << "Increase gain";
			m_onlineCtrls.IncreaseGain();
		}
		else if (key == 'R') {
			//qDebug() << "Record";
			m_onlineCtrls.ToggleRecord();
		}
		else if (key == 'T') {
            // qDebug() << "Device setup";
        }
    }

    if ((m_displayMode == online) || (m_displayMode == review)) {
		if (key == 'P') {
			//qDebug() << "Snapshot";
			m_toolsCtrls.on_snapshot_clicked();
		}
		else if (key == 'H') {
			m_toolsCtrls.ToggleFlipHoriz();
		}
		else if (key == 'V') {
			m_toolsCtrls.ToggleFlipVert();
		}
		else if (key == 'M') {
			m_toolsCtrls.ToggleMeasure();
		}
		else if (key == 'G') {
			m_pSonarSurface->ToggleGridLines();
		}
	}

	if (m_displayMode == review) {
		if (key == ' ') {
			//qDebug() << "Play/Pause";
		}
	}
}

// ----------------------------------------------------------------------------
// Layout and set the visibility of the control panels
void MainView::LayoutCtrls()
{
  QRect r = rect();

  // Position the info string
  m_info.setGeometry(0, 0, r.width(), 16);
  // Position the title controls
  m_titleCtrls.setGeometry(0, 0, r.width(), 32);
  // Layout the title controls
  m_titleCtrls.LayoutCtrls(r);
  // Set the geometry of the mode controls
  m_modeCtrls.setGeometry(0, 32, m_modeCtrls.width(), m_modeCtrls.height());
  // Set the geometry of the options controls
  m_optionsCtrls.setGeometry(r.right() - m_optionsCtrls.width(), 32, m_optionsCtrls.width(), m_optionsCtrls.height());
  // Set the geometry of the Oculus info controls
  m_cursorCtrls.setGeometry(r.right() - m_cursorCtrls.width(), m_optionsCtrls.height() + m_titleCtrls.height() + 10, m_cursorCtrls.width(), m_cursorCtrls.height());

  m_infoCtrls.setGeometry(0, m_modeCtrls.height() + m_titleCtrls.height() + 10, m_infoCtrls.width(), m_infoCtrls.height());

  m_infoForm.layoutCtrls();

  // Set the geometry of the palette control
  QRect rect = m_palSelect.rect();
  m_palSelect.setGeometry(r.width() - rect.width() - 10, r.bottom() - rect.height() - 75, rect.width(), rect.height());

  // Position the online controls
  switch (m_displayMode)
  {
		case offline:
			m_fanDisplay.m_version.setVisible(true);
			m_onlineCtrls.setVisible(false);
			m_reviewCtrls.setVisible(false);
			m_toolsCtrls.setVisible(false);
			//m_infoCtrls.setVisible(false);
			break;

		case online:
			m_fanDisplay.m_version.setVisible(false);
			m_onlineCtrls.setGeometry(10, r.bottom() - 10 - m_onlineCtrls.height(), m_onlineCtrls.width(), m_onlineCtrls.height());
			m_onlineCtrls.setVisible(true);
			m_reviewCtrls.setVisible(false);
			m_toolsCtrls.setVisible(true);
			m_toolsCtrls.setGeometry(r.width() - 10 - m_toolsCtrls.width(), r.bottom() - 10 - m_toolsCtrls.height(), m_toolsCtrls.width(), m_toolsCtrls.height());

			//m_infoCtrls.setVisible(false);
			break;

		case review:
			m_fanDisplay.m_version.setVisible(false);
			m_onlineCtrls.setVisible(false);
			m_reviewCtrls.setVisible(true);
			m_reviewCtrls.setGeometry(10, r.bottom() - 10 - m_reviewCtrls.height(), m_reviewCtrls.width(), m_reviewCtrls.height());
			m_toolsCtrls.setVisible(true);
			m_toolsCtrls.setGeometry(r.width() - 10 - m_toolsCtrls.width(), r.bottom() - 10 - m_toolsCtrls.height(), m_toolsCtrls.width(), m_toolsCtrls.height());
			//m_infoCtrls.setVisible(false);
			break;
  }


  //m_infoForm.setInfo("Connection Lost!");

}

// ----------------------------------------------------------------------------
// Setup the controls display
void MainView::SetDisplayMode(eDisplayMode displayMode)
{
  m_displayMode = displayMode;

  // If we are set to review mode then set the sliderbar extents
  if (displayMode == review)
  {
    m_reviewCtrls.SetNEntries(m_nEntries);
  }
  else {
	  if (displayMode == offline) {
		  m_infoCtrls.HideInfo();
	  }

  }

  m_pSonarSurface->m_disconnected = (displayMode == offline ? true : false);
  m_fanDisplay.update();

  // Update the mode controls
  m_modeCtrls.setDisplayMode(displayMode);

  LayoutCtrls();
}

// ----------------------------------------------------------------------------
// Read the persistent settings from the file
void MainView::ReadSettings()
{
  QSettings settings;

  m_pSonarSurface->m_palIndex = settings.value("PaletteIndex", 1).toInt();
  m_pSonarSurface->m_headDown = settings.value("HeadDown", 1).toBool();
  m_pSonarSurface->m_flipX    = settings.value("FlipX", 1).toBool();

  m_deviceForm.m_gainAssist   = settings.value("GainAssist", 1).toBool();
  m_deviceForm.m_gammaCorrection = settings.value("GammaCorrection", 150).toInt();
  m_deviceForm.m_netSpeedLimit = settings.value("NetSpeedLimit", 100).toInt();
//	m_deviceForm.m_altFreq		  = settings.value("AltFreq", 0).toBool();

  m_onlineCtrls.ReadSettings();
  m_settings.ReadSettings();
  m_toolsCtrls.ReadSettings();

  m_palSelect.m_palette = m_pSonarSurface->m_palIndex;

  m_deviceForm.UpdateControls();
}

// ----------------------------------------------------------------------------
// Write the persistent settings from the file
void MainView::WriteSettings()
{
  QSettings settings;

  settings.setValue("PaletteIndex", m_pSonarSurface->m_palIndex);
  settings.setValue("HeadDown", m_pSonarSurface->m_headDown);
  settings.setValue("FlipX", m_pSonarSurface->m_flipX);
  settings.setValue("Style", m_themeName);

  settings.setValue("GainAssist", m_deviceForm.m_gainAssist);
  settings.setValue("GammaCorrection", m_deviceForm.m_gammaCorrection);
  settings.setValue("NetSpeedLimit", m_deviceForm.m_netSpeedLimit);
	//settings.setValue("AltFreq", m_deviceForm.m_altFreq);

  m_onlineCtrls.WriteSettings();
  m_settings.WriteSettings();
  m_toolsCtrls.WriteSettings();
}

// ----------------------------------------------------------------------------
// (SLOT) Open the current replay file and start replay from the current item
void MainView::StartReplay()
{
  int entry = m_reviewCtrls.GetEntry();

  if (entry == (m_nEntries - 2)) {
	  qDebug() << "End of file";
	  entry = 0;
  }

  if (m_pEntries && entry < m_nEntries)
  {
    m_index = entry;
    m_player.OpenFileAt(m_replayFile, m_pEntries[m_index]);
    m_replay.start();

  }
}

// ----------------------------------------------------------------------------
// (SLOT) Close the current replay
void MainView::StopReplay()
{
	m_replay.stop();
	m_player.CloseFile();

	m_reviewCtrls.SetStop();
}

void MainView::UpdateSonarInfo(OculusPartNumberType pn) {

	if (!m_oculusClient.IsOpen()) {

		return;
	}

	// Find the sonar based on a direct part number match
	int index = 0;
	while (OculusSonarInfo[index].partNumber != partNumberEnd) {
		if (OculusSonarInfo[index].partNumber == pn) {
			qDebug() << "Found sonar: " << pn;
			break;
		}
		index++;
	}

	// Copy across the sonar information
	m_pSonarInfo = new OculusInfo();
	memset(m_pSonarInfo, 0, sizeof(OculusInfo));

	qDebug() << "Part no: " << pn;

	m_pSonarInfo->partNumber = pn;
	m_pSonarInfo->hasLF = OculusSonarInfo[index].hasLF;
	m_pSonarInfo->maxLF = OculusSonarInfo[index].maxLF;
    m_pSonarInfo->hasHF = OculusSonarInfo[index].hasHF;
    m_pSonarInfo->maxHF = OculusSonarInfo[index].maxHF;

    m_pSonarInfo->model = new char[64];

    switch (pn) {
        case partNumberM370s:
        case partNumberMT370s:
        case partNumberMD370s:
        case partNumberMD370s_Burton:
        case partNumberMD370s_Impulse:
            sprintf(m_pSonarInfo->model, "Oculus M370s");
            break;
        case partNumberC550d:
            sprintf(m_pSonarInfo->model, "Oculus C550d");
            break;
        case partNumberM750d:
        case partNumberMT750d:
        case partNumberMD750d:
        case partNumberMD750d_Burton:
        case partNumberMD750d_Impulse:
            sprintf(m_pSonarInfo->model, "Oculus M750d");
            break;
        case partNumberM1200d:
        case partNumberMT1200d:
        case partNumberMD1200d:
        case partNumberMD1200d_Burton:
        case partNumberMD1200d_Impulse:
            sprintf(m_pSonarInfo->model, "Oculus M1200d");
            break;
        case partNumberM3000d:
        case partNumberMT3000d:
        case partNumberMD3000d_Burton:
        case partNumberMD3000d_Impulse:
            sprintf(m_pSonarInfo->model, "Oculus M3000d");
            break;
        case partNumberUndefined:
        default:
            sprintf(m_pSonarInfo->model, "Undefined");
            break;
    }
}

// ----------------------------------------------------------------------------
// (SLOT) Respond to status messages from the Recieve UDP socket
void MainView::NewStatusMsg(OculusStatusMsg osm, quint16 valid, quint16 invalid)
{
	// This occurs when a status message is received from any sonar on the network
	m_modeCtrls.EnableConnect(true);

	m_statusMessageTimesLock.lock();
	m_statusMessageTimes[osm.deviceId] = QDateTime::currentDateTime();
	m_statusMessageTimesLock.unlock();


	// Update the "known sonar"table.
	if (!m_sonarList.contains(osm.deviceId)) {
		// Update the sonar list
		m_sonarLock.lock();
		m_sonarList[osm.deviceId] = osm;
		m_sonarLock.unlock();

		emit NewSonarDetected();
	}
	else {

		// Check here to see if a client has connected or disconnected since last time
		if (m_sonarList.contains(osm.deviceId)) {

			uint32_t client = m_sonarList[osm.deviceId].connectedIpAddr;

			// Update the sonar
			m_sonarLock.lock();
			m_sonarList[osm.deviceId] = osm;
			m_sonarLock.unlock();

			// If the connect client IP address is different, emit a signal
			if (osm.connectedIpAddr != client) {
				emit SonarClientStateChanged();
			}
		}




	}

	/*
	m_ipFromStatus = QString::number(ip1) + "." + QString::number(ip2) + "." + QString::number(ip3) + "." + QString::number(ip4);

	// Split out the connected IP address
	ip1 = (uchar)(osm.connectedIpAddr & 0xff);
	ip2 = (uchar)((osm.connectedIpAddr & 0xff00) >> 8);
	ip3 = (uchar)((osm.connectedIpAddr & 0xff0000) >> 16);
	ip4 = (uchar)((osm.connectedIpAddr & 0xff000000) >> 24);

	m_ipDevStatus = QString::number(ip1) + "." + QString::number(ip2) + "." + QString::number(ip3) + "." + QString::number(ip4);

	// Update the IP address fields
	QString statusMsg = "Sonar:0x" + QString::number(osm.hdr.oculusId, 16) + " IP:"+ m_ipFromStatus + " Dev:" + m_ipDevStatus
	  + "  #:" + QString::number(valid) + "(" + QString::number(invalid) + ") ";
*/

	bool wasTimeout = m_timeout;


	// Split out the IP address
	uchar ip1 = (uchar)(osm.ipAddr & 0xff);
	uchar ip2 = (uchar)((osm.ipAddr & 0xff00) >> 8);
	uchar ip3 = (uchar)((osm.ipAddr & 0xff0000) >> 16);
	uchar ip4 = (uchar)((osm.ipAddr & 0xff000000) >> 24);

	QString addr = QString::number(ip1) + "." + QString::number(ip2) + "." + QString::number(ip3) + "." + QString::number(ip4);


	// Determine if this status message has been produced by the sonar we are presently
	// connected to. If it is our sonar, the update some on-screen labels, etc
	if (m_oculusClient.m_hostname == addr) {
		m_timeout = false;

		if (!m_pSonarInfo) {
			UpdateSonarInfo(osm.partNumber);

		}

		// Capture the information about the sonar
		if ((m_pSonarInfo) && (m_pSonarInfo->partNumber != partNumberUndefined)) {
		//if (m_pSonarInfo) {
			qDebug() << "Updating the part information";

			SetSonarInfo(osm);
			qDebug() << osm.partNumber;

			// Update the online controls to reflect the current part number
			m_onlineCtrls.UpdateControls(osm.partNumber);
		}




		m_onlineCtrls.UpdateRangeSlider(osm.partNumber);
	}


	// Show the connected ports
	QString indicators;
    indicators += (osm.status & (1 << 7) ? "Main " : "");
    qDebug() << indicators;


    // -------------------------------------------------------------------------
    // Keep alives!
    // -------------------------------------------------------------------------

    // If the main socket is open, keep it alive
    if (m_oculusClient.IsOpen()) {
        // Send a simple fire to keep triggering the sonar
		FireSonar();
    }

	// Add some logic to determine whether a connection has been lost. If the device
	// returns (with a suitable time period), automatically reconnect
    if ((wasTimeout) && (! m_timeout)) {
        // qDebug() << "Connecting after TMO";
        m_timeout = false;
        m_oculusClient.Disconnect();
        Sleep(500);
        m_oculusClient.Connect();
    }

}

// ----------------------------------------------------------------------------
// Monitor waits for available sonars
void MainView::MonitorAvailableSonars()
{
    QDateTime timeNow = QDateTime::currentDateTime();
    m_statusMessageTimesLock.lock();
    QMap<uint32_t, QDateTime>::const_iterator i = m_statusMessageTimes.constBegin();
    bool bRemoved = false;
    while (i != m_statusMessageTimes.constEnd()) {
       if (i.value().msecsTo(timeNow) > 2000) {
            // remove the entry from the sonar list
            m_sonarLock.lock();
            m_sonarList.remove(i.key());
            m_sonarLock.unlock();
            bRemoved = true;
       }
       ++i;
    }
    m_statusMessageTimesLock.unlock();
	if (bRemoved) {
        emit NewSonarDetected();

		if (m_sonarList.count() == 0) {
			// Disable the connect button
			this->m_modeCtrls.EnableConnect(false);
		}
	}
}

// ----------------------------------------------------------------------------
// (SLOT) A new sonar signal from the oculus client
void MainView::NewReturnFire(OsBufferEntry* pEntry)
{
    // pEntry->m_rfm.

    uint16_t dst = 0;

    pEntry->m_mutex.lock();
    {
        int width = 0;
        int height = 0;
        double range = 0;
        uint16_t ver = 0;

        // TODO Sort out for the m_rfm (full fire)

        if (pEntry->m_simple) {

            // Get the simple return version
            if (pEntry->m_version == 2) {
                width = pEntry->m_rfm2.nBeams;
                height = pEntry->m_rfm2.nRanges;
                range = height * pEntry->m_rfm2.rangeResolution;
                dst = pEntry->m_rfm2.fireMessage.head.srcDeviceId;
		  }
		  else {
                width = pEntry->m_rfm.nBeams;
                height = pEntry->m_rfm.nRanges;
                range = height * pEntry->m_rfm.rangeResolution;
                dst = pEntry->m_rfm.fireMessage.head.srcDeviceId;
            }
	  }
	  else {
            // Full fire
            width = pEntry->m_rff.ping.nBeams;
            height = pEntry->m_rff.ping_params.nRangeLinesBfm;
            range = pEntry->m_rff.ping.range;
            dst = pEntry->m_rff.head.srcDeviceId;
            ver = pEntry->m_rff.head.msgVersion;
        }


        // Add the new image data into teh osnar surface
        // int width  = pEntry->m_rfm.nBeams;
        // int height = pEntry->m_rfm.nRanges;
        // double range = height * pEntry->m_rfm.rangeResolution;

        // dst = pEntry->m_rfm.fireMessage.head.srcDeviceId;

        m_pSonarSurface->UpdateFan(range, width, pEntry->m_pBrgs, true);
        m_pSonarSurface->UpdateImg(height, width, pEntry->m_pImage);

        // Always pass through to the logger
	m_logger.LogData(rt_oculusSonar, ver, false, pEntry->m_rawSize, pEntry->m_pRaw);
    //m_modeCtrls.setInfo("Logging To: '" + m_logger.m_fileName + "' Size: " + QString::number((double)m_logger.m_loggedSize / (1024 * 1024), 'f', 1));
    m_info.setText("Logging To: '" + m_logger.m_fileName + "' Size: " + QString::number((double)m_logger.m_loggedSize / (1024 * 1024), 'f', 1));

        if (m_displayMode == review) {
            m_infoCtrls.HideInfo();
            // m_infoCtrls.UpdateModel(pEntry->m_rfm.head.partNumber);
        }
    }
    pEntry->m_mutex.unlock();

    // Keep the sonar alive
    FireSonar();

    // Update the dipslay
    m_fanDisplay.update();

    if (m_logger.LogIsActive()) {
        UpdateLogFileName();
    }
}

// ----------------------------------------------------------------------------
// (SLOT) A new sonar signal from the oculus client
void MainView::NewUserConfig(UserConfig config)
{
	m_oculusClient.m_wait.wakeAll();

    m_deviceForm.UpdateControls();
}

void MainView::StyleChanged(QString name)
{
    this->SetTheme(name);
}

// ----------------------------------------------------------------------------
// (SLOT) Send a simple fire message with the currents settings
void MainView::FireSonar()
{
    // If the main socket is not open, return
    if (! m_oculusClient.IsOpen()) {
        return;
    }

    // Cache the control values
    eSonarMode demand = m_onlineCtrls.m_demandMode;
    double range = m_onlineCtrls.m_demandRange;
    int gain = m_onlineCtrls.m_demandGain;
    double sos = m_settings.m_envCtrls.m_speedOfSound;
	bool gainAssist = m_deviceForm.m_gainAssist;
	uint8_t gamma = m_deviceForm.m_gammaCorrection;
	uint8_t netSpeedLimit = m_deviceForm.m_netSpeedLimit;

	if (netSpeedLimit == 100)
	  netSpeedLimit = 0xff; // Will turn off the network speed limiter for 1000 baseT operation

	switch (m_settings.m_envCtrls.m_svType) {
		case freshWater:
			m_oculusClient.Fire(demand, range, gain, 0.0, 0.0, gainAssist, gamma, netSpeedLimit);
			break;
		case saltWater:
			m_oculusClient.Fire(demand, range, gain, 0.0, 35.0, gainAssist, gamma, netSpeedLimit);
			break;
		case fixedValue:
			m_oculusClient.Fire(demand, range, gain, sos, 0.0, gainAssist, gamma, netSpeedLimit);
			break;
	}

}

// ----------------------------------------------------------------------------
// (SLOT) The palette has been modified so force an update on the display
void MainView::PalSelected(int pal)
{
	m_pSonarSurface->m_palIndex = pal;
	m_pSonarSurface->Recalculate();
}

// ----------------------------------------------------------------------------
// (SLOT) Spawn an external browser with the Artemis webv page
void MainView::SpawnOculusWebView()
{
  QDesktopServices::openUrl(QUrl("http://www.blueprintsubsea.com/oculus/"));
}

// ----------------------------------------------------------------------------
// (SLOT) The connection to the sonar failed ...
void MainView::ConnectionFailed(QString error)
{
  SetDisplayMode(offline);

  // need to deal with this
  //m_modeCtrls.setInfo(tr("Connection Failed: ") + error);
  m_info.setText(tr("Connection Failed: ") + error);
}

// ----------------------------------------------------------------------------
// (SLOT) Respond to new data from the player
void MainView::OnNewPayload(unsigned short type, unsigned short version, double time, unsigned payloadSize, quint8 *pPayload)
{
  //Q_UNUSED(time)
  Q_UNUSED(version)

	m_payloadDateTime = QDateTime::fromMSecsSinceEpoch((quint64)(time * 1000.0));

  if (m_displayMode == review) {
      m_reviewCtrls.SetPlaybackTime(m_payloadDateTime);
  }

  /*
  qDebug() << type;


  if (type == rt_ocViewInfo) {
	  // Update the view information with the settings
	  if (payloadSize == sizeof(OculusViewInfo)) {
		  OculusViewInfo info;
		  memcpy(&info, pPayload, sizeof(OculusViewInfo));

		  // Update the display controls
		  m_toolsCtrls.FlipHoriz(info.flipX);
		  m_toolsCtrls.FlipVert(info.flipY);
		  PalSelected(info.palette);

	  }
  }*/

  // ------------------------------------------------------
  // If we have an oculus sonar record then push it into the system
  if (type == rt_oculusSonar)
  {
    m_entry.ProcessRaw((char*)pPayload);
    NewReturnFire(&m_entry);
  }
  // Sonar head data - initialise the sonar view and the review characteristics
  else if (type == rt_apSonarHeader)
  {
	//qDebug() << "Header";

    if (payloadSize == sizeof(ApSonarDataHeader))
    {
      memcpy(&m_sonarReplay, pPayload, sizeof(ApSonarDataHeader));

	  //qDebug() << "Freq: " << m_sonarReplay.frequency;

      // Update the image extents
      m_pSonarSurface->UpdateFan(m_sonarReplay.range, m_sonarReplay.nBrgs, m_sonarReplay.pBrgs);

	  if (m_displayMode == review) {
		  //m_infoCtrls.UpdateModel(0);
		  //m_infoCtrls.UpdateFrequency(m_sonarReplay.frequency);
	  }
    }

  }
  // This is a raw image as defined by a previous rt_apSonarHeader
  else if (type == rt_rawSonarImage)
  {
    // Version 2 is 8bit uncompressed data
    if (version == 2)
    {
      if (payloadSize == (unsigned)(m_sonarReplay.nBrgs * m_sonarReplay.nRngs))
      {
        // Update the image extents
        m_pSonarSurface->UpdateFan(m_sonarReplay.range, m_sonarReplay.nBrgs, m_sonarReplay.pBrgs);
        m_pSonarSurface->m_nRngs = m_sonarReplay.nRngs;
        m_pSonarSurface->m_nBrgs = m_sonarReplay.nBrgs;

        m_pSonarSurface->m_pData = (quint8*) realloc (m_pSonarSurface->m_pData, payloadSize);

        memcpy(m_pSonarSurface->m_pData, pPayload, payloadSize);

        m_pSonarSurface->m_newImgData = true;

        // Update the dipslay
        m_fanDisplay.update();
      }
      else
        qDebug() << "The 8 bit image has incorrect size. PL:" + QString::number(payloadSize) + " IS:" + QString::number(m_sonarReplay.nBrgs * m_sonarReplay.nRngs);
    }
  }

}

// ----------------------------------------------------------------------------
// The review entry has changed
void MainView::ReviewEntryChanged(int entry)
{
	bool playing = m_replay.isActive();

	if (playing) {
	this->StopReplay();
	}

  if (m_pEntries && entry < m_nEntries)
  {
    m_index = entry;
    m_player.OpenFileAt(m_replayFile, m_pEntries[entry]);
    m_player.ReadNextItem();

    // Raw sonar records are in pairs
    if (m_useRawSonar)
      m_player.ReadNextItem();

    m_player.CloseFile();
  }

  if (playing)
	this->StartReplay();
}

// ----------------------------------------------------------------------------
void MainView::ReviewLowerEntryChanged(int entry) {

	bool playing = m_replay.isActive();
	// Stop any currently playing log
	if (playing)
		this->StopReplay();

	if (m_pEntries && entry < m_nEntries)
	{
	  m_indexLower = entry;
	  m_player.OpenFileAt(m_replayFile, m_pEntries[entry]);
	  m_player.ReadNextItem();

	  // Raw sonar records are in pairs
	  if (m_useRawSonar)
		m_player.ReadNextItem();

	  m_player.CloseFile();
	}
}

// ----------------------------------------------------------------------------
// The review entry has changed
void MainView::ReviewUpperEntryChanged(int entry) {

	bool playing = m_replay.isActive();
	// Stop any currently playing log
	if (playing)
		this->StopReplay();

	if (m_pEntries && entry < m_nEntries)
	{
	  m_indexUpper = entry;
	  m_player.OpenFileAt(m_replayFile, m_pEntries[entry]);
	  m_player.ReadNextItem();

	  // Raw sonar records are in pairs
	  if (m_useRawSonar)
		m_player.ReadNextItem();

	  m_player.CloseFile();
	}

}

// ----------------------------------------------------------------------------
void MainView::PlayNext()
{
  //qDebug() << "Play next";

  if (0 <= m_index && m_index < m_nEntries - 2)
  {
    m_index++;
	m_player.Seek(m_pEntries[m_index]);
    m_player.ReadNextItem();

    // Raw sonar records are in pairs
    if (m_useRawSonar)
      m_player.ReadNextItem();

	m_reviewCtrls.blockSignals(true);
	m_reviewCtrls.SetEntry(m_index);
	m_reviewCtrls.blockSignals(false);

	double next;
	m_player.PeekNextItem(&next);

	QDateTime nextTime = QDateTime::fromMSecsSinceEpoch((quint64)(next * 1000.0));

	//qDebug() << m_payloadDateTime;
	//qDebug() << nextTime;

	// Work out the time delta
	qint64 delta = m_payloadDateTime.msecsTo(nextTime);

	//qDebug() << "Delta time: " << delta;

	qDebug() << "Delta time: " << delta;

	// Default to a 10Hz replay speed if the delta time is stupid
	if ((delta < 10) || (delta > 500))
		delta = 100;

	// Account for the play speed
	delta = (delta / m_playSpeed);

	// Update the timer interval
	m_replay.setInterval(delta);
  }

  // Check if we need to reset the index if repeat is enabled
  if ((m_index >= (m_nEntries - 2)) && (m_player.m_repeat)) {
      m_index = 0;
  }

  if (m_index == (m_nEntries-2)) {
	  qDebug() << "Stop replay";
	  this->StopReplay();
  }

}

// ----------------------------------------------------------------------------
bool MainView::Snapshot()
{
    // Get the primary screen
    QScreen* screen = QGuiApplication::primaryScreen();
    //QPixmap pixmap = screen->grabWindow(0);
    QPixmap pixmap = this->grab();

    // Grab the framebuffer
    //QImage snapshot = m_pMainWnd->m_fanDisplay.grabFramebuffer();

    // Create the output filename
    QDateTime dt = QDateTime::currentDateTime();

    // Source file
    QString srcFile;
    QDateTime srcDate;

    // Work out what we're going to call our image file
    if (m_logger.m_fileName != "") {
        // Logging
        srcFile = m_logger.m_fileName;
    }
    else if (m_replayFile != "") {
        // Replaying
        srcFile = m_replayFile;
        srcDate = m_payloadDateTime;
    }
    else {
        // Online but not logging
        srcFile = m_logger.m_logDir + QDir::separator() + QString(m_logger.s_source);
    }




    // Check the date
    if (!srcDate.isValid()) {
        srcDate = QDateTime::currentDateTime();
    }

    // Compile a file name
    QFileInfo info(srcFile);
    QString destPath = info.dir().absolutePath();
    QString destFile = destPath + QDir::separator() + info.baseName() + srcDate.toString("_yyyyMMdd_hhmmss.png");

	// Create the log directory if it doesn't exist
	QDir ld(destPath);

	// If the log directory doesn't exist, create it
	if (!ld.exists(destPath)) {
		QMessageBox msg;
		msg.setIcon(QMessageBox::Critical);
		msg.setStandardButtons(QMessageBox::Ok);

		QString str = "Unable to save the snapshot.\r\n\r\nThe following directory does not exist:\r\n\r\n";
		str += destPath;

		msg.setText(str);
		msg.setWindowTitle("Snapshot Error");
		msg.exec();
		return false;
	}

    // Save the snapshot
    pixmap.save(destFile, "PNG");

	return true;
}

// ----------------------------------------------------------------------------
void MainView::StartLog()
{
    m_logger.OpenLog();

    if (m_logger.m_state == logging) {
      //m_modeCtrls.setInfo("Logging To: " + m_logger.m_fileName);
      m_info.setText("Logging To: " + m_logger.m_fileName);
    }
    else
    {
      //m_modeCtrls.setInfo("Logging Failed! Cannot Open: " + m_logger.m_fileName);
      m_info.setText("Logging Failed! Cannot Open: " + m_logger.m_fileName);
		m_onlineCtrls.CancelRecord();
      update();


    }

	/*
	OculusViewInfo info;
	info.flipX = m_pSonarSurface->m_flipX;
	info.flipY = m_pSonarSurface->m_headDown;
	info.palette = (uint8_t)m_palSelect.m_palette;

	// Write the view information to the log file
	m_logger.LogData(rt_ocViewInfo, 1, false, sizeof(OculusViewInfo), (unsigned char *)&info);
	*/
}

// ----------------------------------------------------------------------------
void MainView::StopLog()
{
    m_logger.CloseLog();

    if (m_logger.m_state == notLogging)
        m_info.setText("");
}

// ----------------------------------------------------------------------------
void MainView::FlipX(bool flip)
{
    SonarSurface* pSonar = (SonarSurface*) m_fanDisplay.RmglGetSurface();

	pSonar->m_flipX = flip;
    pSonar->Recalculate();
}

// ----------------------------------------------------------------------------
void MainView::FlipY(bool flip)
{
    SonarSurface* pSonar = (SonarSurface*)m_fanDisplay.RmglGetSurface();

	pSonar->m_headDown = !flip;
    pSonar->Recalculate();
}

// ----------------------------------------------------------------------------
void MainView::SetTheme(QString theme)
{
    // Get the current application instance
    QApplication *app = static_cast<QApplication *>(QCoreApplication::instance());

    QFile cssFile(":/" + theme + ".css");
    cssFile.open(QFile::ReadOnly);
    QString css = QString(cssFile.readAll());
    cssFile.close();

    // Apply the stylesheet
    app->setStyleSheet(css);

    m_themeName = theme;
}

// ----------------------------------------------------------------------------
void MainView::SetMeasureMode(bool enable)
{
    //m_measureMode = enable;

	// Enable the measurement tool
    m_pSonarSurface->m_measureEnable = enable;

    if (!enable) {
        m_pSonarSurface->m_showLastMeasurement = false;
        m_pSonarSurface->m_measuring = false;
        m_pSonarSurface->Update();
    }
}

// ----------------------------------------------------------------------------
void MainView::RebuildSonarList()
{
    m_sonarLock.lock();
    m_sonarList.clear();
    m_sonarLock.unlock();
}

// ----------------------------------------------------------------------------
void MainView::MouseInfo(float dist, float angle, float x, float y)
{
    // Update the mouse cursor location
    m_cursorCtrls.UpdateMouseInfo(dist, angle, x, y);
}

// ----------------------------------------------------------------------------
void MainView::SetSonarInfo(OculusStatusMsg msg)
{
	// Split out the connected IP address
	uchar ip1 = (uchar)(msg.ipAddr & 0xff);
	uchar ip2 = (uchar)((msg.ipAddr & 0xff00) >> 8);
	uchar ip3 = (uchar)((msg.ipAddr & 0xff0000) >> 16);
	uchar ip4 = (uchar)((msg.ipAddr & 0xff000000) >> 24);

	QString ipAddr = QString::number(ip1) + "." + QString::number(ip2) + "." + QString::number(ip3) + "." + QString::number(ip4);

	// Split out the connected IP address
	uchar msk1 = (uchar)(msg.ipMask & 0xff);
	uchar msk2 = (uchar)((msg.ipMask & 0xff00) >> 8);
	uchar msk3 = (uchar)((msg.ipMask & 0xff0000) >> 16);
	uchar msk4 = (uchar)((msg.ipMask & 0xff000000) >> 24);

	QString ipMask = QString::number(msk1) + "." + QString::number(msk2) + "." + QString::number(msk3) + "." + QString::number(msk4);

	// Capture the part number
	m_partNumber = msg.partNumber;

	// Update the sonar info display
	//m_infoCtrls.UpdateInfo(ipAddr, ipMask, QString::number(msg.deviceId), msg.versionInfo.masterVersion);

	if (m_displayMode == online) {
		//m_infoCtrls.setVisible(true);
		// Update the sonar info display
		m_infoCtrls.UpdateInfo(ipAddr, ipMask, QString::number(msg.deviceId));
		// Update any error status information
		m_infoCtrls.UpdateError(msg.status);

		m_infoCtrls.UpdateModel(msg.partNumber);
	}

}

// ----------------------------------------------------------------------------
void MainView::MouseLeaveFan()
{
    m_cursorCtrls.setVisible(false);
}

// ----------------------------------------------------------------------------
void MainView::MouseEnterFan()
{
    m_cursorCtrls.setVisible(true);
}

// ----------------------------------------------------------------------------
void MainView::SetTitleLogFile(QString string) {

    m_titleCtrls.SetTitle(string);

}

// ----------------------------------------------------------------------------
void MainView::SocketTimeout() {
	m_infoForm.setInfo("Oculus Ethernet Connection Timeout!");

	m_timeout = true;
}

// ----------------------------------------------------------------------------
void MainView::SocketReconnecting() {
	m_infoForm.hide();

	m_timeout = false;
}

// ----------------------------------------------------------------------------
void MainView::SocketDisconnected() {
	//qDebug() << "Timeout state: " << m_timeout;

	m_infoForm.hide();

	if (m_timeout) {
		m_modeCtrls.Disconnect();
		m_partNumber = OculusPartNumberType::partNumberUndefined;

		// Force all sonar entries to rebuild
		m_sonarLock.lock();
		m_sonarList.clear();
		m_statusMessageTimes.clear();
		m_sonarLock.unlock();

		m_modeCtrls.EnableConnect(false);

		m_timeout = false;
	}
}

void MainView::AbortReconnect() {
	this->SocketDisconnected();
}

