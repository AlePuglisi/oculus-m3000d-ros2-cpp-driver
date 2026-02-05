#ifndef SETTINGSCTRLS_H
#define SETTINGSCTRLS_H

#include <QWidget>

namespace Ui {
class SettingsCtrls;
}

class SettingsCtrls : public QWidget
{
  Q_OBJECT

public:
  explicit SettingsCtrls(QWidget *parent = 0);
  ~SettingsCtrls();

  // Methods
  void SelectLogDirectory();
  void SetLogMaxSize();
  void ReadSettings();
  void WriteSettings();

  void UpdateCtrls();

  // Data
  QString m_logDir;
   QString m_logDirTemp;
  uint32_t m_logSize;
uint32_t m_logSizeTemp;

signals:
  void NewLogDirectory(QString dir);
  void MaxLogSize(uint32_t size);

private:
  Ui::SettingsCtrls *ui;
};

#endif // SETTINGSCTRLS_H
