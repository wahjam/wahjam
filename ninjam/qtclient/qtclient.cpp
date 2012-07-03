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

#include <portaudio.h>
#include <QDesktopServices>
#include <QDir>
#include <QApplication>
#include <QMessageBox>

#include "logging.h"
#include "MainWindow.h"

QSettings *settings;

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

  /* Instantiate QSettings now that application information has been set */
  settings = new QSettings(&app);

  /* Set up log file */
  QString logFile;
  if (settings->contains("app/logFile")) {
    logFile = settings->value("app/logFile").toString();
  } else {
    QDir basedir(QDesktopServices::storageLocation(QDesktopServices::DataLocation));

    /* The app data directory might not exist, so create it */
    if (!basedir.mkpath(basedir.absolutePath())) {
      return false;
    }

    logFile = basedir.filePath("log.txt");
  }
  logInit(logFile);

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

  /* Show the connection dialog right away, except on first start when the user
   * needs to configure their audio before playing.
   */
  if (settings->contains("app/lastLaunchVersion")) {
    mainWindow.ShowConnectDialog();
  } else {
    mainWindow.ShowAudioConfigDialog();
  }
  settings->setValue("app/lastLaunchVersion", VERSION);

  return app.exec();
}
