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
