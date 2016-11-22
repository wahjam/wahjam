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
#include <QSysInfo>
#include <QLoggingCategory>

#include "common/njmisc.h"

static FILE *logfp;

static void logMsgHandler(QtMsgType type, const QMessageLogContext &context,
                          const QString &msg)
{
  Q_UNUSED(context);
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
  fprintf(logfp, "%s %s: %s\n", timestamp.toUtf8().data(), typestr, msg.toUtf8().data());
}

/* Called at startup */
static void logSystemInformation()
{
  qDebug("CPU architecture %d-bit %s", QSysInfo::WordSize,
         QSysInfo::ByteOrder == QSysInfo::LittleEndian ?
         "little-endian" : "big-endian");
  qDebug("OS: %s", QSysInfo::prettyProductName().toUtf8().data());
  qDebug("Qt %s", qVersion());
}

void logInit(const QString &filename)
{
  logfp = utf8_fopen(filename.toUtf8().data(), "w");
  if (!logfp) {
    logfp = stderr;
  }
#ifdef Q_OS_WIN
  /* Windows does not support line-buffering, so use no buffering */
  setvbuf(logfp, NULL, _IONBF, 0);

  /* Also unbuffer stderr when qDebug() cannot be used (non-GUI threads) */
  setvbuf(stderr, NULL, _IONBF, 0);
#else
  setvbuf(logfp, NULL, _IOLBF, 0); /* use line buffering */
#endif

  QLoggingCategory::setFilterRules(QStringLiteral("*.debug=true\nqt.*.debug=false"));
  qInstallMessageHandler(logMsgHandler);

  qDebug(APPNAME " %s (%s)", VERSION, COMMIT_ID);
  logSystemInformation();
}
