/******************************************************************************
 * (c) Copyright 2017 Blueprint Subsea.
 * This file is part of Oculus Viewer
 *
 * Oculus Viewer is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Oculus Viewer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

#include "ConnectForm.h"
#include "ui_ConnectForm.h"

#include "MainView.h"

// -----------------------------------------------------------------------------
ConnectForm::ConnectForm(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ConnectForm)
{

    m_pMainView = (MainView*) parent;

    m_timeout.setInterval(2000);

    ui->setupUi(this);

	connect(m_pMainView, &MainView::NewSonarDetected, this, &ConnectForm::NewSonar);
	connect(m_pMainView, &MainView::SonarClientStateChanged, this, &ConnectForm::NewSonar);
    connect(ui->refresh, &QPushButton::clicked, this, &ConnectForm::RefreshList);

	//connect(m_timeout,   &QTimer::timeout, this, &ConnectForm::Timeout);

    connect(ui->sonarList, &QListWidget::doubleClicked, this, &ConnectForm::accept);

    m_timeout.start();
}

// -----------------------------------------------------------------------------
ConnectForm::~ConnectForm()
{
    delete ui;
}

// -----------------------------------------------------------------------------
// This function is called whenever the status socket detects a new sonar that is not in the
// list maintained by the mainView.
// We should also call this when it is detected that a status message has not been received for
// two seconds indicating that a sonar is no longer available
void ConnectForm::NewSonar()
{
    // Clear the list and rebuild
    ui->sonarList->clear();

	if (m_pMainView->m_sonarList.count() == 0) {
		return;
	}

    // Cache the sonar list
    QMap<uint32_t, OculusStatusMsg> map = m_pMainView->m_sonarList;

    QMap<uint32_t, OculusStatusMsg>::const_iterator i = map.constBegin();        

    while (i != map.constEnd()) {

       // Create a QListItem
        QListWidgetItem *item = new QListWidgetItem(ui->sonarList);

        // Split out the IP address
        uchar ip1 = (uchar)(i.value().ipAddr & 0xff);
        uchar ip2 = (uchar)((i.value().ipAddr & 0xff00) >> 8);
        uchar ip3 = (uchar)((i.value().ipAddr & 0xff0000) >> 16);
        uchar ip4 = (uchar)((i.value().ipAddr & 0xff000000) >> 24);

        QString addr = QString::number(ip1) + "." + QString::number(ip2) + "." + QString::number(ip3) + "." + QString::number(ip4);

        // Split out the IP address
        uchar m1 = (uchar)(i.value().ipMask & 0xff);
        uchar m2 = (uchar)((i.value().ipMask & 0xff00) >> 8);
        uchar m3 = (uchar)((i.value().ipMask & 0xff0000) >> 16);
        uchar m4 = (uchar)((i.value().ipMask & 0xff000000) >> 24);

        QString mask = QString::number(m1) + "." + QString::number(m2) + "." + QString::number(m3) + "." + QString::number(m4);

		// Check if the sonar is available
        bool available = (i.value().connectedIpAddr == 0);

        QString str;

        switch (i.value().partNumber) {
            case partNumberM370s:
            case partNumberMT370s:
            case partNumberMD370s:
            case partNumberMD370s_Burton:
            case partNumberMD370s_Impulse:
                str = "M370s";
                break;
            case partNumberC550d:
                str = "C550d";
                break;
            case partNumberM750d:
            case partNumberMT750d:
            case partNumberMD750d:
            case partNumberMD750d_Burton:
            case partNumberMD750d_Impulse:
                str = "M750d";
                break;
            case partNumberM1200d:
            case partNumberMT1200d:
            case partNumberMD1200d:
            case partNumberMD1200d_Burton:
            case partNumberMD1200d_Impulse:
                str = "M1200d";
                break;
            case partNumberM3000d:
            case partNumberMT3000d:
            case partNumberMD3000d_Burton:
            case partNumberMD3000d_Impulse:
                str = "M3000d";
                break;
            case partNumberUndefined:
            default:
                str = "Undefined";
                break;
        }

        //uint64_t ver = ((uint64_t)(i.value().appVersion) << 32) | (uint64_t)(i.value().bitfileVersion);
        QString text;
		text += "Oculus:\t" + str + "\r\n";
        text += "Address:\t" + addr + "\r\n";
        text += "Mask\t" + mask + "\r\n";
        text += "Serial No:\t" + QString::number(i.value().deviceId) + "\r\n";
        text += "Available:\t" + QString(available ? "Yes" : "No");
        item->setText(text);
        item->setIcon(QIcon(":/Oculus.png"));
        //item->setData(Qt::UserRole, ver);

        // Check if someone is already connected, prevent the user from connecting

          if (i.value().connectedIpAddr != 0) {
            Qt::ItemFlags flags = item->flags();
            flags.setFlag(Qt::ItemIsSelectable, false);
            flags.setFlag(Qt::ItemIsEnabled, false);

            item->setFlags(flags);
        }

        ++i;     
    }
    ui->sonarList->update();
}

// -----------------------------------------------------------------------------
void ConnectForm::RefreshList()
{
    ui->sonarList->clear();
    emit RebuildSonarList();
}

// -----------------------------------------------------------------------------
void ConnectForm::Timeout()
{
    //
}

// -----------------------------------------------------------------------------
bool ConnectForm::event(QEvent* pEvent)
{
    if (pEvent->type() == QEvent::Show)
    {
        this->NewSonar();
    }

    return QDialog::event(pEvent);
}

// -----------------------------------------------------------------------------
QListWidgetItem* ConnectForm::getSelection()
{
    if (ui->sonarList->selectedItems().count() == 0) {
        return nullptr;
    }

    return ui->sonarList->selectedItems().first();
}

// -----------------------------------------------------------------------------
int ConnectForm::sonarCount()
{
    return ui->sonarList->count();
}
