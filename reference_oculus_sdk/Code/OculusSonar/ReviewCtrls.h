#ifndef REVIEWCTRLS_H
#define REVIEWCTRLS_H

#include <QWidget>


class MainView;

namespace Ui {
class ReviewCtrls;
}

class ReviewCtrls : public QWidget
{
  Q_OBJECT

public:
  explicit ReviewCtrls(QWidget *parent = 0);
  ~ReviewCtrls();

  void SetNEntries(int nEntries);
  int  GetEntry();
  void SetEntry(int entry);

  // Data
  bool m_playing;

  MainView*      m_pMainWnd;           // Pointer to the main window

  void SetStop();
  void EnsureChecked();
  void SetPlaybackTime(QDateTime time);

signals:
  void EntryChanged(int entry);
  void LowerEntryChanged(int entry);
  void UpperEntryChanged(int entry);
  void OnPlay();
  void OnStop();

public slots:
  void SliderChanged(int value);
  void LowerSliderChanged(int value);
  void UpperSliderChanged(int value);
  void RepeatChanged(bool checked);
  void SpeedChanged(int value);
  void PlayChanged(bool checked);

private:
  Ui::ReviewCtrls *ui;
};

#endif // REVIEWCTRLS_H
