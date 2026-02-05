#include "Recorder.h"
#include "ui_Recorder.h"

Recorder::Recorder(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Recorder)
{
    ui->setupUi(this);
}

Recorder::~Recorder()
{
    delete ui;
}
