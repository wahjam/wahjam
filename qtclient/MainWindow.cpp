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

#include <QApplication>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QSplitter>
#include <QMenuBar>
#include <QMenu>
#include <QStatusBar>
#include <QDateTime>
#include <QDir>
#include <QDesktopServices>
#include <QUrl>
#include <QInputDialog>
#include <QRegExp>

#include "MainWindow.h"
#include "ConnectDialog.h"
#include "JammrConnectDialog.h"
#include "JammrLoginDialog.h"
#include "JammrAccessControlDialog.h"
#include "JammrUpdateChecker.h"
#include "PortAudioConfigDialog.h"
#include "VSTPlugin.h"
#include "VSTProcessor.h"
#include "VSTConfigDialog.h"
#include "common/njmisc.h"
#include "common/UserPrivs.h"

MainWindow *MainWindow::instance; /* singleton */

void MainWindow::OnSamplesTrampoline(float **inbuf, int innch, float **outbuf, int outnch, int len, int srate)
{
  MainWindow::GetInstance()->OnSamples(inbuf, innch, outbuf, outnch, len, srate);
}

int MainWindow::LicenseCallbackTrampoline(int user32, char *licensetext)
{
  return MainWindow::GetInstance()->LicenseCallback(licensetext);
}

void MainWindow::ChatMessageCallbackTrampoline(int user32, NJClient *inst, char **parms, int nparms)
{
  MainWindow::GetInstance()->ChatMessageCallback(parms, nparms);
}

MainWindow *MainWindow::GetInstance()
{
  return instance;
}

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent), audio(NULL)
{
  /* Since the ninjam callbacks do not pass a void* opaque argument we rely on
   * a global variable.
   */
  if (MainWindow::instance) {
    qFatal("MainWindow can only be instantiated once!");
    abort();
  }
  MainWindow::instance = this;

  client.config_savelocalaudio = -1;
  client.LicenseAgreementCallback = LicenseCallbackTrampoline;
  client.ChatMessage_Callback = ChatMessageCallbackTrampoline;
  client.SetLocalChannelInfo(0, "channel0", true, 0, false, 0, true, true);
  client.SetLocalChannelMonitoring(0, false, 0.0f, false, 0.0f, false, false, false, false);

  netManager = new QNetworkAccessManager(this);

  jammrApiUrl = settings->value("jammr/apiUrl", JAMMR_API_URL).toUrl();
  if (!jammrApiUrl.isEmpty()) {
    client.SetProtocol(JAM_PROTO_JAMMR);

    JammrUpdateChecker *updateChecker = new JammrUpdateChecker(this, netManager);
    updateChecker->setUpdateUrl(settings->value("jammr/updateUrl", JAMMR_UPDATE_URL).toUrl());
    updateChecker->setDownloadUrl(settings->value("jammr/downloadUrl", JAMMR_DOWNLOAD_URL).toUrl());
    updateChecker->start();
  }

  QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
  connectAction = fileMenu->addAction(tr("&Connect..."));
  disconnectAction = fileMenu->addAction(tr("&Disconnect"));
  audioConfigAction = fileMenu->addAction(tr("Configure &audio..."));
  QAction *vstConfigAction = fileMenu->addAction(tr("Configure &VST..."));
  QAction *exitAction = fileMenu->addAction(tr("E&xit"));
  exitAction->setShortcuts(QKeySequence::Quit);
  connect(connectAction, SIGNAL(triggered()), this, SLOT(ShowConnectDialog()));
  connect(disconnectAction, SIGNAL(triggered()), this, SLOT(Disconnect()));
  connect(audioConfigAction, SIGNAL(triggered()), this, SLOT(ShowAudioConfigDialog()));
  connect(vstConfigAction, SIGNAL(triggered()), this, SLOT(ShowVSTConfigDialog()));
  connect(exitAction, SIGNAL(triggered()), this, SLOT(close()));

  voteMenu = menuBar()->addMenu(tr("&Vote"));
  QAction *voteBPMAction = voteMenu->addAction(tr("BPM"));
  QAction *voteBPIAction = voteMenu->addAction(tr("BPI"));
  connect(voteBPMAction, SIGNAL(triggered()), this, SLOT(VoteBPMDialog()));
  connect(voteBPIAction, SIGNAL(triggered()), this, SLOT(VoteBPIDialog()));

  adminMenu = menuBar()->addMenu(tr("&Admin"));
  adminTopicAction = adminMenu->addAction(tr("Set topic"));
  adminBPMAction = adminMenu->addAction(tr("Set BPM"));
  adminBPIAction = adminMenu->addAction(tr("Set BPI"));
  adminAccessControlAction = adminMenu->addAction(tr("Access control..."));
  connect(adminTopicAction, SIGNAL(triggered()), this, SLOT(AdminTopicDialog()));
  connect(adminBPMAction, SIGNAL(triggered()), this, SLOT(AdminBPMDialog()));
  connect(adminBPIAction, SIGNAL(triggered()), this, SLOT(AdminBPIDialog()));
  connect(adminAccessControlAction, SIGNAL(triggered()),
          this, SLOT(AdminAccessControlDialog()));

  kickMenu = adminMenu->addMenu(tr("Kick"));
  connect(kickMenu, SIGNAL(aboutToShow()), this, SLOT(KickMenuAboutToShow()));
  connect(kickMenu, SIGNAL(triggered(QAction *)), this,
          SLOT(KickMenuTriggered(QAction *)));

  QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
  QAction *aboutAction = helpMenu->addAction(tr("&About..."));
  connect(aboutAction, SIGNAL(triggered()), this, SLOT(ShowAboutDialog()));

  setupStatusBar();
  client.config_metronome_mute = !metronomeButton->isChecked();

  setWindowTitle(tr(APPNAME));

  chatOutput = new ChatOutput(this);
  chatOutput->connect(chatOutput, SIGNAL(anchorClicked(const QUrl&)),
                      this, SLOT(ChatLinkClicked(const QUrl&)));

  chatInput = new QLineEdit(this);
  chatInput->connect(chatInput, SIGNAL(returnPressed()),
                     this, SLOT(ChatInputReturnPressed()));

  channelTree = new ChannelTreeWidget(this);
  connect(channelTree, SIGNAL(RemoteChannelMuteChanged(int, int, bool)),
          this, SLOT(RemoteChannelMuteChanged(int, int, bool)));

  metronomeBar = new MetronomeBar(this);
  connect(this, SIGNAL(Disconnected()),
          metronomeBar, SLOT(reset()));

  QSplitter *splitter = new QSplitter(this);
  QWidget *content = new QWidget;
  QVBoxLayout *layout = new QVBoxLayout;

  layout->setSpacing(2);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(chatOutput);
  layout->addWidget(chatInput);
  layout->addWidget(metronomeBar);
  content->setLayout(layout);
  content->setTabOrder(chatInput, chatOutput);

  splitter->addWidget(channelTree);
  splitter->addWidget(content);
  splitter->setOrientation(Qt::Vertical);
  splitter->setContentsMargins(5, 5, 5, 5);

  setCentralWidget(splitter);

  BeatsPerIntervalChanged(0);
  BeatsPerMinuteChanged(0);

  /* Connection State */
  connectionStateMachine = new QStateMachine(this);
  connectingState = new QState(connectionStateMachine);
  connectedState = new QState(connectionStateMachine);
  disconnectedState = new QState(connectionStateMachine);

  /* Jammr has a PRIVS chat message that reports user privileges */
  if (jammrApiUrl.isEmpty()) {
    connectingState->assignProperty(voteMenu, "enabled", true);
  }
  connectingState->assignProperty(connectAction, "enabled", false);
  connectingState->assignProperty(disconnectAction, "enabled", true);
  connectingState->assignProperty(audioConfigAction, "enabled", false);
  disconnectedState->assignProperty(voteMenu, "enabled", false);
  disconnectedState->assignProperty(connectAction, "enabled", true);
  disconnectedState->assignProperty(disconnectAction, "enabled", false);
  disconnectedState->assignProperty(audioConfigAction, "enabled", true);
  disconnectedState->assignProperty(adminMenu, "enabled", false);
  disconnectedState->assignProperty(adminTopicAction, "enabled", false);
  disconnectedState->assignProperty(adminBPMAction, "enabled", false);
  disconnectedState->assignProperty(adminBPIAction, "enabled", false);
  disconnectedState->assignProperty(adminAccessControlAction,
                                    "enabled", false);
  disconnectedState->assignProperty(kickMenu, "enabled", false);

  disconnectedState->addTransition(this, SIGNAL(Connecting()), connectingState);
  connectingState->addTransition(this, SIGNAL(Connected()), connectedState);
  connectingState->addTransition(this, SIGNAL(Disconnected()), disconnectedState);
  connectedState->addTransition(this, SIGNAL(Disconnected()), disconnectedState);

  connectionStateMachine->setInitialState(disconnectedState);
  connectionStateMachine->setErrorState(disconnectedState);
  connectionStateMachine->start();

  reconnectTimer = new QTimer(this);
  reconnectTimer->setSingleShot(true);
  connect(reconnectTimer, SIGNAL(timeout()), this, SLOT(Reconnect()));
  resetReconnect();

  connect(&client, SIGNAL(userInfoChanged()),
          this, SLOT(UserInfoChanged()));
  connect(&client, SIGNAL(statusChanged(int)),
          this, SLOT(ClientStatusChanged(int)));
  connect(&client, SIGNAL(beatsPerMinuteChanged(int)),
          this, SLOT(BeatsPerMinuteChanged(int)));
  connect(&client, SIGNAL(beatsPerIntervalChanged(int)),
          this, SLOT(BeatsPerIntervalChanged(int)));
  connect(&client, SIGNAL(beatsPerIntervalChanged(int)),
          metronomeBar, SLOT(setBeatsPerInterval(int)));
  connect(&client, SIGNAL(currentBeatChanged(int)),
          metronomeBar, SLOT(setCurrentBeat(int)));

  vstProcessor = new VSTProcessor(this);
  vstConfigDialog = new VSTConfigDialog(vstProcessor, this);

  QTimer::singleShot(0, this, SLOT(Startup()));
}

MainWindow::~MainWindow()
{
  Disconnect();

  settings->setValue("main/enableXmit", xmitButton->isChecked());
  settings->setValue("main/enableMetronome", metronomeButton->isChecked());
}

void MainWindow::setupStatusBar()
{
  bool enableXmit = settings->value("main/enableXmit", true).toBool();
  bool enableMetronome = settings->value("main/enableMetronome", true).toBool();

  xmitButton = new QToolButton(this);
  xmitButton->setText("Send");
  xmitButton->setCheckable(true);
  xmitButton->setToolTip(tr("Send audio to other users"));
  connect(xmitButton, SIGNAL(toggled(bool)),
          this, SLOT(XmitToggled(bool)));
  xmitButton->setChecked(enableXmit);
  statusBar()->addPermanentWidget(xmitButton);

  metronomeButton = new QToolButton(this);
  metronomeButton->setText("Metronome");
  metronomeButton->setCheckable(true);
  metronomeButton->setToolTip(tr("Enable metronome"));
  connect(metronomeButton, SIGNAL(toggled(bool)),
          this, SLOT(MetronomeToggled(bool)));
  metronomeButton->setChecked(enableMetronome);
  statusBar()->addPermanentWidget(metronomeButton);

  bpmLabel = new QLabel(this);
  bpmLabel->setToolTip(tr("Beats per minute (tempo)"));
  bpmLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  statusBar()->addPermanentWidget(bpmLabel);

  bpiLabel = new QLabel(this);
  bpiLabel->setToolTip(tr("Beats per interval"));
  bpiLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  statusBar()->addPermanentWidget(bpiLabel);
}

/* Idle processing once event loop has started */
void MainWindow::Startup()
{
  /* Maybe we will show a "What's new?" dialog after update in the future */
  settings->setValue("app/lastLaunchVersion", VERSION);

  /* Pop up audio configuration dialog, if necessary */
  if (!settings->contains("audio/hostAPI") ||
      !settings->contains("audio/inputDevice") ||
      !settings->contains("audio/outputDevice") ||
      !settings->contains("audio/sampleRate") ||
      !settings->contains("audio/latency")) {
    ShowAudioConfigDialog();
  }

  ShowConnectDialog();
}

void MainWindow::Connect(const QString &host, const QString &user, const QString &pass)
{
  if (!setupWorkDir()) {
    chatOutput->addInfoMessage(tr("Unable to create work directory."));
    return;
  }

  QString hostAPI = settings->value("audio/hostAPI").toString();
  QString inputDevice = settings->value("audio/inputDevice").toString();
  bool unmuteLocalChannels = settings->value("audio/unmuteLocalChannels", true).toBool();
  QString outputDevice = settings->value("audio/outputDevice").toString();
  double sampleRate = settings->value("audio/sampleRate").toDouble();
  double latency = settings->value("audio/latency").toDouble();
  audio = create_audioStreamer_PortAudio(hostAPI.toLocal8Bit().data(),
                                         inputDevice.toLocal8Bit().data(),
                                         outputDevice.toLocal8Bit().data(),
                                         sampleRate, latency,
                                         OnSamplesTrampoline);
  if (!audio)
  {
    qCritical("create_audioStreamer_PortAudio() failed");

    QDir basedir(QDesktopServices::storageLocation(QDesktopServices::DataLocation));
    QString filename = basedir.filePath("log.txt");
    QUrl url = QUrl::fromLocalFile(filename);

    QMessageBox::critical(this, tr("Failed to start audio"),
        tr("<p>There was a problem starting audio.  Try the following "
           "steps:</p><ul><li>Check that the audio device is connected.</li>"
           "<li>Ensure no other applications are using the device.</li>"
           "<li>Check input and output devices in the Audio Configuration "
           "dialog.</li><li>Select a different Audio System in the Audio "
           "Configuration dialog.</li></ul>"
           "<p>If this problem continues please report a bug and include "
           "contents of the log file at "
           "<a href=\"%1\">%2</a>.</p>").arg(url.toString(), filename));

    ShowAudioConfigDialog();
    return;
  }

  int i, ch;
  for (i = 0; (ch = client.EnumLocalChannels(i)) != -1; i++) {
    client.SetLocalChannelMonitoring(ch, false, 0, false, 0, true, !unmuteLocalChannels, false, false);
  }

  vstProcessor->attach(&client, 0);

  setWindowTitle(tr(APPNAME " - %1").arg(host));

  client.Connect(host.toAscii().data(),
                 user.toUtf8().data(),
                 pass.toUtf8().data());
}

void MainWindow::Disconnect()
{
  resetReconnect();

  delete audio;
  audio = NULL;

  client.Disconnect();
  vstProcessor->detach();

  QString workDirPath = QString::fromUtf8(client.GetWorkDir());
  bool keepWorkDir = client.config_savelocalaudio != -1;
  client.SetWorkDir(NULL);

  if (!workDirPath.isEmpty()) {
    if (!keepWorkDir) {
      cleanupWorkDir(workDirPath);
    }
    chatOutput->addInfoMessage(tr("Disconnected"));
  }

  setWindowTitle(tr(APPNAME));

  BeatsPerMinuteChanged(0);
  BeatsPerIntervalChanged(0);
  emit Disconnected();
}

void MainWindow::Reconnect()
{
  client.Reconnect();
}

bool MainWindow::tryReconnect()
{
  if (reconnectTries == 0) {
    return false;
  }

  reconnectTries--;
  reconnectTimer->start(reconnectMilliseconds);
  reconnectMilliseconds *= 2;
  return true;
}

void MainWindow::resetReconnect()
{
  reconnectTimer->stop();
  reconnectTries = 5;
  reconnectMilliseconds = 3000;
}

bool MainWindow::setupWorkDir()
{
  QDir basedir(QDesktopServices::storageLocation(QDesktopServices::DataLocation));

  /* The app data directory might not exist, so create it */
  if (!basedir.mkpath(basedir.absolutePath())) {
    return false;
  }

  /* Filename generation uses date/time plus a unique number, if necessary */
  int i;
  for (i = 0; i < 16; i++) {
    QString filename(QDateTime::currentDateTime().toString("yyyyMMdd_hhmm"));
    if (i > 0) {
      filename += QString("_%1").arg(i);
    }
    filename += ".wahjam";

    if (basedir.mkdir(filename)) {
      client.SetWorkDir(basedir.filePath(filename).toUtf8().data());
      if (client.config_savelocalaudio != -1) {
        QDir workdir(basedir.filePath(filename));
        client.SetLogFile(workdir.filePath("clipsort.log").toUtf8().data());
      }
      return true;
    }
  }
  return false;
}

void MainWindow::cleanupWorkDir(const QString &path)
{
  QDir workDir(path);

  foreach (const QFileInfo &subdirInfo,
           workDir.entryInfoList(QDir::NoDotAndDotDot | QDir::Dirs)) {
    QDir subdir(subdirInfo.absoluteDir());

    foreach (const QString &file,
             subdir.entryList(QDir::NoDotAndDotDot | QDir::Files)) {
      subdir.remove(file);
    }
    workDir.rmdir(subdirInfo.fileName());
  }

  QString name(workDir.dirName());
  workDir.cdUp();
  workDir.rmdir(name);
}

void MainWindow::ShowNINJAMConnectDialog()
{
  const QUrl url("http://autosong.ninjam.com/serverlist.php");
  ConnectDialog connectDialog(netManager);
  QStringList hosts = settings->value("connect/hosts").toStringList();

  connectDialog.resize(600, 500);
  connectDialog.loadServerList(url);
  connectDialog.setRecentHostsList(hosts);
  connectDialog.setUser(settings->value("connect/user").toString());
  connectDialog.setIsPublicServer(settings->value("connect/public", true).toBool());

  if (connectDialog.exec() != QDialog::Accepted) {
    return;
  }

  hosts.prepend(connectDialog.host());
  hosts.removeDuplicates();
  hosts = hosts.mid(0, 16); /* limit maximum number of elements */

  settings->setValue("connect/hosts", hosts);
  settings->setValue("connect/user", connectDialog.user());
  settings->setValue("connect/public", connectDialog.isPublicServer());

  QString user = connectDialog.user();
  if (connectDialog.isPublicServer()) {
    user.prepend("anonymous:");
  }

  Connect(connectDialog.host(), user, connectDialog.pass());
}

void MainWindow::ShowJammrConnectDialog()
{
  /* Request login details if we haven't stashed them */
  if (jammrApiUrl.userName().isEmpty()) {
    QUrl registerUrl = settings->value("jammr/registerUrl", JAMMR_REGISTER_URL).toUrl();
    JammrLoginDialog loginDialog(netManager, jammrApiUrl, registerUrl);

    loginDialog.setUsername(settings->value("jammr/user").toString());

    if (loginDialog.exec() != QDialog::Accepted) {
      return;
    }

    settings->setValue("jammr/user", loginDialog.username());

    /* Stash login details into the API URL so others can use them */
    jammrApiUrl.setUserName(loginDialog.username());
    jammrApiUrl.setPassword(loginDialog.password());
    jammrAuthToken = loginDialog.token();
  }

  QUrl upgradeUrl = settings->value("jammr/upgradeUrl", JAMMR_UPGRADE_URL).toUrl();
  JammrConnectDialog connectDialog(netManager, jammrApiUrl, upgradeUrl);
  connectDialog.resize(600, 500);

  if (connectDialog.exec() != QDialog::Accepted) {
    return;
  }

  Connect(connectDialog.host(), jammrApiUrl.userName(), jammrAuthToken);
}

void MainWindow::ShowConnectDialog()
{
  if (jammrApiUrl.isEmpty()) {
    ShowNINJAMConnectDialog();
  } else {
    ShowJammrConnectDialog();
  }
}

void MainWindow::ShowAudioConfigDialog()
{
  PortAudioConfigDialog audioDialog;

  audioDialog.setHostAPI(settings->value("audio/hostAPI").toString());
  audioDialog.setInputDevice(settings->value("audio/inputDevice").toString());
  audioDialog.setUnmuteLocalChannels(settings->value("audio/unmuteLocalChannels", true).toBool());
  audioDialog.setOutputDevice(settings->value("audio/outputDevice").toString());
  audioDialog.setSampleRate(settings->value("audio/sampleRate").toDouble());
  audioDialog.setLatency(settings->value("audio/latency").toDouble());

  if (audioDialog.exec() == QDialog::Accepted) {
    settings->setValue("audio/hostAPI", audioDialog.hostAPI());
    settings->setValue("audio/inputDevice", audioDialog.inputDevice());
    settings->setValue("audio/unmuteLocalChannels", audioDialog.unmuteLocalChannels());
    settings->setValue("audio/outputDevice", audioDialog.outputDevice());
    settings->setValue("audio/sampleRate", audioDialog.sampleRate());
    settings->setValue("audio/latency", audioDialog.latency());
  }
}

void MainWindow::ShowVSTConfigDialog()
{
  vstConfigDialog->show();
}

void MainWindow::ShowAboutDialog()
{
  QMessageBox::about(this, tr("About " APPNAME),
      tr("<h1>%1 version %2</h1>"
         "<p><b>Website:</b> <a href=\"http://%3/\">http://%3/</a></p>"
         "<p><b>Git commit:</b> <a href=\"http://github.com/wahjam/wahjam/commit/%4\">%4</a></p>"
         "<p>Based on <a href=\"http://ninjam.com/\">NINJAM</a>.</p>"
         "<p>Licensed under the GNU General Public License version 2, see "
         "<a href=\"http://www.gnu.org/licenses/gpl-2.0.html\">"
         "http://www.gnu.org/licenses/gpl-2.0.html</a> for details.</p>").arg(APPNAME, VERSION, ORGDOMAIN, COMMIT_ID));
}

void MainWindow::UserInfoChanged()
{
  ChannelTreeWidget::RemoteChannelUpdater updater(channelTree);

  int useridx;
  for (useridx = 0; useridx < client.GetNumUsers(); useridx++) {
    const char *name = client.GetUserState(useridx, NULL, NULL, NULL);
    updater.addUser(useridx, QString::fromUtf8(name));

    int i = 0;
    int channelidx;
    while ((channelidx = client.EnumUserChannels(useridx, i++)) != -1) {
      bool mute;
      name = client.GetUserChannelState(useridx, channelidx, NULL, NULL, NULL, &mute, NULL);
      updater.addChannel(channelidx, QString::fromUtf8(name), mute);
    }
  }

  updater.commit();
}

void MainWindow::OnSamples(float **inbuf, int innch, float **outbuf, int outnch, int len, int srate)
{
  client.AudioProc(inbuf, innch, outbuf, outnch, len, srate);
}

bool MainWindow::LicenseCallback(const char *licensetext)
{
  QMessageBox msgBox(this);

  msgBox.setText("Please review this server license agreement.");
  msgBox.setInformativeText(licensetext);
  msgBox.setStandardButtons(QMessageBox::Cancel | QMessageBox::Ok);
  msgBox.setTextFormat(Qt::PlainText);

  return msgBox.exec() == QMessageBox::Ok;
}

void MainWindow::ClientStatusChanged(int newStatus)
{
  QString errstr = QString::fromUtf8(client.GetErrorStr());
  QString statusMessage;

  switch (newStatus) {
  case NJClient::NJC_STATUS_PRECONNECT:
  case NJClient::NJC_STATUS_AUTHENTICATING:
    return; /* ignore */

  case NJClient::NJC_STATUS_CONNECTING:
    emit Connecting();
    return;

  case NJClient::NJC_STATUS_CANTCONNECT:
    if (tryReconnect()) {
      return;
    }
    statusMessage = tr("Error: connecting failed");
    break;

  case NJClient::NJC_STATUS_VERSIONMISMATCH:
    statusMessage = tr("Error: client is outdated, please update");
    break;

  case NJClient::NJC_STATUS_INVALIDAUTH:
    statusMessage = tr("Error: authentication failed");

    /* Clear stashed Jammr REST API credentials */
    jammrApiUrl.setUserInfo("");
    jammrAuthToken.clear();
    break;

  case NJClient::NJC_STATUS_OK:
    resetReconnect();
    emit Connected();
    return;

  case NJClient::NJC_STATUS_DISCONNECTED:
    statusMessage = tr("Error: unexpected disconnect");
    break;
  }

  /* Use NJClient's error message, if available */
  if (!errstr.isEmpty()) {
    statusMessage = "Error: " + errstr;
  }

  chatOutput->addInfoMessage(statusMessage);
  Disconnect();
}

void MainWindow::BeatsPerMinuteChanged(int bpm)
{
  if (bpm > 0) {
    bpmLabel->setText(tr("BPM: %1").arg(bpm));
  } else {
    bpmLabel->setText(tr("BPM: N/A"));
  }
}

void MainWindow::BeatsPerIntervalChanged(int bpi)
{
  if (bpi > 0) {
    bpiLabel->setText(tr("BPI: %1").arg(bpi));
  } else {
    bpiLabel->setText(tr("BPI: N/A"));
  }
}

void MainWindow::ChatMessageCallback(char **charparms, int nparms)
{
  QString parms[nparms];
  int i;

  for (i = 0; i < nparms; i++) {
    if (charparms[i]) {
      parms[i] = QString::fromUtf8(charparms[i]);
    }
  }

  if (parms[0] == "TOPIC") {
    if (parms[1].isEmpty()) {
      if (parms[2].isEmpty()) {
        chatOutput->addInfoMessage(tr("No topic is set."));
      } else {
        chatOutput->addInfoMessage(tr("Topic is: %1").arg(parms[2]));
      }
    } else {
      if (parms[2].isEmpty()) {
        chatOutput->addInfoMessage(tr("%1 removes topic.").arg(parms[1]));
      } else {
        chatOutput->addInfoMessage(
        tr("%1 sets topic to: %2").arg(parms[1], parms[2]));
      }
    }

    /* TODO set topic */
  } else if (parms[0] == "MSG") {
    QRegExp re("\\[voting system\\] leading candidate: (\\d+)\\/\\d+ votes"
               " for (\\d+) (BPI|BPM) \\[each vote expires in \\d+s\\]");

    if (parms[1].isEmpty() && re.exactMatch(parms[2]) && re.cap(1) == "1") {
      // Add vote [+1] link
      QString type = re.cap(3).toLower();
      QString value = re.cap(2);
      QString href = QString("send-message:!vote %1 %2").arg(type, value);
      chatOutput->addMessage(parms[1], parms[2]);
      chatOutput->addLink(href, "[+1]");
    } else if (parms[2].startsWith("/me ")) {
      // Special formatting for action messages
      chatOutput->addActionMessage(parms[1], parms[2].mid(4));
    } else {
      chatOutput->addMessage(parms[1], parms[2]);
    }
  } else if (parms[0] == "PRIVMSG") {
    chatOutput->addPrivateMessage(parms[1], parms[2]);
  } else if (parms[0] == "JOIN") {
    chatOutput->addInfoMessage(tr("%1 has joined the server").arg(parms[1]));
  } else if (parms[0] == "PART") {
    chatOutput->addInfoMessage(tr("%1 has left the server").arg(parms[1]));
  } else if (parms[0] == "PRIVS") {
    unsigned int privs = privsFromString(parms[1]);
    voteMenu->setEnabled(privs & PRIV_VOTE);
    adminMenu->setEnabled(privs & (PRIV_TOPIC | PRIV_BPM | PRIV_KICK));
    adminTopicAction->setEnabled(privs & PRIV_TOPIC);
    adminBPMAction->setEnabled(privs & PRIV_BPM);
    adminBPIAction->setEnabled(privs & PRIV_BPM);
    adminAccessControlAction->setEnabled(!jammrApiUrl.isEmpty() &&
                                         (privs & PRIV_KICK));
    kickMenu->setEnabled(privs & PRIV_KICK);
  } else {
    chatOutput->addInfoMessage(tr("Unrecognized command:"));
    for (i = 0; i < nparms; i++) {
      chatOutput->addLine(QString("[%1]").arg(i), parms[i]);
    }
  }
}

void MainWindow::ChatLinkClicked(const QUrl &url)
{
  if (url.scheme() == "send-message") {
    SendChatMessage(url.path());
  } else if (url.scheme() == "http" || url.scheme() == "https") {
    QDesktopServices::openUrl(url);
  }
}

void MainWindow::ChatInputReturnPressed()
{
  QString line = chatInput->text();
  chatInput->clear();
  if (! line.isEmpty()) {
    SendChatMessage(line);
  }
}

void MainWindow::SendChatMessage(const QString &line)
{
  QString command, parm, msg;
  if (line.compare("/clear", Qt::CaseInsensitive) == 0) {
    chatOutput->clear();
    return;
  } else if (line.startsWith("/me ", Qt::CaseInsensitive)) {
    command = "MSG";
    parm = line;
  } else if (line.startsWith("/topic ", Qt::CaseInsensitive) ||
             line.startsWith("/kick ", Qt::CaseInsensitive) ||
             line.startsWith("/bpm ", Qt::CaseInsensitive) ||
             line.startsWith("/bpi ", Qt::CaseInsensitive)) {
    command = "ADMIN";
    parm = line.mid(1);
  } else if (line.startsWith("/admin ", Qt::CaseInsensitive)) {
    command = "ADMIN";
    parm = line.section(' ', 1, -1, QString::SectionSkipEmpty);
  } else if (line.startsWith("/msg ", Qt::CaseInsensitive)) {
    command = "PRIVMSG";
    parm = line.section(' ', 1, 1, QString::SectionSkipEmpty);
    msg = line.section(' ', 2, -1, QString::SectionSkipEmpty);
    if (msg.isEmpty()) {
      chatOutput->addErrorMessage(tr("/msg requires a username and a message."));
      return;
    }
    chatOutput->addLine(tr("-> *%1* ").arg(parm), msg);
  } else {
    command = "MSG";
    parm = line;
  }

  bool connected = client.GetStatus() == NJClient::NJC_STATUS_OK;
  if (connected) {
    if (command == "PRIVMSG") {
      client.ChatMessage_Send(command.toUtf8().data(), parm.toUtf8().data(), msg.toUtf8().data());
    } else {
      client.ChatMessage_Send(command.toUtf8().data(), parm.toUtf8().data());
    }
  }

  if (!connected) {
    chatOutput->addErrorMessage("not connected to a server.");
  }
}

void MainWindow::RemoteChannelMuteChanged(int useridx, int channelidx, bool mute)
{
  client.SetUserChannelState(useridx, channelidx, false, false, false, 0, false, 0, true, mute, false, false);
}

void MainWindow::VoteBPMDialog()
{
  bool ok;
  int bpm = QInputDialog::getInt(this, tr("Vote BPM"),
                                 tr("Tempo in beats per minute:"),
                                 120, 40, 400, 1, &ok);
  if (ok) {
    SendChatMessage(QString("!vote bpm %1").arg(bpm));
  }
}

void MainWindow::VoteBPIDialog()
{
  bool ok;
  int bpi = QInputDialog::getInt(this, tr("Vote BPI"),
                                 tr("Interval length in beats:"),
                                 16, 4, 64, 1, &ok);
  if (ok) {
    SendChatMessage(QString("!vote bpi %1").arg(bpi));
  }
}

void MainWindow::XmitToggled(bool checked)
{
  int i, ch;
  for (i = 0; (ch = client.EnumLocalChannels(i)) != -1; i++) {
    client.SetLocalChannelInfo(ch, NULL, false, 0, false, 0, true, checked);
  }
}

void MainWindow::MetronomeToggled(bool checked)
{
  client.config_metronome_mute = !checked;
}

void MainWindow::AdminTopicDialog()
{
  bool ok;
  QString newTopic = QInputDialog::getText(this, tr("Set topic"),
      tr("New topic:"), QLineEdit::Normal, "", &ok);

  if (ok && !newTopic.isEmpty()) {
    SendChatMessage(QString("/topic %1").arg(newTopic));
  }
}

void MainWindow::AdminBPMDialog()
{
  bool ok;
  int bpm = QInputDialog::getInt(this, tr("Set BPM"),
                                 tr("Tempo in beats per minute:"),
                                 120, 40, 400, 1, &ok);
  if (ok) {
    SendChatMessage(QString("/bpm %1").arg(bpm));
  }
}

void MainWindow::AdminBPIDialog()
{
  bool ok;
  int bpi = QInputDialog::getInt(this, tr("Set BPI"),
                                 tr("Interval length in beats:"),
                                 16, 4, 64, 1, &ok);
  if (ok) {
    SendChatMessage(QString("/bpi %1").arg(bpi));
  }
}

void MainWindow::AdminAccessControlDialog()
{
  JammrAccessControlDialog accessControlDialog(netManager,
      jammrApiUrl, client.GetHostName(), this);
  accessControlDialog.exec();
}

void MainWindow::KickMenuAboutToShow()
{
  kickMenu->clear();

  for (int i = 0; i < client.GetNumUsers(); i++) {
    const char *username = client.GetUserState(i);
    if (username) {
      kickMenu->addAction(username);
    }
  }
}

void MainWindow::KickMenuTriggered(QAction *action)
{
  SendChatMessage(QString("/kick %1").arg(action->text()));
}
