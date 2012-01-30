#include <portaudio.h>
#include <QApplication>
#include <QMessageBox>

#include "MainWindow.h"

static void portAudioCleanup()
{
  Pa_Terminate();
}

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);

  /* These are used by QSettings persistent settings */
  app.setOrganizationName("Wahjam Project");
  app.setOrganizationDomain("wahjam.org");
  app.setApplicationName("Wahjam");

  /* Initialize PortAudio once for the whole application */
  PaError error = Pa_Initialize();
  if (error != paNoError) {
    QMessageBox::critical(NULL, QObject::tr("Unable to initialize PortAudio"),
                          QString::fromLocal8Bit(Pa_GetErrorText(error)));
    return 0;
  }
  atexit(portAudioCleanup);

  MainWindow mainWindow;
  mainWindow.show();
  mainWindow.ShowConnectDialog();
  return app.exec();
}
