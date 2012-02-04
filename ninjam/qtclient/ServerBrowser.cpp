/*
    Copyright (C) 2012 tea <Ikkei.Shimomura@gmail.com>

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

#include <QRegExp>
#include "ServerBrowser.h"


/**
 * ServerBrowser constructor
 */
ServerBrowser::ServerBrowser(QWidget *parent)
  : QTreeWidget(parent)
{
  setHeaderLabels(QStringList() << "Server" << "Tempo" << "Users");
  setRootIsDecorated(false);
  setItemsExpandable(false);
  setColumnWidth(0, 200);


  // NOTE: network-access-manager should be application global (singleton).
  // but currently, we don't use QtNetwork other place, so it just stays here.

  // NOTE: when the network-access-manager move scope, be careful the life
  // cycle of reply object. it should be deleted by manual.

  manager = new QNetworkAccessManager(this);


  connect(this, SIGNAL(itemClicked(QTreeWidgetItem*,int)),
          this, SLOT(onItemClicked(QTreeWidgetItem*,int)));
  connect(this, SIGNAL(itemActivated(QTreeWidgetItem*,int)),
          this, SLOT(onItemActivated(QTreeWidgetItem*,int)));
}

void ServerBrowser::loadServerList(const QUrl &url)
{
  QNetworkRequest request(url);

  reply = manager->get(request);

  connect(reply, SIGNAL(finished()),
          this, SLOT(completeDownloadServerList()));
  connect(reply, SIGNAL(finished()),
          reply, SLOT(deleteLater()));
}


void ServerBrowser::onItemClicked(QTreeWidgetItem *item, int column)
{
  emit serverItemClicked(item->data(0, Qt::DisplayRole).toString());
}

void ServerBrowser::onItemActivated(QTreeWidgetItem *item, int column)
{
  emit serverItemActivated(item->data(0, Qt::DisplayRole).toString());
}

void ServerBrowser::completeDownloadServerList()
{
  QTextStream stream(reply);

  parseServerList(&stream);
}

void ServerBrowser::parseServerList(QTextStream *stream)
{
  QRegExp serverPattern("SERVER\\s+\"([^\"]+)\"\\s+\"([^\"]+)\"\\s+\"([^\"]+)\".*");

  while (!stream->atEnd()) {
    if (!serverPattern.exactMatch(stream->readLine())) {
      continue;
    }

    QTreeWidgetItem *item = new QTreeWidgetItem(this);
    item->setText(0, serverPattern.cap(1)); // server
    item->setText(1, serverPattern.cap(2)); // bpm/bpi
    item->setText(2, serverPattern.cap(3)); // member list
  }
}

