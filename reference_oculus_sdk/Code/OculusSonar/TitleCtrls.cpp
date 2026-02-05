#include "TitleCtrls.h"
#include "ui_TitleCtrls.h"

#include <QMouseEvent>

#include "MainView.h"

TitleCtrls::TitleCtrls(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TitleCtrls)
{
    this->setObjectName("TitleBar");

    m_pMainView = (MainView *)parent;

    ui->setupUi(this);

    connect(ui->close, &QPushButton::clicked, this, &TitleCtrls::CloseApp);
    connect(ui->maximize, &QPushButton::clicked, this, &TitleCtrls::MaximizeApp);
    connect(ui->minimize, &QPushButton::clicked, this, &TitleCtrls::MinimizeApp);
    connect(ui->settings, &QPushButton::clicked, this, &TitleCtrls::ShowSettings);
	connect(ui->help, &QPushButton::clicked, this, &TitleCtrls::ShowHelp);
}

TitleCtrls::~TitleCtrls()
{
    delete ui;
}

void TitleCtrls::LayoutCtrls(QRect r)
{
    ui->frame->lower();
    ui->frame->setGeometry(0, 0, r.right(), 32);
    ui->close->setGeometry(r.right() - 26 - 3, 2, 28, 28);
    ui->maximize->setGeometry(r.right() - 52 - 6, 2, 28, 28);
    ui->minimize->setGeometry(r.right() - 78 - 9, 2, 28, 28);
    ui->settings->setGeometry(r.right() - 130 - 12, 2, 28, 28);
	ui->help->setGeometry(r.right() - 156 - 15, 2, 28, 28);
    ui->title->setGeometry(10, 6, r.right() - 160, ui->title->height());
}

void TitleCtrls::CloseApp()
{
    QApplication::quit();
}

void TitleCtrls::MaximizeApp()
{
    // Toggle between maximized and normal states
    if (m_pMainView->isMaximized()) {
        m_pMainView->showNormal();
    }
    else {
        m_pMainView->showMaximized();
    }
}

void TitleCtrls::MinimizeApp()
{
    m_pMainView->showMinimized();
}

void TitleCtrls::ShowSettings()
{
     m_pMainView->m_settings.exec();
}

void TitleCtrls::ShowHelp() {
	m_pMainView->m_helpForm.exec();
}

void TitleCtrls::mousePressEvent(QMouseEvent* event)
{
    // Do nothing if the window is maximized or minimized
    if ((this->isMaximized()) || (this->isMinimized()))
        return;

    if(event->button() == Qt::LeftButton)
    {
        mMoving = true;
        mLastMousePosition = event->pos();
    }
}

void TitleCtrls::mouseMoveEvent(QMouseEvent* event)
{
    if( event->buttons().testFlag(Qt::LeftButton) && mMoving)
    {
        QPoint pos = this->mapToGlobal(event->pos()) - mLastMousePosition;

        m_pMainView->move(pos);

    }
}

void TitleCtrls::mouseReleaseEvent(QMouseEvent* event)
{
    if(event->button() == Qt::LeftButton)
    {
        mMoving = false;
    }
}

void TitleCtrls::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
        this->MaximizeApp();
}

void TitleCtrls::SetTitle(QString title) {

    if (title.length() != 0) {
        ui->title->setText("Oculus Demo - " + title);
    }
    else {
        ui->title->setText("Oculus Demo");
    }

}
