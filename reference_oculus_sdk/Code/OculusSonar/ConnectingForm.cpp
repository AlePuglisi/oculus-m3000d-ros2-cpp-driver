#include "ConnectingForm.h"
#include "ui_ConnectingForm.h"

// -----------------------------------------------------------------------------
ConnectingForm::ConnectingForm(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ConnectingForm)
{
    ui->setupUi(this);
}

// -----------------------------------------------------------------------------
ConnectingForm::~ConnectingForm()
{
    delete ui;
}
