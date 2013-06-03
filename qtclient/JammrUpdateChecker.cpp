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

#include "JammrUpdateChecker.h"

JammrUpdateChecker::JammrUpdateChecker(QWidget *parent_, QNetworkAccessManager *netManager_)
  : parent(parent_), netManager(netManager_), reply(NULL)
{
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

  qDebug("Checking for updates from %s", updateUrl.toString().toLatin1().data());
}

void JammrUpdateChecker::requestFinished()
{
  reply->deleteLater();

  QNetworkReply::NetworkError err = reply->error();
  if (err != QNetworkReply::NoError) {
    qCritical("Update checker network reply failed (error=%d)", err);
    return;
  }

  QString latestVersion = QString::fromUtf8(reply->readAll()).trimmed();
  qDebug("Update checker finished, current \"%s\" latest \"%s\"",
         VERSION, latestVersion.toLatin1().data());
  if (latestVersion.isEmpty() || latestVersion == VERSION) {
    return;
  }

  QMessageBox::StandardButton button = QMessageBox::information(parent,
      tr("Updates available..."),
      tr("It is recommended that you update to version %1.  Download updates now?").arg(latestVersion),
      QMessageBox::Yes | QMessageBox::No,
      QMessageBox::Yes);
  if (button == QMessageBox::Yes) {
    QDesktopServices::openUrl(downloadUrl);
  }
}
