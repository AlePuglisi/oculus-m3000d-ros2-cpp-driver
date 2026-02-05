#include <QFileDialog>
#include <QStandardPaths>
#include <QSettings>
#include <QDebug>

#include "SettingsCtrls.h"
#include "ui_SettingsCtrls.h"

SettingsCtrls::SettingsCtrls(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::SettingsCtrls)
{
  // Initialise the log directory to be the directory the software is run from
  m_logDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);

  m_logSize = 500;

  ui->setupUi(this);
  //ui->logDir->setText(m_logDir);
  //ui->logSize->setText(QString::number(m_logSize));

  connect(ui->browseDir, &QPushButton::clicked, this, &SettingsCtrls::SelectLogDirectory);
  //connect(ui->logSize, &QLineEdit::textChanged, this, &SettingsCtrls::SetLogMaxSize);
  connect(ui->logSize, &QLineEdit::editingFinished, this, &SettingsCtrls::SetLogMaxSize);

  UpdateCtrls();
}

SettingsCtrls::~SettingsCtrls()
{
  delete ui;
}

// ----------------------------------------------------------------------------
void SettingsCtrls::SelectLogDirectory()
{
  QFileDialog fd;

	// Select the log directory
  QString logDir = fd.getExistingDirectory(nullptr, "Select directory to log to");

  if (logDir.length() > 0)
  {
	m_logDirTemp = logDir;
	ui->logDir->setText(m_logDirTemp);

	//emit NewLogDirectory(m_logDir);
  }  
}

// ----------------------------------------------------------------------------
void SettingsCtrls::SetLogMaxSize()
{
	//qDebug() << ui->logSize->text();

	uint32_t logSize = (uint32_t)ui->logSize->text().split(" ").first().toUInt();

	// Clip the log size to reasonable ranges
	if (logSize < 50) {
		logSize = 50;
	}
	else if (logSize > 500) {
		logSize = 500;
	}

	// Update the text
	ui->logSize->setText(QString::number(logSize));

	m_logSizeTemp = logSize;

	//emit MaxLogSize(m_logSize);
}

void SettingsCtrls::UpdateCtrls() {

	ui->logDir->setText(m_logDir);
	ui->logSize->setText(QString::number(m_logSize));
}

// ----------------------------------------------------------------------------
void SettingsCtrls::ReadSettings()
{
  QSettings settings;
  m_logDir  = settings.value("LogDir", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();
  m_logSize = settings.value("LogMaxSize", 500).toInt();

  if (m_logSize < 50) {
	  m_logSize = 50;
  }
  else if (m_logSize > 500) {
	  m_logSize = 500;
  }

  m_logSizeTemp = m_logSize;

  if (m_logDir.length() == 0) {
	  m_logDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);

  }

  m_logDirTemp = m_logDir;

  emit NewLogDirectory(m_logDir);
  emit MaxLogSize(m_logSize);

  UpdateCtrls();
}

// ----------------------------------------------------------------------------
void SettingsCtrls::WriteSettings()
{
  QSettings settings;
  settings.setValue("LogDir", m_logDir);
  settings.setValue("LogMaxSize", m_logSize);
}
