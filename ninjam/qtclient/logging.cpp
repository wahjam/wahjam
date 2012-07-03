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

#include <stdio.h>
#include <QDateTime>

static FILE *logfp;

static void logMsgHandler(QtMsgType type, const char *msg)
{
  Q_ASSERT(logfp != NULL);

  const char *typestr;
  switch (type) {
  case QtDebugMsg:
    typestr = "DEBUG";
    break;
  case QtWarningMsg:
    typestr = "WARN";
    break;
  case QtCriticalMsg:
    typestr = "CRIT";
    break;
  case QtFatalMsg:
    typestr = "FATAL";
    break;
  default:
    typestr = "???";
    break;
  }

  QString timestamp(QDateTime::currentDateTime().toUTC().toString("MMM dd yyyy hh:mm:ss"));
  fprintf(logfp, "%s %s: %s\n", timestamp.toUtf8().data(), typestr, msg);
}

void logInit(const QString &filename)
{
  logfp = fopen(filename.toUtf8().data(), "w");
  if (!logfp) {
    logfp = stderr;
  }
  setbuf(logfp, NULL);

  qInstallMsgHandler(logMsgHandler);

  qDebug("Wahjam %s (%s)", VERSION, COMMIT_ID);
}
