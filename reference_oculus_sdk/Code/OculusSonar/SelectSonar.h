#ifndef SELECTSONAR_H
#define SELECTSONAR_H

#include <QWidget>

namespace Ui {
class SelectSonar;
}

class SelectSonar : public QWidget
{
    Q_OBJECT

public:
    explicit SelectSonar(QWidget *parent = 0);
    ~SelectSonar();

private:
    Ui::SelectSonar *ui;
};

#endif // SELECTSONAR_H
