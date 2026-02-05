#include "InfoForm.h"
#include "ui_InfoForm.h"

#include <QDebug>

// -----------------------------------------------------------------------------
InfoForm::InfoForm(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::InfoForm) {

	ui->setupUi(this);


	m_tmo.setInterval(10000);

	m_tick.setInterval(1000);

	this->setWindowFlags(Qt::Window | Qt::FramelessWindowHint);

	connect(ui->abort, &QPushButton::clicked, this, &InfoForm::AbortReconnect);
	connect(&m_tmo, &QTimer::timeout, this, &InfoForm::AbortReconnect);
	connect(&m_tick, &QTimer::timeout, this, &InfoForm::UpdateTimeout);

}

void InfoForm::UpdateTimeout() {
	QString str = "awaiting reconnection... (abort in " + QString::number(m_count) + "s)";
	ui->label->setText(str);

	m_count--;
}

void InfoForm::showEvent(QShowEvent *event) {
	Q_UNUSED(event);

	m_count = 10;

	this->UpdateTimeout();
	m_tmo.start();
	m_tick.start();

}

// -----------------------------------------------------------------------------
InfoForm::~InfoForm() {
	delete ui;
}

// -----------------------------------------------------------------------------
void InfoForm::setInfo(QString info) {
	this->layoutCtrls();
	ui->infoLabel->setText(info);
	// Show the form
	this->show();
}

// -----------------------------------------------------------------------------
void InfoForm::layoutCtrls() {
	// Resize this dialog
	QRect p = ((QWidget *)(this->parent()))->geometry();
	// Get the rect of this control
	QRect c = this->rect();
	// Move its center to the center of the parent
	c.moveCenter(p.center());

	// Move to the top left of the rect - this has the effect of making the
	// control central on the screen
	this->move(c.topLeft());
}
