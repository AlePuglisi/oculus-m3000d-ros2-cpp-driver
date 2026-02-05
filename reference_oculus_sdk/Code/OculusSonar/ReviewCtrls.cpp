#include <QFileDialog>
#include <QDebug>

#include "../RmUtil/RmLogger.h"
#include "../Oculus/Oculus.h"

#include "ReviewCtrls.h"
#include "ui_ReviewCtrls.h"

#include "MainView.h"

ReviewCtrls::ReviewCtrls(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::ReviewCtrls)
{
  // Attach the parent as a typed main window
  m_pMainWnd = (MainView*) parent;

  ui->setupUi(this);
  m_playing = false;

  //connect(ui->reviewSlider, &QSlider::valueChanged, this, &ReviewCtrls::SliderChanged);
  connect(ui->reviewSlider, &RangeSlider::currentValueChanged, this, &ReviewCtrls::SliderChanged);
  connect(ui->reviewSlider, &RangeSlider::lowerValueChanged, this, &ReviewCtrls::LowerSliderChanged);
  connect(ui->reviewSlider, &RangeSlider::upperValueChanged, this, &ReviewCtrls::UpperSliderChanged);

  connect(ui->repeat, &QPushButton::toggled, this, &ReviewCtrls::RepeatChanged);

  connect(ui->speedSlider, &QSlider::valueChanged, this, &ReviewCtrls::SpeedChanged);

  connect(ui->play, &QPushButton::toggled, this, &ReviewCtrls::PlayChanged);

}

// -----------------------------------------------------------------------------
ReviewCtrls::~ReviewCtrls()
{
  m_pMainWnd = nullptr;

  delete ui;
}

// ----------------------------------------------------------------------------
// Setup the slider bat with the current number of entries
void ReviewCtrls::SetNEntries(int nEntries)
{
  ui->reviewSlider->setRange(0, nEntries - 1);

  ui->reviewSlider->setLowerValue(0);
  ui->reviewSlider->setUpperValue(nEntries);

}

// ----------------------------------------------------------------------------
// Return the current slider position
int ReviewCtrls::GetEntry()
{
  return ui->reviewSlider->currentValue();
}

// ----------------------------------------------------------------------------
void ReviewCtrls::SetEntry(int entry)
{
	ui->reviewSlider->setCurrentValue(entry);
}

// ----------------------------------------------------------------------------
// Pass through entry from the slider bar
void ReviewCtrls::SliderChanged(int value)
{
  emit EntryChanged(value);
}

void ReviewCtrls::LowerSliderChanged(int value) {
	emit LowerEntryChanged(value);
}

void ReviewCtrls::UpperSliderChanged(int value) {
	emit UpperEntryChanged(value);
}

// ----------------------------------------------------------------------------
// Speed changed
void ReviewCtrls::SpeedChanged(int value)
{
    int speeds[] = { 1, 2, 4, 8 };
    int speed = speeds[value];

	m_pMainWnd->m_playSpeed = speed;

    // Update the speed text
    ui->playbackSpeed->setText(QString::number(speed) + "x");
}

// ----------------------------------------------------------------------------
// Play changed
void ReviewCtrls::PlayChanged(bool checked)
{
    m_playing = checked;

    if (m_playing) {
        emit OnPlay();
    }
    else {
        emit OnStop();
    }

}

// ----------------------------------------------------------------------------
void ReviewCtrls::RepeatChanged(bool checked)
{
    m_pMainWnd->m_player.m_repeat = checked;
}

void ReviewCtrls::SetPlaybackTime(QDateTime time) {

		ui->playbackTime->setText(time.toString("dd-MM-yyyy HH:mm:ss.zzz"));

}

void ReviewCtrls::SetStop() {
	ui->play->setChecked(false);
}


void ReviewCtrls::EnsureChecked() {
	ui->play->setChecked(true);
}
