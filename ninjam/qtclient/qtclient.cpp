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
  connectDialog.setHost(settings.value("connect/host").toString());
  connectDialog.setUser(settings.value("connect/user").toString());
  connectDialog.setIsPublicServer(settings.value("connect/public", true).toBool());

  if (connectDialog.exec() != QDialog::Accepted) {
    return 0;
  }

  settings.setValue("connect/host", connectDialog.host());
  settings.setValue("connect/user", connectDialog.user());
  settings.setValue("connect/public", connectDialog.isPublicServer());

  QString user = connectDialog.user();
  if (connectDialog.isPublicServer()) {
    user.prepend("anonymous:");
  }

  mainWindow.Connect(connectDialog.host(), user, connectDialog.pass());
  return app.exec();
}
