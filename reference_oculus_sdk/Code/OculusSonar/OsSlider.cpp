#include "OsSlider.h"
#include "ui_OsSlider.h"

OsSlider::OsSlider(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OsSlider)
{
    ui->setupUi(this);
}

OsSlider::~OsSlider()
{
    delete ui;
}
