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

#include <QPushButton>
#include <QVBoxLayout>

#include "JammrConnectDialog.h"

JammrConnectDialog::JammrConnectDialog(QNetworkAccessManager *netManager_,
                                       const QUrl &apiUrl_, QWidget *parent)
  : QDialog(parent), netManager(netManager_), apiUrl(apiUrl_)
{
  serverBrowser = new JammrServerBrowser(netManager, this);
  connect(serverBrowser, SIGNAL(serverItemClicked(const QString &)),
          this, SLOT(setHost(const QString &)));
  connect(serverBrowser, SIGNAL(serverItemActivated(const QString &)),
          this, SLOT(onServerSelected(const QString &)));

  QPushButton *connectButton = new QPushButton(tr("&Connect"));
  connect(connectButton, SIGNAL(clicked()), this, SLOT(accept()));

  QVBoxLayout *layout = new QVBoxLayout;
  layout->setSpacing(2);
  layout->setContentsMargins(5, 5, 5, 5);
  layout->addWidget(serverBrowser);
  layout->addWidget(connectButton);
  setLayout(layout);
  setWindowTitle(tr("Connect to server..."));
  loadServerList();
}

QString JammrConnectDialog::host() const
{
  return selectedHost;
}

void JammrConnectDialog::setHost(const QString &host)
{
  selectedHost = host;
}

void JammrConnectDialog::onServerSelected(const QString &host)
{
  setHost(host);
  accept();
}

void JammrConnectDialog::loadServerList()
{
  serverBrowser->loadServerList(apiUrl);
}
