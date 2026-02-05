#include "ToolsCtrls.h"
#include "ui_ToolsCtrls.h"

#include "MainView.h"

#include "../Displays/SonarSurface.h"

#include <QSettings>

// -----------------------------------------------------------------------------
ToolsCtrls::ToolsCtrls(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ToolsCtrls)
{
    // Attach the parent as a typed main window
    m_pMainWnd = (MainView*) parent;

    ui->setupUi(this);

    ui->snapshotInfo->setVisible(false);

    m_pTimer = new QTimer();
    m_pTimer->setInterval(3000);
    connect(m_pTimer, &QTimer::timeout, [=]() {
        ui->snapshotInfo->setVisible(false);
        m_pTimer->stop();
    });

    // Connect up the signals/slots
    connect(ui->palette, &QPushButton::toggled, this, &ToolsCtrls::ShowPalette);
    connect(ui->measure, &QPushButton::toggled, this, &ToolsCtrls::TakeMeasurement);
	connect(ui->flipHoriz, &QPushButton::toggled, this, &ToolsCtrls::FlipHoriz);
	connect(ui->flipVert, &QPushButton::toggled, this, &ToolsCtrls::FlipVert);

		//connect(ui->rotateView, &QPushButton::clicked, this, &ToolsCtrls::RotateView);
}

// -----------------------------------------------------------------------------
ToolsCtrls::~ToolsCtrls()
{
    delete ui;
}

// -----------------------------------------------------------------------------
void ToolsCtrls::on_palette_clicked()
{
   // Toggle the visibility of the palette
   //m_pMainWnd->m_palSelect.setVisible(!m_pMainWnd->m_palSelect.isVisible());
}

// -----------------------------------------------------------------------------
void ToolsCtrls::on_snapshot_clicked()
{
   // Take a snapshot of the current window
   if (m_pMainWnd->Snapshot()) {

	   ui->snapshotInfo->setVisible(true);
	   m_pTimer->start();
   }
}

// -----------------------------------------------------------------------------
void ToolsCtrls::ShowPalette(bool checked)
{
    // Toggle the visibility of the palette
    m_pMainWnd->m_palSelect.setVisible(checked);
}

// -----------------------------------------------------------------------------
void ToolsCtrls::TakeMeasurement(bool checked)
{
    m_pMainWnd->SetMeasureMode(checked);
}

void ToolsCtrls::FlipHoriz(bool checked) {
	Q_UNUSED(checked);
	m_pMainWnd->FlipX(checked);
}

void ToolsCtrls::FlipVert(bool checked) {
	Q_UNUSED(checked);
	m_pMainWnd->FlipY(checked);
}

// -----------------------------------------------------------------------------
void ToolsCtrls::RotateView()
{
	// Change the rotation of the display
	//m_pMainWnd->RotateView();

}

// -----------------------------------------------------------------------------
void ToolsCtrls::ReadSettings()
{
    QSettings settings;

	bool vert = settings.value("UpDownState", 0).toBool();
	bool horiz = settings.value("LeftRightState", 0).toBool();

    // Flip Y
	ui->flipVert->setChecked(vert);
    // Flip X
	ui->flipHoriz->setChecked(horiz);
}

// -----------------------------------------------------------------------------
void ToolsCtrls::WriteSettings()
{
    QSettings settings;

    settings.setValue("UpDownState", ui->flipVert->isChecked());
    settings.setValue("LeftRightState", ui->flipHoriz->isChecked());
}


void ToolsCtrls::ToggleFlipHoriz() {
	bool check = ui->flipHoriz->isChecked();
	ui->flipHoriz->setChecked(!check);
}

void ToolsCtrls::ToggleFlipVert() {
	bool check = ui->flipVert->isChecked();
	ui->flipVert->setChecked(!check);
}

void ToolsCtrls::ToggleMeasure() {
	bool check = ui->measure->isChecked();
	ui->measure->setChecked(!check);
}
