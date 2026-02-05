#ifndef INFOFORM_H
#define INFOFORM_H

#include <QDialog>
#include <QTimer>

namespace Ui {
	class InfoForm;
}

// -----------------------------------------------------------------------------
class InfoForm : public QDialog
{
	Q_OBJECT

public:
	explicit InfoForm(QWidget *parent = 0);
	~InfoForm();

	void setInfo(QString info);

	void showEvent(QShowEvent *);

	void layoutCtrls();

	void UpdateTimeout();

private:
	Ui::InfoForm *ui;

	QTimer m_tmo;
	QTimer m_tick;

	int m_count;

signals:
	void AbortReconnect();
};

#endif // INFOFORM_H
