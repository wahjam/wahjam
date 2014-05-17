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

#include <QNetworkRequest>
#include <QCryptographicHash>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>

#include "common/UserPrivs.h"

#include "ninjamsrv.h"
#include "JammrUserLookup.h"

JammrUserLookup::JammrUserLookup(const QUrl &apiUrl,
                                 const QString &apiServerName,
                                 int max_channels_, const QString &username_)
{
  QUrlQuery query;
  query.addQueryItem("server", apiServerName);

  tokenUrl = apiUrl;
  tokenUrl.setPath(apiUrl.path() + "tokens/" + username_ + "/");
  tokenUrl.setQuery(query);

  max_channels = max_channels_;
  username.Set(username_.toUtf8().data());
}

void JammrUserLookup::start()
{
  QNetworkRequest request(tokenUrl);
  request.setRawHeader("Referer", tokenUrl.toString(QUrl::RemoveUserInfo).toLatin1().data());

  reply = netmanager->get(request);
  connect(reply, SIGNAL(finished()), this, SLOT(requestFinished()));

  qDebug("Looking up user info at %s", tokenUrl.toString(QUrl::RemoveUserInfo).toLatin1().data());
}

void JammrUserLookup::requestFinished()
{
  reply->deleteLater();

  QNetworkReply::NetworkError netErr = reply->error();
  if (netErr != QNetworkReply::NoError) {
    qCritical("User info lookup network reply failed (error=%d)", netErr);
    emit completed();
    return;
  }

  qDebug("Finished looking up user info");

  QJsonParseError jsonErr;
  QJsonDocument doc(QJsonDocument::fromJson(reply->readAll(), &jsonErr));
  if (doc.isNull()) {
    qCritical("User info lookup JSON parse error: %s",
              jsonErr.errorString().toLatin1().constData());
    emit completed();
    return;
  }

  QString token = doc.object().value("token").toString();
  if (token.isEmpty()) {
    qCritical("User info lookup \"token\" JSON parsing failed");
    emit completed();
    return;
  }

  QByteArray hash = QCryptographicHash::hash((QString(username.Get()) + ":" + token).toUtf8(),
                                             QCryptographicHash::Sha1);
  memcpy(sha1buf_user, hash.data(), sizeof(sha1buf_user));

  QString privsString = doc.object().value("privs").toString();
  if (privsString.isNull()) {
    qCritical("User info lookup \"privs\" JSON parsing failed");
    emit completed();
    return;
  }
  privs = privsFromString(privsString);

  user_valid = 1;
  emit completed();
}
