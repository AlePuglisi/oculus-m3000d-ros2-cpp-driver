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

#ifndef CONNECTFORM_H
#define CONNECTFORM_H

#include <QDialog>
#include <QListWidgetItem>
#include <QTimer>

class MainView;

namespace Ui {
class ConnectForm;
}

class ConnectForm : public QDialog
{
    Q_OBJECT

public:
    explicit ConnectForm(QWidget *parent = 0);
    ~ConnectForm();

    // Data
    MainView* m_pMainView;

    void NewSonar();

    void RefreshList();

    bool event(QEvent *pEvent)                    Q_DECL_OVERRIDE;

    QListWidgetItem *getSelection();

    int sonarCount();

private:
    Ui::ConnectForm *ui;

    QTimer          m_timeout;

signals:

    void RebuildSonarList();

public slots:

    void Timeout();

};

#endif // CONNECTFORM_H
