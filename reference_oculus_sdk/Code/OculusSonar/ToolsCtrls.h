#ifndef TOOLSCTRLS_H
#define TOOLSCTRLS_H

#include <QWidget>


// forward definition for the main window
class MainView;

namespace Ui {
class ToolsCtrls;
}

class ToolsCtrls : public QWidget
{
    Q_OBJECT

public:
    explicit ToolsCtrls(QWidget *parent = 0);
    ~ToolsCtrls();


    // Data
    MainView*      m_pMainWnd;           // Pointer to the main window

    // Methods
    void ReadSettings();
    void WriteSettings();

	void ToggleFlipHoriz();
	void ToggleFlipVert();
	void ToggleMeasure();

private:
    Ui::ToolsCtrls *ui;

    QTimer *m_pTimer;

private slots:
  void on_palette_clicked();

 // void on_flipHoriz_clicked();
 // void on_flipVert_clicked();

public slots:

  void on_snapshot_clicked();

	void ShowPalette(bool checked);
	void TakeMeasurement(bool checked);

	void RotateView();

	void FlipHoriz(bool checked);
	void FlipVert(bool checked);

};

#endif // TOOLSCTRLS_H
