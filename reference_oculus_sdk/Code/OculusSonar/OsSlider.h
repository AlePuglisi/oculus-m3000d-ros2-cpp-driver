#ifndef OSSLIDER_H
#define OSSLIDER_H

#include <QWidget>

namespace Ui {
class OsSlider;
}

class OsSlider : public QWidget
{
    Q_OBJECT

public:
    explicit OsSlider(QWidget *parent = 0);
    ~OsSlider();

private:
    Ui::OsSlider *ui;
};

#endif // OSSLIDER_H
