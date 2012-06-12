/*
    Copyright (C) 2012 Ikkei Shimomura (tea) <Ikkei.Shimomura@gmail.com>

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

#include <QApplication>
#include <QRegExp>
#include <QDebug>
#include "ServerBrowser.h"


ServerBrowser::ServerBrowser(QNetworkAccessManager *manager_, QWidget *parent)
  : QTreeWidget(parent), netManager(manager_)
{
  setHeaderLabels(QStringList() << "Server" << "Tempo" << "Users");
  setRootIsDecorated(false);
  setItemsExpandable(false);
  setColumnWidth(0, 200);

  connect(this, SIGNAL(itemClicked(QTreeWidgetItem*,int)),
          this, SLOT(clickItem(QTreeWidgetItem*,int)));
  connect(this, SIGNAL(itemActivated(QTreeWidgetItem*,int)),
          this, SLOT(activateItem(QTreeWidgetItem*,int)));
}

void ServerBrowser::loadServerList(const QUrl &url)
{
  QNetworkRequest request(url);
  QNetworkReply *reply = netManager->get(request);

  Q_ASSERT(reply && reply->isRunning());

  connect(reply, SIGNAL(error(QNetworkReply::NetworkError)),
          this, SLOT(errorDownloadServerList(QNetworkReply::NetworkError)));
  connect(reply, SIGNAL(finished()),
          this, SLOT(completeDownloadServerList()));
  connect(reply, SIGNAL(finished()),
          reply, SLOT(deleteLater()));
}

void ServerBrowser::loadServerList(const QString &urlString)
{
  const QUrl url(urlString);
  loadServerList(url);
}

void ServerBrowser::clickItem(QTreeWidgetItem *item, int column)
{
  Q_UNUSED(column);

  emit serverItemClicked(item->data(0, Qt::DisplayRole).toString());
}

void ServerBrowser::activateItem(QTreeWidgetItem *item, int column)
{
  Q_UNUSED(column);

  emit serverItemActivated(item->data(0, Qt::DisplayRole).toString());
}

void ServerBrowser::errorDownloadServerList(QNetworkReply::NetworkError code)
{
  QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());

  if (reply) {
    Q_ASSERT(reply->error() != QNetworkReply::NoError);

    qDebug() << reply->errorString();
  }
}

void ServerBrowser::completeDownloadServerList()
{
  QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());

  if (reply) {
    Q_ASSERT(reply->isFinished());

    if (reply->error() == QNetworkReply::NoError) {
      QTextStream stream(reply);
      parseServerList(&stream);
    }
  }
  else {
    qDebug() << "sender must be QNetworkReply instance";
  }
}

void ServerBrowser::parseServerList(QTextStream *stream)
{
  QRegExp serverPattern("SERVER\\s+\"([^\"]+)\"\\s+\"([^\"]+)\"\\s+\"([^\"]+)\".*");
  Q_ASSERT(serverPattern.isValid());

  while (!stream->atEnd()) {
    if (!serverPattern.exactMatch(stream->readLine())) {
      continue;
    }

    Q_ASSERT(serverPattern.captureCount() == 3);

    addItem(serverPattern.cap(1),
            serverPattern.cap(2),
	    serverPattern.cap(3));

    qApp->processEvents();
  }
}

void ServerBrowser::addItem(const QString &serverName,
                            const QString &metronomeInfo,
	                    const QString &memberList)
{
  QTreeWidgetItem *item = new QTreeWidgetItem(this);
  Q_CHECK_PTR(item);

  item->setText(0, serverName);
  item->setText(1, metronomeInfo);
  item->setText(2, memberList);
}

