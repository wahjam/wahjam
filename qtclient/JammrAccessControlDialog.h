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

#ifndef _JAMMRACCESSCONTROLDIALOG_H_
#define _JAMMRACCESSCONTROLDIALOG_H_

#include <QUrl>
#include <QDialog>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QListWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QRadioButton>
#include <QDialogButtonBox>

class JammrAccessControlDialog : public QDialog
{
  Q_OBJECT

public:
  JammrAccessControlDialog(QNetworkAccessManager *netManager_,
                           const QUrl &apiUrl_,
                           const QString &server_,
                           QWidget *parent = 0);

public slots:
  void refresh();
  void addUsername();
  void removeUsername();
  void applyChanges();

private slots:
  void completeFetchAcl();
  void completeStoreAcl();

private:
  QNetworkAccessManager *netManager;
  QUrl apiUrl;
  QString server;
  QNetworkReply *reply;
  QListWidget *usernamesList;
  QPushButton *addButton;
  QPushButton *removeButton;
  QLineEdit *usernameEdit;
  QRadioButton *allowRadio;
  QRadioButton *blockRadio;
  QPushButton *applyButton;
  QPushButton *cancelButton;
  QDialogButtonBox *dialogButtonBox;

  void setWidgetsEnabled(bool enable);
};

#endif /* _JAMMRACCESSCONTROLDIALOG_H_ */
