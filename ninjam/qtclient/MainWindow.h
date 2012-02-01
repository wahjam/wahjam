/*
    Copyright (C) 2012 Stefan Hajnoczi <stefanha@gmail.com>

    Wahjam is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Wahjam is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Wahjam; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

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
