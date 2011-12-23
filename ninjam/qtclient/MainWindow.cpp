
#include "MainWindow.h"
#include "ConnectDialog.h"
#include "ClientRunThread.h"
#include "../../WDL/jnetlib/jnetlib.h"

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
  : QMainWindow(parent), audioEnabled(false)
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

  client.config_savelocalaudio = 0;
  client.LicenseAgreementCallback = LicenseCallbackTrampoline;
  client.ChatMessage_Callback = ChatMessageCallbackTrampoline;
  client.SetLocalChannelInfo(0, "channel0", true, 0, false, 0, true, true);
  client.SetLocalChannelMonitoring(0, false, 0.0f, false, 0.0f, false, false, false, false);

  /* TODO set work dir */

  ConnectDialog connectDialog;

  if (connectDialog.exec() != QDialog::Accepted) {
    return; /* TODO exit */
  }

  QString user = connectDialog.GetUser();
  if (connectDialog.IsPublicServer()) {
    user.prepend("anonymous:");
  }

  client.Connect(connectDialog.GetHost().toAscii().data(),
                 user.toUtf8().data(),
                 connectDialog.GetPass().toUtf8().data());
  audioEnabled = true;

  setWindowTitle(tr("Wahjam"));
  setCentralWidget(new QWidget);

  runThread = new ClientRunThread(&clientMutex, &client);
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

int MainWindow::LicenseCallback(char *licensetext)
{
  /* TODO */
  abort();
  return TRUE;
}

void MainWindow::ChatMessageCallback(char **parms, int nparms)
{
  /* TODO */
  abort();
}
