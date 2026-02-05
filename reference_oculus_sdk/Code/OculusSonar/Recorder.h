#ifndef RECORDER_H
#define RECORDER_H

#include <QDialog>



namespace Ui {
class Recorder;
}

class Recorder : public QDialog
{
    Q_OBJECT

public:
    explicit Recorder(QWidget *parent = 0);
    ~Recorder();

private:
    Ui::Recorder *ui;
};

#endif // RECORDER_H
