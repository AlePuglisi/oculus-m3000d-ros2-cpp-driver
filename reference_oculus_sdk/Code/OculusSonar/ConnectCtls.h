#ifndef CONNECTCTLS_H
#define CONNECTCTLS_H

#include <QWidget>

namespace Ui {
class ConnectCtls;
}

class ConnectCtls : public QWidget
{
    Q_OBJECT

public:
    explicit ConnectCtls(QWidget *parent = 0);
    ~ConnectCtls();

private:
    Ui::ConnectCtls *ui;
};

#endif // CONNECTCTLS_H
