#ifndef DEVICEFORM_H
#define DEVICEFORM_H

#include <QDialog>

#include "../Oculus/Oculus.h"
#include "../Oculus/DataWrapper.h"
#include "../Oculus/OssDataWrapper.h"

#include "../Oculus/OsClientCtrl.h"

class MainView;

namespace Ui {
class DeviceForm;
}

class OsFirwareUpdate;

class DeviceForm : public QDialog
{
    Q_OBJECT

public:
    explicit DeviceForm(QWidget *parent = 0);
    ~DeviceForm();

    // Data
    MainView* m_pMainView;

    bool      m_gainAssist;
    uint8_t   m_gammaCorrection;
    uint8_t   m_netSpeedLimit;

	bool      m_gainAssistNew;
	uint8_t   m_gammaCorrectionNew;
	uint8_t   m_netSpeedLimitNew;

    void UpdateControls();

    void showEvent(QShowEvent *);

    void accept();

	void setPartNo(OculusPartNumberType);

	void ValidateIpAddress();
	void ValidateNetmask();

public slots:
    void DhcpChanged(int value);
    void GainAssistChanged(int value);
    void GammaCorrectionChanged(int value);
    void NetSpeedLimitChanged(int value);
		//void AltFrequencyChanged(int value);

	void NewUserConfig();
	void NewVersionInfo();

private:
    Ui::DeviceForm *ui;

	OculusPartNumberType m_partNo;
};

#endif // DEVICEFORM_H
