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

#ifndef _ADDVSTPLUGINDIALOG_H_
#define _ADDVSTPLUGINDIALOG_H_

#include <QDialog>
#include <QStringList>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>

class AddVSTPluginDialog : public QDialog
{
  Q_OBJECT
  Q_PROPERTY(QString searchPath READ searchPath WRITE setSearchPath)
  Q_PROPERTY(QStringList plugins READ plugins WRITE setPlugins)
  Q_PROPERTY(QString fileName READ fileName)

public:
  AddVSTPluginDialog(QWidget *parent = 0);

  QString searchPath() const;
  void setSearchPath(const QString &path);
  QStringList plugins() const;
  void setPlugins(const QStringList &plugins);
  QString fileName() const;

private slots:
  void addSearchPath();
  void scan();
  void itemSelectionChanged();

private:
  QLineEdit *searchPathEdit;
  QListWidget *pluginsList;
  QPushButton *okButton;

  void addPlugin(const QString &file);
};

#endif /* _ADDVSTPLUGINDIALOG_H_ */
