#ifndef _CLIENTRUNTHREAD_H_
#define _CLIENTRUNTHREAD_H_

#include <QMutex>
#include <QThread>
#include <QWaitCondition>

#include "../njclient.h"

/*
 * Thread that invokes NJClient::Run() at regular intervals and bounces
 * callbacks to the GUI thread
 */
class ClientRunThread : public QThread
{
  Q_OBJECT

public:
  ClientRunThread(QMutex *mutex, NJClient *client, QObject *parent = 0);

  void run();
  void stop();
  bool licenseCallbackTrampoline(const char *licensetext);
  void chatMessageCallbackTrampoline(char **parms, int nparms);

signals:
  void licenseCallback(const char *licensetext, bool *result);
  void chatMessageCallback(char **parms, int nparms);
  void userInfoChanged();

private:
  bool running;
  QMutex *mutex;
  QWaitCondition cond;
  NJClient *client;
};

#endif /* _CLIENTRUNTHREAD_H_ */
