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
#include <QLineEdit>
#include <QLabel>
#include <QToolButton>
#include <QMutex>
#include <QAction>
#include <QUrl>
#include <QStateMachine>
#include <QState>
#include <QNetworkAccessManager>
#include <QMenuBar>
#include <QSplitter>

#include "qtclient.h"
#include "ChannelTreeWidget.h"
#include "MetronomeBar.h"
#include "ChatOutput.h"
#include "SettingsDialog.h"
#include "EffectProcessor.h"
#include "LoudNoiseDetector.h"
#include "NJClient.h"
#include "PortAudioSettingsPage.h"
#include "PortAudioStreamer.h"
#include "PortMidiSettingsPage.h"
#include "PortMidiStreamer.h"
#include "UISettingsPage.h"

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  MainWindow(QWidget *parent = 0);
  ~MainWindow();

  void Connect(const QString &host, const QString &user, const QString &pass);

signals:
  void Connecting();
  void Connected();
  void Disconnected();

public slots:
  void ShowConnectDialog();
  void Disconnect();
  void SendChatMessage(const QString &line);

private slots:
  void AudioStoppedUnexpectedly();
  void ChatInputReturnPressed();
  void ChatLinkClicked(const QUrl &url);
  void UserInfoChanged();
  void ClientStatusChanged(int newStatus);
  void BeatsPerIntervalChanged(int bpm);
  void BeatsPerMinuteChanged(int bpi);
  void LoudNoiseDetected();
  void RemoteChannelMuteChanged(int useridx, int channelidx, bool mute);
  void ShowLog();
  void ShowAboutDialog();
  void VoteBPMDialog();
  void VoteBPIDialog();
  void XmitToggled(bool checked);
  void MetronomeToggled(bool checked);
  void Reconnect();
  void AdminTopicDialog();
  void AdminBPMDialog();
  void AdminBPIDialog();
  void AdminAccessControlDialog();
  void KickMenuAboutToShow();
  void KickMenuTriggered(QAction *action);
  void Startup();
  void SettingsDialogClosed();

private:
  NJClient client;
  LoudNoiseDetector loudNoiseDetector;
  QUrl jammrApiUrl;
  QString jammrAuthToken;
  PortAudioStreamer portAudioStreamer;
  PortMidiStreamer portMidiStreamer;
  EffectProcessor *effectProcessor;
  SettingsDialog *settingsDialog;
  PortAudioSettingsPage *portAudioSettingsPage;
  PortMidiSettingsPage *portMidiSettingsPage;
  UISettingsPage *uiSettingsPage;
  QNetworkAccessManager *netManager;
  ChatOutput *chatOutput;
  QLineEdit *chatInput;
  ChannelTreeWidget *channelTree;
  QSplitter *splitter;
  QAction *connectAction;
  QAction *disconnectAction;
  QMenuBar *globalMenuBar;
  QMenu *voteMenu;
  QMenu *adminMenu;
  QAction *adminTopicAction;
  QAction *adminBPMAction;
  QAction *adminBPIAction;
  QAction *adminAccessControlAction;
  QMenu *kickMenu;
  QLabel *bpmLabel;
  QLabel *bpiLabel;
  MetronomeBar *metronomeBar;
  QToolButton *xmitButton;
  QToolButton *metronomeButton;
  QStateMachine *connectionStateMachine;
  QState *connectingState;
  QState *connectedState;
  QState *disconnectedState;
  QTimer *reconnectTimer;
  int reconnectTries;
  int reconnectMilliseconds;
  int defaultChatInputFontSize;
  bool detectLoudNoises;
  bool loudNoiseDetected;

  void setupChannelTree();
  void setupStatusBar();
  void setupPortAudioSettingsPage();
  void setupPortMidiSettingsPage();
  void setupUISettingsPage();
  bool tryReconnect();
  void resetReconnect();
  void updateChatFontSize(int size);
  void closeEvent(QCloseEvent *event);
  void ShowNINJAMConnectDialog();
  void ShowJammrConnectDialog();
  void OnSamples(float **inbuf, int innch, float **outbuf, int outnch, int len, const PaStreamCallbackTimeInfo *timeInfo);
  void chatAddLine(const QString &prefix, const QString &content,
                   const QString &href = "", const QString &linktext = "");
  void chatAddMessage(const QString &src, const QString &msg,
                      const QString &href = "", const QString &linktext = "");
  bool LicenseCallback(const char *licensetext);
  void ChatMessageCallback(char **parms, int nparms);
  static void OnSamplesTrampoline(float **inbuf, int innch, float **outbuf, int outnch, int len, const PaStreamCallbackTimeInfo *timeInfo);
  static int LicenseCallbackTrampoline(int user32, char *licensetext);
  static void ChatMessageCallbackTrampoline(int user32, NJClient *inst, char **parms, int nparms);
};

#endif /* _MAINWINDOW_H_ */
