#include "MainWindow.h"
#include "ConnectDialog.h"
#include "../../WDL/jnetlib/jnetlib.h"

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent)
{
  JNL::open_socketlib();

  setWindowTitle(tr("Wahjam"));
  setCentralWidget(new QWidget);

  ConnectDialog connectDialog;
/*  NJClient client; */

  if (connectDialog.exec() != QDialog::Accepted) {
    return; /* TODO */
  }

  QString user = connectDialog.GetUser();
  if (connectDialog.IsPublicServer()) {
    user.prepend("anonymous:");
  }
/*  client.Connect(connectDialog.GetHost().toAscii().data(),
                 user.toUtf8().data(),
                 connectDialog.GetPass().toUtf8().data()); */
}

MainWindow::~MainWindow()
{
  JNL::close_socketlib();
}
