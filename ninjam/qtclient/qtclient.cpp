#include <QApplication>
#include <QDebug>

#include "ConnectDialog.h"
#include "../njclient.h"

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);
  ConnectDialog connectDialog;
  NJClient client;

  if (connectDialog.exec() != QDialog::Accepted) {
    return 0;
  }

  QString user = connectDialog.GetUser();
  if (connectDialog.IsPublicServer()) {
    user.prepend("anonymous:");
  }

  client.Connect(connectDialog.GetHost().toAscii().data(),
                 user.toUtf8().data(),
                 connectDialog.GetPass().toUtf8().data());

  return app.exec();
}
