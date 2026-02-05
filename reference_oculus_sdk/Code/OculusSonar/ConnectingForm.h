#ifndef CONNECTINGFORM_H
#define CONNECTINGFORM_H

#include <QDialog>

namespace Ui {
class ConnectingForm;
}

class ConnectingForm : public QDialog
{
    Q_OBJECT

public:
    explicit ConnectingForm(QWidget *parent = 0);
    ~ConnectingForm();

private:
    Ui::ConnectingForm *ui;
};

#endif // CONNECTINGFORM_H
