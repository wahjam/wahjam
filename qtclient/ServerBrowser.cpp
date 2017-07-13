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

#include "ServerBrowser.h"


/**
 * ServerBrowser constructor
 */
ServerBrowser::ServerBrowser(QNetworkAccessManager *manager_, QWidget *parent)
  : QTreeWidget(parent), netManager(manager_)
{
  setRootIsDecorated(false);
  setItemsExpandable(false);
  setColumnWidth(0, 200);

  connect(this, SIGNAL(itemSelectionChanged()),
          this, SLOT(onItemSelectionChanged()));
  connect(this, SIGNAL(itemActivated(QTreeWidgetItem*,int)),
          this, SLOT(onItemActivated(QTreeWidgetItem*,int)));
}

void ServerBrowser::loadServerList(const QUrl &url)
{
  reply = sendNetworkRequest(url);

  connect(reply, SIGNAL(finished()),
          this, SLOT(completeDownloadServerList()));
  connect(reply, SIGNAL(finished()),
          reply, SLOT(deleteLater()));
}


void ServerBrowser::onItemSelectionChanged()
{
  QTreeWidgetItem *item = currentItem();
  QString hostname;

  if (item) {
    hostname = item->data(0, Qt::UserRole).toString();
  }

  emit serverItemSelected(hostname);
}

void ServerBrowser::onItemActivated(QTreeWidgetItem *item, int column)
{
  Q_UNUSED(column);
  emit serverItemActivated(item->data(0, Qt::UserRole).toString());
}

void ServerBrowser::completeDownloadServerList()
{
  QTextStream stream(reply);

  parseServerList(&stream);
}
