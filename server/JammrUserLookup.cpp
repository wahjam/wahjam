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
#include <QDomDocument>

#include "common/UserPrivs.h"

#include "ninjamsrv.h"
#include "JammrUserLookup.h"

JammrUserLookup::JammrUserLookup(const QUrl &apiUrl,
                                 const QString &apiServerName,
                                 int max_channels_, const QString &username_)
{
  tokenUrl = apiUrl;
  tokenUrl.setPath(apiUrl.path() + "tokens/" + username_ + "/");
  tokenUrl.addQueryItem("server", apiServerName);
  tokenUrl.addQueryItem("format", "xml");

  max_channels = max_channels_;
  username.Set(username_.toUtf8().data());
}

void JammrUserLookup::start()
{
  QNetworkRequest request(tokenUrl);
  request.setRawHeader("Referer", tokenUrl.toString(QUrl::RemoveUserInfo).toAscii().data());

  reply = netmanager->get(request);
  connect(reply, SIGNAL(finished()), this, SLOT(requestFinished()));

  qDebug("Looking up user info at %s", tokenUrl.toString(QUrl::RemoveUserInfo).toLatin1().data());
}

void JammrUserLookup::requestFinished()
{
  reply->deleteLater();

  QNetworkReply::NetworkError err = reply->error();
  if (err != QNetworkReply::NoError) {
    qCritical("User info lookup network reply failed (error=%d)", err);
    emit completed();
    return;
  }

  qDebug("Finished looking up user info");

  QDomDocument doc;
  if (!doc.setContent(reply)) {
    qCritical("User info lookup XML parse error");
    emit completed();
    return;
  }

  QDomNode node(doc.elementsByTagName("token").item(0));
  if (node.isNull()) {
    qCritical("User info lookup <token> XML parsing failed");
    emit completed();
    return;
  }

  QString token = node.firstChild().nodeValue();
  QByteArray hash = QCryptographicHash::hash((QString(username.Get()) + ":" + token).toUtf8(),
                                             QCryptographicHash::Sha1);
  memcpy(sha1buf_user, hash.data(), sizeof(sha1buf_user));

  node = doc.elementsByTagName("privs").item(0);
  if (node.isNull()) {
    qCritical("User info lookup <privs> XML parsing failed");
    emit completed();
    return;
  }
  privs = privsFromString(node.firstChild().nodeValue());

  user_valid = 1;
  emit completed();
}
