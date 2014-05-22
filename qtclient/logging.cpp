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

#if defined(Q_OS_WIN)
  switch (QSysInfo::windowsVersion()) {
  case QSysInfo::WV_2000:
    qDebug("Windows 2000");
    break;
  case QSysInfo::WV_XP:
    qDebug("Windows XP");
    break;
  case QSysInfo::WV_2003:
    qDebug("Windows 2003");
    break;
  case QSysInfo::WV_VISTA:
    qDebug("Windows Vista");
    break;
  case QSysInfo::WV_WINDOWS7:
    qDebug("Windows 7");
    break;
  default:
    qDebug("Unsupported or unrecognized Windows version");
    break;
  }
#elif defined(Q_OS_MAC)
  switch (QSysInfo::MacintoshVersion) {
  case QSysInfo::MV_10_3:
    qDebug("Mac OS X 10.3");
    break;
  case QSysInfo::MV_10_4:
    qDebug("Mac OS X 10.4");
    break;
  case QSysInfo::MV_10_5:
    qDebug("Mac OS X 10.5");
    break;
  case QSysInfo::MV_10_6:
    qDebug("Mac OS X 10.6");
    break;
  case QSysInfo::MV_10_7:
    qDebug("Mac OS X 10.7");
    break;
  default:
    qDebug("Unsupported or unrecognized Mac OS version");
    break;
  }
#elif defined(Q_OS_LINUX)
  {
    FILE *issuefp = utf8_fopen("/etc/issue", "r");
    qDebug("Reading /etc/issue...");
    if (issuefp) {
      char buf[BUFSIZ];
      while (fgets(buf, sizeof(buf), issuefp)) {
        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') {
          buf[len - 1] = '\0';
          len--;
        }
        if (len > 0) {
          qDebug(buf);
        }
      }
      fclose(issuefp);
    }
    qDebug("End of /etc/issue");
  }
#endif
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

  qInstallMessageHandler(logMsgHandler);

  qDebug(APPNAME " %s (%s)", VERSION, COMMIT_ID);
  logSystemInformation();
}
