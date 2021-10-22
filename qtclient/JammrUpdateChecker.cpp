/*
    Copyright (C) 2013 Stefan Hajnoczi <stefanha@gmail.com>

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

#include <QNetworkRequest>
#include <QMessageBox>
#include <QDesktopServices>
#include <QCoreApplication>

#include "JammrUpdateChecker.h"

JammrUpdateChecker::JammrUpdateChecker(QWidget *parent_, QNetworkAccessManager *netManager_)
  : QObject(parent_), parent(parent_), netManager(netManager_), reply(nullptr)
{
}

JammrUpdateChecker::~JammrUpdateChecker()
{
  if (reply) {
    reply->abort();
  }
}

void JammrUpdateChecker::setUpdateUrl(const QUrl &updateUrl_)
{
  updateUrl = updateUrl_;
}

void JammrUpdateChecker::setDownloadUrl(const QUrl &downloadUrl_)
{
  downloadUrl = downloadUrl_;
}

void JammrUpdateChecker::start()
{
  if (updateUrl.isEmpty()) {
    return;
  }

  QNetworkRequest request(updateUrl);
  reply = netManager->get(request);
  connect(reply, SIGNAL(finished()), this, SLOT(requestFinished()));
  connect(reply, &QNetworkReply::sslErrors,
          this, &JammrUpdateChecker::requestSslErrors);

  qDebug("Checking for updates from %s", updateUrl.toString().toLatin1().data());
}

/* Parse a major.minor.version string into numbers */
static void parseVersionString(const QString &str,
                               unsigned *major,
                               unsigned *minor,
                               unsigned *patch)
{
  QStringList fields = str.split('.');

  if (fields.size() != 3) {
    *major = 0;
    *minor = 0;
    *patch = 0;
    return;
  }

  *major = fields.at(0).toUInt();
  *minor = fields.at(1).toUInt();
  *patch = fields.at(2).toUInt();
}

static bool isUpToDate(const QString &ourVersion, const QString &latestVersion)
{
  unsigned ourMajor, ourMinor, ourPatch;
  parseVersionString(ourVersion, &ourMajor, &ourMinor, &ourPatch);

  unsigned latestMajor, latestMinor, latestPatch;
  parseVersionString(latestVersion, &latestMajor, &latestMinor, &latestPatch);

  if (ourMajor > latestMajor) {
    return true;
  }
  if (ourMajor == latestMajor) {
    if (ourMinor > latestMinor) {
      return true;
    }
    if (ourMinor == latestMinor) {
      return ourPatch >= latestPatch;
    }
  }
  return false;
}

void JammrUpdateChecker::requestSslErrors(const QList<QSslError> &errors)
{
  for (const QSslError &error : errors) {
    qCritical("SSL error: %s", error.errorString().toLatin1().constData());
  }
}

void JammrUpdateChecker::requestFinished()
{
  QNetworkReply *theReply = reply;
  reply->deleteLater();
  reply = nullptr;

  QNetworkReply::NetworkError err = theReply->error();
  if (err == QNetworkReply::OperationCanceledError) {
    return;
  } else if (err != QNetworkReply::NoError) {
    qCritical("Update checker network reply failed (error=%d, "
              "errorString=\"%s\")",
              err, theReply->errorString().toLatin1().constData());
    return;
  }

  int statusCode = theReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
  if (statusCode != 200 /* HTTP SUCCESS */) {
    qCritical("Update checker HTTP reply failed (status=%d)", statusCode);
    return;
  }

  QString latestVersion = QString::fromUtf8(theReply->readAll()).trimmed();
  qDebug("Update checker finished, current \"%s\" latest \"%s\"",
         VERSION, latestVersion.toLatin1().data());

  if (isUpToDate(VERSION, latestVersion)) {
    return;
  }

  QMessageBox::StandardButton button = QMessageBox::information(parent,
      tr("Updates available..."),
      tr("It is recommended that you update to version %1.  Close jammr and download updates now?").arg(latestVersion),
      QMessageBox::Yes | QMessageBox::No,
      QMessageBox::Yes);
  if (button == QMessageBox::Yes) {
    QDesktopServices::openUrl(downloadUrl);
    QCoreApplication::quit();
  }
}
