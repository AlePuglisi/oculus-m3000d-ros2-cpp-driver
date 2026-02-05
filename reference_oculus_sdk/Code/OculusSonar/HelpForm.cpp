#include "HelpForm.h"
#include "ui_HelpForm.h"

HelpForm::HelpForm(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::HelpForm)
{
	ui->setupUi(this);

	ui->version->setText(QApplication::applicationVersion());
}

HelpForm::~HelpForm()
{
	delete ui;
}
