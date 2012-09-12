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

#ifndef _JAMMRCONNECTDIALOG_H_
#define _JAMMRCONNECTDIALOG_H_

#include <QUrl>
#include <QDialog>
#include <QNetworkAccessManager>

#include "JammrServerBrowser.h"

class JammrConnectDialog : public QDialog
{
  Q_OBJECT
  Q_PROPERTY(QString host READ host WRITE setHost)

public:
  JammrConnectDialog(QNetworkAccessManager *netManager_, const QUrl &apiUrl_, QWidget *parent = 0);
  QString host() const;

public slots:
  void setHost(const QString &host);
  void loadServerList();

private slots:
  void onServerSelected(const QString &host);

private:
  QNetworkAccessManager *netManager;
  JammrServerBrowser *serverBrowser;
  QString selectedHost;
  QUrl apiUrl;
};

#endif /* _JAMMRCONNECTDIALOG_H_ */
