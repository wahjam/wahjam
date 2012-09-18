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
#include <QPushButton>
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
#include "PortAudioConfigDialog.h"
#include "../njmisc.h"

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

  connectAction = new QAction(tr("&Connect..."), this);
  connect(connectAction, SIGNAL(triggered()), this, SLOT(ShowConnectDialog()));

  disconnectAction = new QAction(tr("&Disconnect"), this);
  connect(disconnectAction, SIGNAL(triggered()), this, SLOT(Disconnect()));

  audioConfigAction = new QAction(tr("Configure &audio..."), this);
  connect(audioConfigAction, SIGNAL(triggered()), this, SLOT(ShowAudioConfigDialog()));

  QAction *exitAction = new QAction(tr("E&xit"), this);
  exitAction->setShortcuts(QKeySequence::Quit);
  connect(exitAction, SIGNAL(triggered()), this, SLOT(close()));

  QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
  fileMenu->addAction(connectAction);
  fileMenu->addAction(disconnectAction);
  fileMenu->addAction(audioConfigAction);
  fileMenu->addAction(exitAction);

  voteMenu = menuBar()->addMenu(tr("&Vote"));
  QAction *voteBPMAction = new QAction(tr("BPM"), this);
  QAction *voteBPIAction = new QAction(tr("BPI"), this);
  connect(voteBPMAction, SIGNAL(triggered()), this, SLOT(VoteBPMDialog()));
  connect(voteBPIAction, SIGNAL(triggered()), this, SLOT(VoteBPIDialog()));
  voteMenu->addAction(voteBPMAction);
  voteMenu->addAction(voteBPIAction);

  QAction *aboutAction = new QAction(tr("&About..."), this);
  connect(aboutAction, SIGNAL(triggered()), this, SLOT(ShowAboutDialog()));

  QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
  helpMenu->addAction(aboutAction);

  setupStatusBar();

  setWindowTitle(tr("Wahjam"));

  chatOutput = new ChatOutput(this);
  chatOutput->connect(chatOutput, SIGNAL(anchorClicked(const QUrl&)),
                      this, SLOT(ChatLinkClicked(const QUrl&)));

  chatInput = new QLineEdit(this);
  chatInput->connect(chatInput, SIGNAL(returnPressed()),
                     this, SLOT(ChatInputReturnPressed()));

  channelTree = new ChannelTreeWidget(this);
  setupChannelTree();
  connect(channelTree, SIGNAL(MetronomeMuteChanged(bool)),
          this, SLOT(MetronomeMuteChanged(bool)));
  connect(channelTree, SIGNAL(LocalChannelMuteChanged(int, bool)),
          this, SLOT(LocalChannelMuteChanged(int, bool)));
  connect(channelTree, SIGNAL(LocalChannelBroadcastChanged(int, bool)),
          this, SLOT(LocalChannelBroadcastChanged(int, bool)));
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
  connectedState = new QState(connectionStateMachine);
  disconnectedState = new QState(connectionStateMachine);

  connectedState->assignProperty(voteMenu, "enabled", true);
  connectedState->assignProperty(connectAction, "enabled", false);
  connectedState->assignProperty(disconnectAction, "enabled", true);
  connectedState->assignProperty(audioConfigAction, "enabled", false);
  disconnectedState->assignProperty(voteMenu, "enabled", false);
  disconnectedState->assignProperty(connectAction, "enabled", true);
  disconnectedState->assignProperty(disconnectAction, "enabled", false);
  disconnectedState->assignProperty(audioConfigAction, "enabled", true);

  connectedState->addTransition(this, SIGNAL(Disconnected()), disconnectedState);
  disconnectedState->addTransition(this, SIGNAL(Connected()), connectedState);

  connectionStateMachine->setInitialState(disconnectedState);
  connectionStateMachine->setErrorState(disconnectedState);
  connectionStateMachine->start();

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
}

MainWindow::~MainWindow()
{
  Disconnect();
}

/* Must be called with client mutex held or before client thread is started */
void MainWindow::setupChannelTree()
{
  int i, ch;
  for (i = 0; (ch = client.EnumLocalChannels(i)) != -1; i++) {
    bool broadcast, mute;
    const char *name = client.GetLocalChannelInfo(ch, NULL, NULL, &broadcast);
    client.GetLocalChannelMonitoring(ch, NULL, NULL, &mute, NULL);

    channelTree->addLocalChannel(ch, QString::fromUtf8(name), mute, broadcast);
  }
}

void MainWindow::setupStatusBar()
{
  bpmLabel = new QLabel(this);
  bpmLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  statusBar()->addPermanentWidget(bpmLabel);

  bpiLabel = new QLabel(this);
  bpiLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  statusBar()->addPermanentWidget(bpiLabel);
}

void MainWindow::Connect(const QString &host, const QString &user, const QString &pass)
{
  if (!setupWorkDir()) {
    chatOutput->addInfoMessage(tr("Unable to create work directory."));
    return;
  }

  QString hostAPI = settings->value("audio/hostAPI").toString();
  QString inputDevice = settings->value("audio/inputDevice").toString();
  QString outputDevice = settings->value("audio/outputDevice").toString();
  audio = create_audioStreamer_PortAudio(hostAPI.toLocal8Bit().data(),
                                         inputDevice.toLocal8Bit().data(),
                                         outputDevice.toLocal8Bit().data(),
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

  setWindowTitle(tr("Wahjam - %1").arg(host));

  client.Connect(host.toAscii().data(),
                 user.toUtf8().data(),
                 pass.toUtf8().data());
}

void MainWindow::Disconnect()
{
  delete audio;
  audio = NULL;

  client.Disconnect();
  QString workDirPath = QString::fromUtf8(client.GetWorkDir());
  bool keepWorkDir = client.config_savelocalaudio != -1;
  client.SetWorkDir(NULL);

  if (!workDirPath.isEmpty()) {
    if (!keepWorkDir) {
      cleanupWorkDir(workDirPath);
    }
    chatOutput->addInfoMessage(tr("Disconnected"));
  }

  setWindowTitle(tr("Wahjam"));

  BeatsPerMinuteChanged(0);
  BeatsPerIntervalChanged(0);
  emit Disconnected();
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

void MainWindow::ShowConnectDialog()
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

void MainWindow::ShowAudioConfigDialog()
{
  PortAudioConfigDialog audioDialog;

  audioDialog.setHostAPI(settings->value("audio/hostAPI").toString());
  audioDialog.setInputDevice(settings->value("audio/inputDevice").toString());
  audioDialog.setOutputDevice(settings->value("audio/outputDevice").toString());

  if (audioDialog.exec() == QDialog::Accepted) {
    settings->setValue("audio/hostAPI", audioDialog.hostAPI());
    settings->setValue("audio/inputDevice", audioDialog.inputDevice());
    settings->setValue("audio/outputDevice", audioDialog.outputDevice());
  }
}

void MainWindow::ShowAboutDialog()
{
  QMessageBox::about(this, tr("About Wahjam"),
      tr("<h1>Wahjam version %1</h1>"
         "<p><b>Website:</b> <a href=\"http://wahjam.org/\">http://wahjam.org/</a></p>"
         "<p><b>Git commit:</b> <a href=\"http://github.com/wahjam/wahjam/commit/%2\">%2</a></p>"
         "<p>Based on <a href=\"http://ninjam.com/\">NINJAM</a>.</p>"
         "<p>Licensed under the GNU General Public License version 2, see "
         "<a href=\"http://www.gnu.org/licenses/gpl-2.0.html\">"
         "http://www.gnu.org/licenses/gpl-2.0.html</a> for details.</p>").arg(VERSION, COMMIT_ID));
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

  if (newStatus == NJClient::NJC_STATUS_OK) {
    QString host = QString::fromUtf8(client.GetHostName());
    QString username = QString::fromUtf8(client.GetUserName());

    statusMessage = tr("Connected to %1 as %2").arg(host, username);
    emit Connected();
  } else if (!errstr.isEmpty()) {
    statusMessage = "Error: " + errstr;
  } else if (newStatus == NJClient::NJC_STATUS_DISCONNECTED) {
    statusMessage = tr("Error: unexpected disconnect");
  } else if (newStatus == NJClient::NJC_STATUS_INVALIDAUTH) {
    statusMessage = tr("Error: authentication failed");
  } else if (newStatus == NJClient::NJC_STATUS_CANTCONNECT) {
    statusMessage = tr("Error: connecting failed");
  }

  chatOutput->addInfoMessage(statusMessage);

  if (newStatus < 0) {
    Disconnect();
  }
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

void MainWindow::MetronomeMuteChanged(bool mute)
{
  client.config_metronome_mute = mute;
}

void MainWindow::LocalChannelMuteChanged(int ch, bool mute)
{
  client.SetLocalChannelMonitoring(ch, false, 0, false, 0, true, mute, false, false);
}

void MainWindow::LocalChannelBroadcastChanged(int ch, bool broadcast)
{
  client.SetLocalChannelInfo(ch, NULL, false, 0, false, 0, true, broadcast);
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

