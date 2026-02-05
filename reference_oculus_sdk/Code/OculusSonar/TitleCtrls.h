#ifndef TITLECTRLS_H
#define TITLECTRLS_H

#include <QWidget>

class MainView;

namespace Ui {
class TitleCtrls;
}

class TitleCtrls : public QWidget
{
    Q_OBJECT

public:
    explicit TitleCtrls(QWidget *parent = 0);
    ~TitleCtrls();

    void LayoutCtrls(QRect r);

    // Data
    MainView *m_pMainView;

    void CloseApp();
    void MaximizeApp();
    void MinimizeApp();
    void ShowSettings();
	void ShowHelp();

    void SetTitle(QString title);

protected:
    void mouseMoveEvent(QMouseEvent* event);
    void mousePressEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);
    void mouseDoubleClickEvent(QMouseEvent* event);


private:
    Ui::TitleCtrls *ui;

    QPoint mLastMousePosition;
    bool mMoving;
};

#endif // TITLECTRLS_H
