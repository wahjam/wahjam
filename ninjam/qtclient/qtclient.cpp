#include <QApplication>

#include "MainWindow.h"

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);

  /* These are used by QSettings persistent settings */
  app.setOrganizationName("Wahjam Project");
  app.setOrganizationDomain("wahjam.org");
  app.setApplicationName("Wahjam");

  MainWindow mainWindow;
  mainWindow.show();
  mainWindow.ShowConnectDialog();
  return app.exec();
}
