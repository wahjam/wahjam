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

#include "logging.h"

static FILE *logfp;

/* Called by Qt's qDebug(), qWarning(), qCritical(), and qFatal() */
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

  if (logfp == stdout) {
    /* Standard output may be used with process monitoring daemons that add
     * their own timestamps, so just print the type and message. */
    fprintf(logfp, "%s: %s\n", typestr, msg.toUtf8().data());
  } else {
    QString timestamp(QDateTime::currentDateTime().toUTC().toString("MMM dd yyyy hh:mm:ss"));
    fprintf(logfp, "%s %s: %s\n", timestamp.toUtf8().data(), typestr, msg.toUtf8().data());
  }
}

void logInit(const QString &filename)
{
  Q_ASSERT(logfp == NULL);

  if (!filename.isEmpty() && filename != "-")
  {
    logfp = fopen(filename.toUtf8().data(), "at");
    if (logfp) {
      fprintf(logfp, "Wahjam Server " VERSION " (" COMMIT_ID ") built on " __DATE__ " at " __TIME__ "\n");
    } else {
      printf("Error opening log file '%s'\n", filename.toUtf8().data());
    }
  }
  if (!logfp) {
    logfp = stdout;
  }
#ifdef Q_OS_WIN
  /* Windows does not support line-buffering, so use no buffering */
  setvbuf(logfp, NULL, _IONBF, 0);
#else
  setvbuf(logfp, NULL, _IOLBF, 0); /* use line buffering */
#endif

  qInstallMessageHandler(logMsgHandler);
}
