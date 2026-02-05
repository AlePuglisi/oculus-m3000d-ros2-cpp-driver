#include "ConnectCtls.h"
#include "ui_ConnectCtls.h"

ConnectCtls::ConnectCtls(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ConnectCtls)
{
    ui->setupUi(this);
}

ConnectCtls::~ConnectCtls()
{
    delete ui;
}
