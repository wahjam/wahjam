#include <QApplication>

#include "../njclient.h"

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);
  NJClient client;

  return app.exec();
}
