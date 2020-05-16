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
#include <QMenu>
#include <QStatusBar>
#include <QDesktopServices>
#include <QUrl>
#include <QInputDialog>
#include <QRegExp>
#include <QSslConfiguration>
#include <qt5keychain/keychain.h>

#include "MainWindow.h"
#include "ConnectDialog.h"
#include "JammrConnectDialog.h"
#include "JammrLoginDialog.h"
#include "JammrAccessControlDialog.h"
#include "JammrUpdateChecker.h"
#include "PortAudioSettingsPage.h"
#include "EffectProcessor.h"
#include "EffectSettingsPage.h"
#include "UISettingsPage.h"
#include "PortAudioStreamer.h"
#include "screensleep.h"
#include "common/njmisc.h"
#include "common/UserPrivs.h"

static MainWindow *mainWindow;

void MainWindow::OnSamplesTrampoline(float **inbuf, int innch, float **outbuf, int outnch, int len)
{
  mainWindow->OnSamples(inbuf, innch, outbuf, outnch, len);
}

int MainWindow::LicenseCallbackTrampoline(int user32, char *licensetext)
{
  Q_UNUSED(user32);
  return mainWindow->LicenseCallback(licensetext);
}

void MainWindow::ChatMessageCallbackTrampoline(int user32, NJClient *inst, char **parms, int nparms)
{
  Q_UNUSED(user32);
  Q_UNUSED(inst);
  mainWindow->ChatMessageCallback(parms, nparms);
}

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent), portAudioStreamer(OnSamplesTrampoline),
    globalMenuBar(NULL)
{
  /* Since the ninjam callbacks do not pass a void* opaque argument we rely on
   * a global variable.
   */
  if (mainWindow) {
    qFatal("MainWindow can only be instantiated once!");
    abort();
  }
  mainWindow = this;

  client.LicenseAgreementCallback = LicenseCallbackTrampoline;
  client.ChatMessage_Callback = ChatMessageCallbackTrampoline;
  client.SetLocalChannelInfo(0, "channel0", true, 0, false, 0, true, true);
  client.SetLocalChannelMonitoring(0, false, 0.0f, false, 0.0f, false, false, false, false);
  client.SetMidiStreamer(&portMidiStreamer);

  /* Certificate verification can be disabled for local testing */
  if (!settings->value("ssl/verify", true).toBool()) {
    QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::QueryPeer);
    QSslConfiguration::setDefaultConfiguration(sslConfig);
  }

  netManager = new QNetworkAccessManager(this);

  jammrApiUrl = settings->value("jammr/apiUrl", JAMMR_API_URL).toUrl();
  if (!jammrApiUrl.isEmpty()) {
    client.SetProtocol(JAM_PROTO_JAMMR);

    JammrUpdateChecker *updateChecker = new JammrUpdateChecker(this, netManager);
    updateChecker->setUpdateUrl(settings->value("jammr/updateUrl", JAMMR_UPDATE_URL).toUrl());
    updateChecker->setDownloadUrl(settings->value("jammr/downloadUrl", JAMMR_DOWNLOAD_URL).toUrl());
    updateChecker->start();
  }

  settingsDialog = new SettingsDialog(this);
  connect(settingsDialog, SIGNAL(rejected()),
          this, SLOT(SettingsDialogClosed()));
  setupPortAudioSettingsPage();
  setupPortMidiSettingsPage();

#ifdef Q_OS_MAC
  /* Mac applications use a global menu not associated with a particular window */
  QMenuBar *theMenuBar = globalMenuBar = new QMenuBar;
#else
  QMenuBar *theMenuBar = menuBar();
#endif

  QMenu *fileMenu = theMenuBar->addMenu(tr("&File"));
  connectAction = fileMenu->addAction(tr("&Connect..."));
  connect(connectAction, SIGNAL(triggered()), this, SLOT(ShowConnectDialog()));
  disconnectAction = fileMenu->addAction(tr("&Disconnect"));
  connect(disconnectAction, SIGNAL(triggered()), this, SLOT(Disconnect()));

  QAction *settingsAction = fileMenu->addAction(tr("&Settings..."));
  settingsAction->setMenuRole(QAction::PreferencesRole);
  connect(settingsAction, SIGNAL(triggered()), settingsDialog, SLOT(show()));

#ifndef Q_OS_MAC
  /* Skip this on Mac since a Quit menu item is implicitly created */
  QAction *exitAction = fileMenu->addAction(tr("E&xit"));
  exitAction->setMenuRole(QAction::QuitRole);
  exitAction->setShortcuts(QKeySequence::Quit);
  connect(exitAction, SIGNAL(triggered()), this, SLOT(close()));
#endif

  voteMenu = theMenuBar->addMenu(tr("&Vote"));
  QAction *voteBPMAction = voteMenu->addAction(tr("BPM"));
  QAction *voteBPIAction = voteMenu->addAction(tr("BPI"));
  connect(voteBPMAction, SIGNAL(triggered()), this, SLOT(VoteBPMDialog()));
  connect(voteBPIAction, SIGNAL(triggered()), this, SLOT(VoteBPIDialog()));

  adminMenu = theMenuBar->addMenu(tr("&Admin"));
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

  QMenu *helpMenu = theMenuBar->addMenu(tr("&Help"));
  QAction *logAction = helpMenu->addAction(tr("Show &log"));
  connect(logAction, SIGNAL(triggered()), this, SLOT(ShowLog()));
#ifndef Q_OS_MAC
  /* Skip on Mac since the About menu item goes into a different menu */
  helpMenu->addSeparator();
#endif
  QAction *aboutAction = helpMenu->addAction(tr("&About..."));
  aboutAction->setMenuRole(QAction::AboutRole);
  connect(aboutAction, SIGNAL(triggered()), this, SLOT(ShowAboutDialog()));

  setWindowTitle(tr(APPNAME));

  chatOutput = new ChatOutput(this);
  chatOutput->connect(chatOutput, SIGNAL(anchorClicked(const QUrl&)),
                      this, SLOT(ChatLinkClicked(const QUrl&)));

  chatInput = new QLineEdit(this);
  chatInput->setPlaceholderText("Enter your chat message here...");
  chatInput->connect(chatInput, SIGNAL(returnPressed()),
                     this, SLOT(ChatInputReturnPressed()));
  defaultChatInputFontSize = chatInput->font().pointSize();

  channelTree = new ChannelTreeWidget(this);
  connect(channelTree, SIGNAL(RemoteChannelMuteChanged(int, int, bool)),
          this, SLOT(RemoteChannelMuteChanged(int, int, bool)));

  metronomeBar = new MetronomeBar(this);
  connect(this, SIGNAL(Disconnected()),
          metronomeBar, SLOT(reset()));

  splitter = new QSplitter(this);
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
  setupStatusBar();

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
  connectingState->assignProperty(portAudioSettingsPage, "locked", true);
  connectingState->assignProperty(portMidiSettingsPage, "locked", true);
  disconnectedState->assignProperty(voteMenu, "enabled", false);
  disconnectedState->assignProperty(connectAction, "enabled", true);
  disconnectedState->assignProperty(disconnectAction, "enabled", false);
  disconnectedState->assignProperty(portAudioSettingsPage, "locked", false);
  disconnectedState->assignProperty(portMidiSettingsPage, "locked", false);
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

  connect(&portAudioStreamer, SIGNAL(StoppedUnexpectedly()),
          this, SLOT(AudioStoppedUnexpectedly()));

  effectProcessor = new EffectProcessor(&portMidiStreamer, this);
  settingsDialog->addPage(tr("Effect plugins"),
                          new EffectSettingsPage(effectProcessor));
  setupUISettingsPage();

  restoreGeometry(settings->value("main/geometry").toByteArray());
  restoreState(settings->value("main/windowState").toByteArray());
  splitter->restoreState(settings->value("main/splitterState").toByteArray());

  QTimer::singleShot(0, this, SLOT(Startup()));
}

MainWindow::~MainWindow()
{
  Disconnect();

  settings->setValue("main/enableXmit", xmitButton->isChecked());
  settings->setValue("main/enableMetronome", metronomeButton->isChecked());

  delete globalMenuBar;

  mainWindow = NULL;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
  settings->setValue("main/geometry", saveGeometry());
  settings->setValue("main/windowState", saveState());
  settings->setValue("main/splitterState", splitter->saveState());
  QMainWindow::closeEvent(event);
}

void MainWindow::setupStatusBar()
{
  bool enableXmit = settings->value("main/enableXmit", true).toBool();
  bool enableMetronome = settings->value("main/enableMetronome", true).toBool();

  xmitButton = new QToolButton(this);
  xmitButton->setText("Send");
  xmitButton->setCheckable(true);
  xmitButton->setToolTip(tr("Send audio to other users"));
  xmitButton->setChecked(enableXmit);
  XmitToggled(enableXmit);
  connect(xmitButton, SIGNAL(toggled(bool)),
          this, SLOT(XmitToggled(bool)));
  statusBar()->addPermanentWidget(xmitButton);

  metronomeButton = new QToolButton(this);
  metronomeButton->setText("Metronome");
  metronomeButton->setCheckable(true);
  metronomeButton->setToolTip(tr("Enable metronome"));
  metronomeButton->setChecked(enableMetronome);
  MetronomeToggled(enableMetronome);
  connect(metronomeButton, SIGNAL(toggled(bool)),
          this, SLOT(MetronomeToggled(bool)));
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

void MainWindow::setupPortAudioSettingsPage()
{
  portAudioSettingsPage = new PortAudioSettingsPage;
  connect(portAudioSettingsPage, SIGNAL(unlockRequest()),
          this, SLOT(Disconnect()));

  portAudioSettingsPage->setHostAPI(settings->value("audio/hostAPI").toString());
  portAudioSettingsPage->setInputDevice(settings->value("audio/inputDevice").toString());
  portAudioSettingsPage->setUnmuteLocalChannels(settings->value("audio/unmuteLocalChannels", true).toBool());
  if (settings->contains("audio/inputChannels")) {
    portAudioSettingsPage->setInputChannels(settings->value("audio/inputChannels").toList());
  } else {
    settings->setValue("audio/inputChannels", portAudioSettingsPage->inputChannels());
  }
  portAudioSettingsPage->setOutputDevice(settings->value("audio/outputDevice").toString());
  if (settings->contains("audio/outputChannels")) {
    portAudioSettingsPage->setOutputChannels(settings->value("audio/outputChannels").toList());
  } else {
    settings->setValue("audio/outputChannels", portAudioSettingsPage->outputChannels());
  }
  portAudioSettingsPage->setSampleRate(settings->value("audio/sampleRate").toDouble());
  portAudioSettingsPage->setLatency(settings->value("audio/latency").toDouble());

  settingsDialog->addPage(tr("Audio"), portAudioSettingsPage);
}

void MainWindow::setupPortMidiSettingsPage()
{
  portMidiSettingsPage = new PortMidiSettingsPage;
  connect(portMidiSettingsPage, SIGNAL(unlockRequest()),
          this, SLOT(Disconnect()));

  portMidiSettingsPage->setInputDevice(settings->value("midi/inputDevice").toString());
  portMidiSettingsPage->setOutputDevice(settings->value("midi/outputDevice").toString());
  portMidiSettingsPage->setSendMidiBeatClock(settings->value("midi/sendMidiBeatClock", false).toBool());

  settingsDialog->addPage(tr("MIDI"), portMidiSettingsPage);
}

void MainWindow::setupUISettingsPage()
{
  int chatFontSize = settings->value("ui/chatFontSize").toInt();

  uiSettingsPage = new UISettingsPage;
  uiSettingsPage->setChatFontSize(chatFontSize);

  settingsDialog->addPage(tr("User interface"), uiSettingsPage);

  updateChatFontSize(chatFontSize);
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
    settingsDialog->exec();
  }

  ShowConnectDialog();
}

void MainWindow::AudioStoppedUnexpectedly()
{
  Disconnect();

  QMessageBox::critical(this, tr("Audio stopped unexpectedly"),
      tr("<p>An audio error has occurred. This may indicate an incompatibility "
         "with the input or output audio device."
#ifndef Q_OS_MAC /* macOS only has only one audio system */
         " Try selecting a different Audio System."
#endif
         "<p>If this problem continues please report a bug and include "
         "contents of the log file at <a href=\"%1\">%2</a>.</p>").arg(
             QUrl::fromLocalFile(logFilePath).toString(),
             logFilePath));

  settingsDialog->setPage(0);
  settingsDialog->exec();
}

void MainWindow::Connect(const QString &host, const QString &user, const QString &pass)
{
  QString midiInputDevice = settings->value("midi/inputDevice").toString();
  QString midiOutputDevice = settings->value("midi/outputDevice").toString();
  client.SetSendMidiBeatClock(settings->value("midi/sendMidiBeatClock").toBool());

  QString hostAPI = settings->value("audio/hostAPI").toString();
  QString inputDevice = settings->value("audio/inputDevice").toString();
  bool unmuteLocalChannels = settings->value("audio/unmuteLocalChannels", true).toBool();
  QList<QVariant> inputChannels = settings->value("audio/inputChannels").toList();
  QString outputDevice = settings->value("audio/outputDevice").toString();
  QList<QVariant> outputChannels = settings->value("audio/outputChannels").toList();
  double sampleRate = settings->value("audio/sampleRate").toDouble();
  double latency = settings->value("audio/latency").toDouble();

  client.SetSampleRate(sampleRate);

  portMidiStreamer.start(midiInputDevice, midiOutputDevice, latency * 1000);

  if (!portAudioStreamer.Start(hostAPI.toUtf8().data(),
                               inputDevice.toUtf8().data(),
                               inputChannels,
                               outputDevice.toUtf8().data(),
                               outputChannels,
                               sampleRate, latency)) {
    qCritical("Failed to start PortAudio");

    portMidiStreamer.stop();

    QMessageBox::critical(this, tr("Failed to start audio"),
        tr("<p>There was a problem starting audio.  Try the following "
           "steps:</p><ul><li>Check that the audio device is connected.</li>"
           "<li>Ensure no other applications are using the device.</li>"
           "<li>Check input and output devices in the Audio Configuration "
           "dialog.</li><li>Select a different Audio System in the Audio "
           "Configuration dialog.</li></ul>"
           "<p>If this problem continues please report a bug and include "
           "contents of the log file at "
           "<a href=\"%1\">%2</a>.</p>").arg(
             QUrl::fromLocalFile(logFilePath).toString(),
             logFilePath));

    settingsDialog->setPage(0);
    settingsDialog->exec();
    return;
  }

  int i, ch;
  for (i = 0; (ch = client.EnumLocalChannels(i)) != -1; i++) {
    client.SetLocalChannelMonitoring(ch, false, 0, false, 0, true, !unmuteLocalChannels, false, false);
  }

  effectProcessor->attach(&client, 0);

  setWindowTitle(tr(APPNAME " - %1").arg(host));

  screenPreventSleep();

  client.Connect(host.toLatin1().data(),
                 user.toUtf8().data(),
                 pass.toUtf8().data());
}

void MainWindow::Disconnect()
{
  resetReconnect();

  portAudioStreamer.Stop();
  client.Disconnect();
  effectProcessor->detach();
  portMidiStreamer.stop();

  chatOutput->addInfoMessage(tr("Disconnected"));

  screenAllowSleep();
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

static QString keychainGetPassword()
{
  QKeychain::ReadPasswordJob job(ORGDOMAIN);
  QEventLoop loop;

  job.setAutoDelete(false);
  job.connect(&job, SIGNAL(finished(QKeychain::Job*)), &loop, SLOT(quit()));
  job.setKey("password");
  job.start();
  loop.exec();

  return job.textData();
}

static void keychainSetPassword(const QString &password)
{
  QKeychain::WritePasswordJob job(ORGDOMAIN);
  QEventLoop loop;

  job.setAutoDelete(false);
  job.connect(&job, SIGNAL(finished(QKeychain::Job*)), &loop, SLOT(quit()));
  job.setKey("password");
  job.setTextData(password);
  job.start();
  loop.exec();
}

static void keychainDeletePassword()
{
  QKeychain::DeletePasswordJob job(ORGDOMAIN);
  QEventLoop loop;

  job.setAutoDelete(false);
  job.connect(&job, SIGNAL(finished(QKeychain::Job*)), &loop, SLOT(quit()));
  job.setKey("password");
  job.start();
  loop.exec();
}

void MainWindow::ShowNINJAMConnectDialog()
{
  const QUrl url("http://autosong.ninjam.com/serverlist.php");
  ConnectDialog connectDialog(netManager, this);
  QStringList hosts = settings->value("connect/hosts").toStringList();

  connectDialog.resize(600, 500);
  connectDialog.loadServerList(url);
  connectDialog.setRecentHostsList(hosts);
  connectDialog.setUser(settings->value("connect/user").toString());
  connectDialog.setIsPublicServer(settings->value("connect/public", true).toBool());
  connectDialog.setRememberPassword(settings->value("connect/rememberPassword", true).toBool());

  bool passwordSaved = settings->value("connect/passwordSaved", false).toBool();

  if (connectDialog.rememberPassword() && passwordSaved) {
    connectDialog.setPass(keychainGetPassword());
  }

  if (connectDialog.exec() != QDialog::Accepted) {
    return;
  }

  hosts.prepend(connectDialog.host());
  hosts.removeDuplicates();
  hosts = hosts.mid(0, 16); /* limit maximum number of elements */

  settings->setValue("connect/hosts", hosts);
  settings->setValue("connect/user", connectDialog.user());
  settings->setValue("connect/public", connectDialog.isPublicServer());

  if (connectDialog.rememberPassword()) {
    keychainSetPassword(connectDialog.pass());
    settings->setValue("connect/passwordSaved", true);
  } else if (passwordSaved) {
    /* If it was previously saved, delete it now */
    keychainDeletePassword();
    settings->setValue("connect/passwordSaved", false);
  }

  settings->setValue("connect/rememberPassword", connectDialog.rememberPassword());

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
    QUrl resetPasswordUrl = settings->value("jammr/resetPasswordUrl", JAMMR_RESET_PASSWORD_URL).toUrl();
    JammrLoginDialog loginDialog(netManager, jammrApiUrl, registerUrl, resetPasswordUrl, this);

    loginDialog.setUsername(settings->value("jammr/user").toString());
    loginDialog.setRememberPassword(settings->value("jammr/rememberPassword", true).toBool());

    bool passwordSaved = settings->value("jammr/passwordSaved", false).toBool();

    if (loginDialog.rememberPassword() && passwordSaved) {
      loginDialog.setPassword(keychainGetPassword());
    }

    if (loginDialog.exec() != QDialog::Accepted) {
      return;
    }

    settings->setValue("jammr/user", loginDialog.username());

    if (loginDialog.rememberPassword()) {
      keychainSetPassword(loginDialog.password());
      settings->setValue("jammr/passwordSaved", true);
    } else if (passwordSaved) {
      /* If it was previously saved, delete it now */
      keychainDeletePassword();
      settings->setValue("jammr/passwordSaved", false);
    }

    settings->setValue("jammr/rememberPassword", loginDialog.rememberPassword());

    /* Stash login details into the API URL so others can use them */
    jammrApiUrl.setUserName(loginDialog.username());
    jammrApiUrl.setPassword(loginDialog.password());
    jammrAuthToken = loginDialog.token();
  }

  QUrl upgradeUrl = settings->value("jammr/upgradeUrl", JAMMR_UPGRADE_URL).toUrl();
  JammrConnectDialog connectDialog(netManager, jammrApiUrl, upgradeUrl, this);
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

void MainWindow::ShowLog()
{
  QDesktopServices::openUrl(QUrl::fromLocalFile(logFilePath));
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
}

void MainWindow::OnSamples(float **inbuf, int innch, float **outbuf, int outnch, int len)
{
  client.AudioProc(inbuf, innch, outbuf, outnch, len);
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
    chatOutput->addBannerMessage(
        "Welcome!  Tips for successful jams:\n"
        "1. Set length of chord progression (in beats) with '!vote bpi NUMBER'\n"
        "2. Set tempo with '!vote bpm NUMBER' and enable Metronome button if no drums\n"
        "3. Take turns soloing.  For example 1 minute per person."
    );
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
  QString *parms;
  int i;

  parms = new QString[nparms];

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

  delete [] parms;
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
  metronomeBar->setVisible(checked);
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

void MainWindow::SettingsDialogClosed()
{
  /* Save audio settings */
  settings->setValue("audio/hostAPI", portAudioSettingsPage->hostAPI());
  settings->setValue("audio/inputDevice", portAudioSettingsPage->inputDevice());
  settings->setValue("audio/unmuteLocalChannels", portAudioSettingsPage->unmuteLocalChannels());
  settings->setValue("audio/inputChannels", portAudioSettingsPage->inputChannels());
  settings->setValue("audio/outputDevice", portAudioSettingsPage->outputDevice());
  settings->setValue("audio/outputChannels", portAudioSettingsPage->outputChannels());
  settings->setValue("audio/sampleRate", portAudioSettingsPage->sampleRate());
  settings->setValue("audio/latency", portAudioSettingsPage->latency());

  /* Save MIDI settings */
  settings->setValue("midi/inputDevice", portMidiSettingsPage->inputDevice());
  settings->setValue("midi/outputDevice", portMidiSettingsPage->outputDevice());
  settings->setValue("midi/sendMidiBeatClock", portMidiSettingsPage->sendMidiBeatClock());

  /* Save UI settings */
  int chatFontSize = uiSettingsPage->chatFontSize();
  settings->setValue("ui/chatFontSize", chatFontSize);
  updateChatFontSize(chatFontSize);
}

void MainWindow::updateChatFontSize(int size)
{
  int chatInputFontSize = size;

  if (size == 0) {
    chatInputFontSize = defaultChatInputFontSize;
  }

  QFont font(chatInput->font());
  font.setPointSize(chatInputFontSize);
  chatInput->setFont(font);

  chatOutput->setFontSize(size);

  channelTree->setFontSize(size);
}
