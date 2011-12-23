#ifndef _MAINWINDOW_H_
#define _MAINWINDOW_H_

#include <QMainWindow>
#include <QWidget>
#include <QMutex>
#include "../njclient.h"
#include "../audiostream.h"

class ClientRunThread;

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  MainWindow(QWidget *parent = 0);
  ~MainWindow();

  static MainWindow *GetInstance();

public slots:
  void LicenseCallback(const char *licensetext, bool *result);

private:
  static MainWindow *instance;

  NJClient client;
  audioStreamer *audio;
  bool audioEnabled;
  QMutex clientMutex;
  ClientRunThread *runThread;

  void OnSamples(float **inbuf, int innch, float **outbuf, int outnch, int len, int srate);
  static void OnSamplesTrampoline(float **inbuf, int innch, float **outbuf, int outnch, int len, int srate);
  static int LicenseCallbackTrampoline(int user32, char *licensetext);
  void ChatMessageCallback(char **parms, int nparms);
  static void ChatMessageCallbackTrampoline(int user32, NJClient *inst, char **parms, int nparms);
};

#endif /* _MAINWINDOW_H_ */
