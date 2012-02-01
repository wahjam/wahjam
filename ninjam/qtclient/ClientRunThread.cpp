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
