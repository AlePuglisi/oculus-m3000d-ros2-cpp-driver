#include "SelectSonar.h"
#include "ui_SelectSonar.h"

SelectSonar::SelectSonar(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SelectSonar)
{
    ui->setupUi(this);
}

SelectSonar::~SelectSonar()
{
    delete ui;
}
