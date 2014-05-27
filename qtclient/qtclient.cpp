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

#include <QStandardPaths>
#include <QDir>
#include <QApplication>
#include <QMessageBox>
#include <QTextCodec>

#include "logging.h"
#include "common/audiostream.h"
#include "PortMidiStreamer.h"
#include "MainWindow.h"

QSettings *settings;
QString logFilePath;

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);

  /* Workaround for Qt5 Windows builds that cannot find their default codec.
   * See http://qt-project.org/forums/viewthread/24896/P15/#121196
   */
  if (!QTextCodec::codecForLocale()) {
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
  }

  /* These are used by QSettings persistent settings */
  app.setOrganizationName(ORGNAME);
  app.setOrganizationDomain(ORGDOMAIN);
  app.setApplicationName(APPNAME);

  /* Instantiate QSettings now that application information has been set */
  settings = new QSettings(&app);

  /* Set up log file */
  if (settings->contains("app/logFile")) {
    logFilePath = settings->value("app/logFile").toString();
  } else {
    QDir basedir(QStandardPaths::writableLocation(QStandardPaths::DataLocation));

    /* The app data directory might not exist, so create it */
    if (!basedir.mkpath(basedir.absolutePath())) {
      return false;
    }

    logFilePath = basedir.filePath("log.txt");
  }
  logInit(logFilePath);

  /* Initialize PortAudio once for the whole application */
  if (!portAudioInit()) {
    QMessageBox::critical(NULL, QObject::tr("Unable to initialize PortAudio"),
                          QObject::tr("Audio could not be initialized, "
                                      "please report this bug."));
    return 1;
  }

  portMidiInit(); /* ignore errors since this is not critical */

  MainWindow mainWindow;
  mainWindow.show();

  return app.exec();
}
