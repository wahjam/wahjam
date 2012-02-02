#include "ClientRunThread.h"

/*
 * Pass in a mutex to synchronize NJClient access with other threads in the
 * program.
 */
ClientRunThread::ClientRunThread(QMutex *mutex, NJClient *client, QObject *parent)
  : QThread(parent), mutex(mutex), client(client)
{
}

void ClientRunThread::run()
{
  mutex->lock();
  running = true;
  while (running) {
    while (!client->Run());
    if (client->HasUserInfoChanged()) {
      emit userInfoChanged();
    }
    cond.wait(mutex, 20 /* milliseconds */);
  }
  mutex->unlock();
}

void ClientRunThread::stop()
{
  mutex->lock();
  running = false;
  cond.wakeOne();
  mutex->unlock();
  wait();
}

/* Called from trampoline function, bounces into GUI thread */
bool ClientRunThread::licenseCallbackTrampoline(const char *licensetext)
{
  bool result = false;
  emit licenseCallback(licensetext, &result);
  return result;
}

void ClientRunThread::chatMessageCallbackTrampoline(char **parms, int nparms)
{
  emit chatMessageCallback(parms, nparms);
}
