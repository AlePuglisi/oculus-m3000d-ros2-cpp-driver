#include "SettingsForm.h"
#include "ui_SettingsForm.h"

// -----------------------------------------------------------------------------
SettingsForm::SettingsForm(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsForm)
{
    this->setObjectName("SettingsWindow");

    ui->setupUi(this);

    // Add the tab pages to the tab container
    ui->tabs->addTab(&m_settingsCtrls, "General");
    ui->tabs->addTab(&m_envCtrls, "Environment");
    ui->tabs->addTab(&m_appCtrls, "Application");
}

// -----------------------------------------------------------------------------
SettingsForm::~SettingsForm()
{
    delete ui;
}

void SettingsForm::showEvent(QShowEvent *event) {
	m_envCtrls.UpdateCtrls();
	m_settingsCtrls.UpdateCtrls();
}

void SettingsForm::accept() {
	// Apply the settings
	QDialog::accept();

	// Update the env ctrls
	m_envCtrls.m_salinity = m_envCtrls.m_salinityTemp;
	m_envCtrls.m_speedOfSound = m_envCtrls.m_speedOfSoundTemp;
	m_envCtrls.m_svType = m_envCtrls.m_svTypeTemp;

	m_settingsCtrls.m_logDir = m_settingsCtrls.m_logDirTemp;
	m_settingsCtrls.m_logSize = m_settingsCtrls.m_logSizeTemp;

	emit m_settingsCtrls.NewLogDirectory(m_settingsCtrls.m_logDir);
	emit m_settingsCtrls.MaxLogSize(m_settingsCtrls.m_logSize);

}

// -----------------------------------------------------------------------------
void SettingsForm::ReadSettings()
{
    m_settingsCtrls.ReadSettings();
    m_envCtrls.ReadSettings();
    m_appCtrls.ReadSettings();
}

// -----------------------------------------------------------------------------
void SettingsForm::WriteSettings()
{
    m_settingsCtrls.WriteSettings();
    m_envCtrls.WriteSettings();
    m_appCtrls.WriteSettings();
}
