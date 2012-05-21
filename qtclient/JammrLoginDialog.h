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

#ifndef _JAMMRLOGINDIALOG_H_
#define _JAMMRLOGINDIALOG_H_

#include <QDialog>
#include <QLineEdit>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class JammrLoginDialog : public QDialog
{
  Q_OBJECT
  Q_PROPERTY(QString username READ username WRITE setUsername)
  Q_PROPERTY(QString password READ password WRITE setPassword)
  Q_PROPERTY(QString token READ token)

public:
  JammrLoginDialog(QNetworkAccessManager *netmanager_, const QUrl &apiUrl_,
                   const QUrl &registerUrl, QWidget *parent = 0);
  QString username() const;
  QString password() const;
  void setUsername(const QString &username);
  void setPassword(const QString &password);
  QString token() const;

private slots:
  void login();
  void requestFinished();

private:
  QLineEdit *userEdit;
  QLineEdit *passEdit;
  QNetworkAccessManager *netmanager;
  QNetworkReply *reply;
  QUrl apiUrl;
  QString hexToken;
};

#endif /* _JAMMRLOGINDIALOG_H_ */
