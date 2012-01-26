#include <QMessageBox>
#include <QVBoxLayout>

#include "MainWindow.h"
#include "ClientRunThread.h"
#include "../../WDL/jnetlib/jnetlib.h"

MainWindow *MainWindow::instance; /* singleton */

void MainWindow::OnSamplesTrampoline(float **inbuf, int innch, float **outbuf, int outnch, int len, int srate)
{
  MainWindow::GetInstance()->OnSamples(inbuf, innch, outbuf, outnch, len, srate);
}

int MainWindow::LicenseCallbackTrampoline(int user32, char *licensetext)
{
  /* Bounce back into ClientRunThread */
  return MainWindow::GetInstance()->runThread->licenseCallbackTrampoline(licensetext);
}

void MainWindow::ChatMessageCallbackTrampoline(int user32, NJClient *inst, char **parms, int nparms)
{
  /* Bounce back into ClientRunThread */
  MainWindow::GetInstance()->runThread->chatMessageCallbackTrampoline(parms, nparms);
}

MainWindow *MainWindow::GetInstance()
{
  return instance;
}

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent), audio(NULL), audioEnabled(false)
{
  /* Since the ninjam callbacks do not pass a void* opaque argument we rely on
   * a global variable.
   */
  if (MainWindow::instance) {
    fprintf(stderr, "MainWindow can only be instantiated once!\n");
    abort();
  }
  MainWindow::instance = this;

  JNL::open_socketlib();

  client.config_savelocalaudio = 0;
  client.LicenseAgreementCallback = LicenseCallbackTrampoline;
  client.ChatMessage_Callback = ChatMessageCallbackTrampoline;
  client.SetLocalChannelInfo(0, "channel0", true, 0, false, 0, true, true);
  client.SetLocalChannelMonitoring(0, false, 0.0f, false, 0.0f, false, false, false, false);

  setWindowTitle(tr("Wahjam"));

  chatOutput = new QTextEdit(this);
  chatOutput->setReadOnly(true);

  chatInput = new QLineEdit(this);
  chatInput->connect(chatInput, SIGNAL(returnPressed()),
                     this, SLOT(ChatInputReturnPressed()));

  QWidget *content = new QWidget(this);
  QVBoxLayout *layout = new QVBoxLayout;
  layout->addWidget(chatOutput);
  layout->addWidget(chatInput);
  content->setLayout(layout);
  content->setTabOrder(chatInput, chatOutput);

  setCentralWidget(content);

  runThread = new ClientRunThread(&clientMutex, &client);

  /* Hook up an inter-thread signal for the license agreement dialog */
  connect(runThread, SIGNAL(licenseCallback(const char *, bool *)),
          this, SLOT(LicenseCallback(const char *, bool *)),
          Qt::BlockingQueuedConnection);

  /* Hook up an inter-thread signal for the chat message callback */
  connect(runThread, SIGNAL(chatMessageCallback(char **, int)),
          this, SLOT(ChatMessageCallback(char **, int)),
          Qt::BlockingQueuedConnection);

  runThread->start();
}

MainWindow::~MainWindow()
{
  audioEnabled = false;
  if (runThread) {
    runThread->stop();
  }
  JNL::close_socketlib();
}

void MainWindow::Connect(const QString &host, const QString &user, const QString &pass)
{
  /* TODO set work dir */

  /* TODO replace with PortAudio */
#if defined(_WIN32)
#error
#elif defined(_MAC)
#error
#else
  char device[] = "in pulse out pulse";
  audio = create_audioStreamer_ALSA(device, OnSamplesTrampoline);
#endif
  if (!audio)
  {
    printf("Error opening audio!\n");
    exit(1);
  }

  client.Connect(host.toAscii().data(),
                 user.toUtf8().data(),
                 pass.toUtf8().data());
  audioEnabled = true;
}

void MainWindow::OnSamples(float **inbuf, int innch, float **outbuf, int outnch, int len, int srate)
{
  if (!audioEnabled) {
    int x;
    // clear all output buffers
    for (x = 0; x < outnch; x ++) memset(outbuf[x],0,sizeof(float)*len);
    return;
  }
  client.AudioProc(inbuf, innch, outbuf, outnch, len, srate);
}

void MainWindow::LicenseCallback(const char *licensetext, bool *result)
{
  QMessageBox msgBox(this);

  msgBox.setText("Please review this server license agreement.");
  msgBox.setInformativeText(licensetext);
  msgBox.setStandardButtons(QMessageBox::Cancel | QMessageBox::Ok);
  msgBox.setTextFormat(Qt::PlainText);

  *result = msgBox.exec() == QMessageBox::Ok ? TRUE : FALSE;
}

/* Append line with bold formatted prefix to the chat widget */
void MainWindow::chatAddLine(const QString &prefix, const QString &content)
{
  QTextCharFormat oldFormat = chatOutput->currentCharFormat();
  QTextCharFormat boldFormat = oldFormat;
  boldFormat.setFontWeight(QFont::Bold);

  chatOutput->setCurrentCharFormat(boldFormat);
  chatOutput->append(prefix);
  chatOutput->setCurrentCharFormat(oldFormat);
  chatOutput->insertPlainText(content);
}

/* Append a message from a given source to the chat widget */
void MainWindow::chatAddMessage(const QString &src, const QString &msg)
{
  if (src.isEmpty()) {
    chatAddLine("*** ", msg);
  } else if (msg.startsWith("/me ")) {
    chatAddLine(QString("* %1 ").arg(src), msg.mid(4));
  } else {
    chatAddLine(QString("<%1> ").arg(src), msg);
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
        chatAddLine("No topic is set.", "");
      } else {
        chatAddLine(QString("Topic is: "), parms[2]);
      }
    } else {
      if (parms[2].isEmpty()) {
        chatAddLine(QString("%1 removes topic.").arg(parms[1]), "");
      } else {
        chatAddLine(QString("%1 sets topic to: ").arg(parms[1]), parms[2]);
      }
    }

    /* TODO set topic */
  } else if (parms[0] == "MSG") {
    chatAddMessage(parms[1], parms[2]);
  } else if (parms[0] == "PRIVMSG") {
    chatAddLine(QString("* %1 * ").arg(parms[1]), parms[2]);
  } else if (parms[0] == "JOIN") {
    chatAddLine(QString("%1 has joined the server").arg(parms[1]), "");
  } else if (parms[0] == "PART") {
    chatAddLine(QString("%1 has left the server").arg(parms[1]), "");
  } else {
    chatOutput->append("Unrecognized command:");
    for (i = 0; i < nparms; i++) {
      chatOutput->append(QString("[%1] %2").arg(i).arg(parms[i]));
    }
  }
}

void MainWindow::ChatInputReturnPressed()
{
  QString line = chatInput->text();
  chatInput->clear();

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
      chatAddLine("error: /msg requires a username and a message.", "");
      return;
    }
    chatAddLine(QString("-> *%1* ").arg(parm), msg);
  } else {
    command = "MSG";
    parm = line;
  }

  clientMutex.lock();
  bool connected = client.GetStatus() == NJClient::NJC_STATUS_OK;
  if (connected) {
    if (command == "PRIVMSG") {
      client.ChatMessage_Send(command.toUtf8().data(), parm.toUtf8().data(), msg.toUtf8().data());
    } else {
      client.ChatMessage_Send(command.toUtf8().data(), parm.toUtf8().data());
    }
  }
  clientMutex.unlock();

  if (!connected) {
    chatAddLine("error: not connected to a server.", "");
  }
}
