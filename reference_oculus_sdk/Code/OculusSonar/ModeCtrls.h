#ifndef MODECTRLS_H
#define MODECTRLS_H

#include <QWidget>

class MainView;

namespace Ui {
class ModeCtrls;
}

class ModeCtrls : public QWidget
{
    Q_OBJECT

public:
    explicit ModeCtrls(QWidget *parent = 0);
    ~ModeCtrls();

    // Data
    MainView*      m_pMainWnd;           // Pointer to the main window

	QString m_lastDir;

    void setDisplayMode(int mode);

    void setInfo(QString info);

    void EnableConnect(bool enable);

	void OpenFileEx(QString fileName = "");

	void ToggleConnect();
	void ToggleOpen();

	bool ExecDeviceConfig();

private:
    Ui::ModeCtrls *ui;




public slots:

  void Connect(bool checked);
  void Disconnect();

  void OpenFile(bool checked);

  void CloseFile();
  void ShowDeviceConfig();

};

#endif // MODECTRLS_H
