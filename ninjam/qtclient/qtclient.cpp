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

#include <QApplication>
#include <QSettings>

#include "MainWindow.h"
#include "ConnectDialog.h"

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);

  /* These are used by QSettings persistent settings */
  app.setOrganizationName("Wahjam Project");
  app.setOrganizationDomain("wahjam.org");
  app.setApplicationName("Wahjam");

  MainWindow mainWindow;
  mainWindow.show();

  ConnectDialog connectDialog;
  QSettings settings;
  QStringList hosts = settings.value("connect/hosts").toStringList();

  connectDialog.setRecentHostsList(hosts);
  connectDialog.setUser(settings.value("connect/user").toString());
  connectDialog.setIsPublicServer(settings.value("connect/public", true).toBool());

  if (connectDialog.exec() != QDialog::Accepted) {
    return 0;
  }

  hosts.prepend(connectDialog.host());
  hosts.removeDuplicates();
  hosts = hosts.mid(0, 16); /* limit maximum number of elements */

  settings.setValue("connect/hosts", hosts);
  settings.setValue("connect/user", connectDialog.user());
  settings.setValue("connect/public", connectDialog.isPublicServer());

  QString user = connectDialog.user();
  if (connectDialog.isPublicServer()) {
    user.prepend("anonymous:");
  }

  mainWindow.Connect(connectDialog.host(), user, connectDialog.pass());
  return app.exec();
}
