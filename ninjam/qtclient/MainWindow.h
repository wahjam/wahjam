#ifndef _MAINWINDOW_H_
#define _MAINWINDOW_H_

#include <QMainWindow>
#include <QWidget>
#include <QTextEdit>
#include <QLineEdit>
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

  void Connect(const QString &host, const QString &user, const QString &pass);

  static MainWindow *GetInstance();

public slots:
  void LicenseCallback(const char *licensetext, bool *result);
  void ChatMessageCallback(char **parms, int nparms);
  void ChatInputReturnPressed();

private:
  static MainWindow *instance;

  NJClient client;
  audioStreamer *audio;
  bool audioEnabled;
  QMutex clientMutex;
  ClientRunThread *runThread;
  QTextEdit *chatOutput;
  QLineEdit *chatInput;

  void OnSamples(float **inbuf, int innch, float **outbuf, int outnch, int len, int srate);
  void chatAddLine(const QString &prefix, const QString &content);
  void chatAddMessage(const QString &src, const QString &msg);
  static void OnSamplesTrampoline(float **inbuf, int innch, float **outbuf, int outnch, int len, int srate);
  static int LicenseCallbackTrampoline(int user32, char *licensetext);
  static void ChatMessageCallbackTrampoline(int user32, NJClient *inst, char **parms, int nparms);
};

#endif /* _MAINWINDOW_H_ */
