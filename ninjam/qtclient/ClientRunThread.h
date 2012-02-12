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
  void statusChanged(int newStatus);
  void beatInfoChanged(int bpm, int bpi);
  void currentBeatChanged(int currentBeat, int bpi);

private:
  bool running;
  QMutex *mutex;
  QWaitCondition cond;
  NJClient *client;
};

#endif /* _CLIENTRUNTHREAD_H_ */
