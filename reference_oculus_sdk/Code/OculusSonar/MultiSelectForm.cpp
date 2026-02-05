#include "MultiSelectForm.h"
#include "ui_MultiSelectForm.h"

MultiSelectForm::MultiSelectForm(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MultiSelectForm)
{
    ui->setupUi(this);
}

MultiSelectForm::~MultiSelectForm()
{
    delete ui;
}
