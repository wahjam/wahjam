#include <QApplication>

#include "MainWindow.h"
#include "ConnectDialog.h"

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);
  MainWindow mainWindow;
  mainWindow.show();

  ConnectDialog connectDialog;
  if (connectDialog.exec() != QDialog::Accepted) {
    return 0;
  }

  QString user = connectDialog.user();
  if (connectDialog.isPublicServer()) {
    user.prepend("anonymous:");
  }

  mainWindow.Connect(connectDialog.host(), user, connectDialog.pass());
  return app.exec();
}
