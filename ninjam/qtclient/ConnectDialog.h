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

class ConnectDialog : public QDialog
{
  Q_OBJECT
  Q_PROPERTY(QString host READ host)
  Q_PROPERTY(QString user READ user WRITE setUser)
  Q_PROPERTY(bool isPublicServer READ isPublicServer WRITE setIsPublicServer)
  Q_PROPERTY(QString pass READ pass)

public:
  ConnectDialog(QWidget *parent = 0);
  void setRecentHostsList(const QStringList &hosts);
  QString host() const;
  QString user() const;
  void setUser(const QString &user);
  bool isPublicServer() const;
  void setIsPublicServer(bool isPublicServer);
  QString pass() const;

private slots:
  void publicServerStateChanged(int state);

private:
  QComboBox *hostEdit;
  QLineEdit *userEdit;
  QCheckBox *publicCheckbox;
  QLineEdit *passEdit;
};

#endif /* _CONNECTDIALOG_H_ */
