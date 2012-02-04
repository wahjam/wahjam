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

#ifndef _CONNECTDIALOG_H_
#define _CONNECTDIALOG_H_

#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include "ServerBrowser.h"

class ConnectDialog : public QDialog
{
  Q_OBJECT
  Q_PROPERTY(QString host READ host WRITE setHost)
  Q_PROPERTY(QString user READ user WRITE setUser)
  Q_PROPERTY(bool isPublicServer READ isPublicServer WRITE setIsPublicServer)
  Q_PROPERTY(QString pass READ pass)

public:
  ConnectDialog(QWidget *parent = 0);
  QString host() const;
  QString user() const;
  bool isPublicServer() const;
  QString pass() const;

public slots:
  void setHost(const QString &host);
  void setUser(const QString &user);
  void setRecentHostsList(const QStringList &hosts);
  void setIsPublicServer(bool isPublicServer);
  void loadServerList(const QUrl &url);

private slots:
  void publicServerStateChanged(int state);
  void onServerSelected(const QString &host);

private:
  ServerBrowser *serverBrowser;
  QComboBox *hostEdit;
  QLineEdit *userEdit;
  QCheckBox *publicCheckbox;
  QLineEdit *passEdit;
};

#endif /* _CONNECTDIALOG_H_ */
