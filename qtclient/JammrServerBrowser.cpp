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

#include <QDomDocument>

#include "JammrServerBrowser.h"

JammrServerBrowser::JammrServerBrowser(QNetworkAccessManager *manager, QWidget *parent)
  : ServerBrowser(manager, parent)
{
  setHeaderLabels(QStringList() << "Topic" << "Tempo" << "Slots" << "Users");
}

QNetworkReply *JammrServerBrowser::sendNetworkRequest(const QUrl &apiUrl)
{
  QUrl livejamsUrl(apiUrl);
  livejamsUrl.setPath(apiUrl.path() + "livejams/");
  livejamsUrl.setQuery("format=xml");

  QNetworkRequest request(livejamsUrl);
  request.setRawHeader("Referer", livejamsUrl.toString(QUrl::RemoveUserInfo).toLatin1().data());

  return netManager->get(request);
}

void JammrServerBrowser::parseServerList(QTextStream *stream)
{
  QDomDocument doc;


  if (!doc.setContent(stream->device())) {
    qCritical("Server list XML parse error");
    return;
  }

  /* The XML looks like this:
   *
   * <response>
   *     <resource>
   *         <users></users>
   *         <bpi>8</bpi>
   *         <bpm>120</bpm>
   *         <server>jam1.jammr.net:10100</server>
   *         <topic>Public jam - Play nicely</topic>
   *         <numusers>0</numusers>
   *         <is_public>True</is_public>
   *         <maxusers>0</maxusers>
   *     </resource>
   * </response>
   */

  for (QDomNode resource = doc.elementsByTagName("response").at(0).firstChild();
       !resource.isNull();
       resource = resource.nextSibling()) {
    QString server = resource.firstChildElement("server").firstChild().nodeValue();
    if (server.isEmpty()) {
      continue; // skip invalid element
    }

    QString topic = resource.firstChildElement("topic").firstChild().nodeValue();
    QString bpm = resource.firstChildElement("bpm").firstChild().nodeValue();
    QString bpi = resource.firstChildElement("bpi").firstChild().nodeValue();
    QString numUsers = resource.firstChildElement("numusers").firstChild().nodeValue();
    QString maxUsers = resource.firstChildElement("maxusers").firstChild().nodeValue();

    QStringList users;
    for (QDomNode user = resource.firstChildElement("users").firstChild();
         !user.isNull();
         user = user.nextSibling()) {
      users << user.firstChild().nodeValue();
    }

    QTreeWidgetItem *item = new QTreeWidgetItem(this);
    item->setData(0, Qt::UserRole, server);
    item->setText(0, topic);
    item->setText(1, QString("%1 BPM/%2").arg(bpm, bpi));
    item->setText(2, QString("%1/%2").arg(numUsers, maxUsers));
    item->setText(3, users.join(", "));
  }
}
