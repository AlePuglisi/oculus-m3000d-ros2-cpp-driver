#ifndef SETTINGSFORM_H
#define SETTINGSFORM_H

#include <QDialog>

#include "EnvCtrls.h"
#include "SettingsCtrls.h"
#include "AppCtrls.h"

namespace Ui {
class SettingsForm;
}

class SettingsForm : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsForm(QWidget *parent = 0);
    ~SettingsForm();

    SettingsCtrls m_settingsCtrls;
    EnvCtrls      m_envCtrls;
    AppCtrls      m_appCtrls;

	void showEvent(QShowEvent *event);

	void accept();

    void ReadSettings();
    void WriteSettings();

private:
    Ui::SettingsForm *ui;
};

#endif // SETTINGSFORM_H
