/*
    Copyright (C) 2005-2007 Cockos Incorporated
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

#ifndef _WIN32
#include <arpa/inet.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <QTcpSocket>
#include <QDir>
#include <QDateTime>

#include "ninjamsrv.h"
#include "Server.h"

Server::Server(CreateUserLookupFn *createUserLookup, QObject *parent)
  : QObject(parent)
{
  group = new User_Group(createUserLookup, this);
  connect(group, SIGNAL(userAuthenticated()),
          this, SLOT(userAuthenticated()));
  connect(group, SIGNAL(userDisconnected()),
          this, SLOT(userDisconnected()));

  connect(&listener, SIGNAL(newConnection()),
          this, SLOT(acceptNewConnection()));

  sessionUpdateTimer.setSingleShot(true);
  connect(&sessionUpdateTimer, SIGNAL(timeout()),
          this, SLOT(updateNextSession()));
}

void AccessControlList::add(unsigned long addr, unsigned long mask, int flags)
{
  ACLEntry f = {addr, mask, flags};
  int os = list.GetSize();
  list.Resize(os + sizeof(f));
  memcpy((char *)list.Get() + os, &f, sizeof(f));
}

void AccessControlList::clear()
{
  list.Resize(0);
}

int AccessControlList::lookup(unsigned long addr)
{
  ACLEntry *p = (ACLEntry *)list.Get();
  int x = list.GetSize() / sizeof(ACLEntry);
  while (x--)
  {
    if ((addr & p->mask) == p->addr) {
      return p->flags;
    }
    p++;
  }
  return 0;
}

void Server::enforceACL()
{
  int x;
  int killcnt=0;
  for (x = 0; x < group->m_users.GetSize(); x ++)
  {
    User_Connection *c=group->m_users.Get(x);
    uint32_t addr = c->m_netcon.GetRemoteAddr().toIPv4Address();
    if (config->acl.lookup(addr) == ACL_FLAG_DENY)
    {
      c->m_netcon.Kill();
      killcnt++;
    }
  }
  if (killcnt) qDebug("killed %d users by enforcing ACL",killcnt);
}

/* Keeps a reference to config, do not delete while Server exists */
bool Server::setConfig(ServerConfig *config_)
{
  config = config_;

  group->m_max_users = config->maxUsers;
  if (!group->m_topictext.Get()[0]) {
      group->m_topictext.Set(config->defaultTopic.Get());
  }
  group->m_keepalive = config->keepAlive;
  group->m_voting_threshold = config->votingThreshold;
  group->m_voting_timeout = config->votingTimeout;
  group->m_allow_hidden_users = config->allowHiddenUsers;

  /* Only set certain settings when not active */
  if (!listener.isListening()) {
    group->SetProtocol(config->jammrApiUrl.isEmpty() ?
                       JAM_PROTO_NINJAM :
                       JAM_PROTO_JAMMR);

    group->SetConfig(config->defaultBPI, config->defaultBPM);
  }

  enforceACL();
  group->SetLicenseText(config->license.Get());
  updateNextSession();

  listener.close();
  if (!listener.listen(QHostAddress::Any, config->port)) {
    qWarning("Error listening on port %d!", config->port);
    return false;
  }

  qDebug("Port: %d", config->port);
  return true;
}

void Server::acceptNewConnection()
{
  QTcpSocket *sock = listener.nextPendingConnection();
  if (!sock) {
    return;
  }

  uint32_t addr = sock->peerAddress().toIPv4Address();
  int flag = config->acl.lookup(addr);
  if (flag == ACL_FLAG_DENY)
  {
    qDebug("Denying connection (via ACL) from %s:%u",
           sock->peerAddress().toString().toLatin1().constData(),
           sock->peerPort());
    delete sock;
    return;
  }

  group->AddConnection(sock, flag == ACL_FLAG_RESERVE);
}

void Server::userAuthenticated()
{
  if (sessionUpdateTimer.isActive()) {
    return; /* nothing to do */
  }
  updateNextSession();
}

void Server::userDisconnected()
{
  if (!sessionUpdateTimer.isActive()) {
    return; /* nothing to do */
  }
  if (group->numAuthenticatedUsers() < 2) {
    updateNextSession();
  }
}

void Server::setActiveSessionUpdateTimer()
{
  int sec = config->logSessionLen * 60;
  if (sec < 60) {
    sec = 30;
  }

  sessionUpdateTimer.start(sec * 1000 /* milliseconds */);
}

void Server::updateNextSession()
{
  sessionUpdateTimer.stop();
  group->SetLogDir(NULL);

  if (config->logPath.Get()[0] == '\0') {
    return;
  }

  if (group->numAuthenticatedUsers() < 2) {
    return;
  }

  QDir logDir(config->logPath.Get());
  QString sessionPath;
  int cnt;
  for (cnt = 0; cnt < 16; cnt++) {
    QString sessionName = QDateTime::currentDateTime().toString("yyyyMMdd_hhmm");
    if (cnt > 0) {
      sessionName += QString("_%1").arg(cnt);
    }
    sessionName += ".wahjam";

    sessionPath = logDir.absoluteFilePath(sessionName);
    if (logDir.mkdir(sessionName)) {
      break;
    }
  }

  if (cnt < 16 )
  {
    group->SetLogDir(sessionPath.toLocal8Bit().constData());
    setActiveSessionUpdateTimer();
  }
  else
  {
    qWarning("Error creating a session archive directory! Gave up after '%s' failed!",
             sessionPath.toLatin1().constData());
  }
}
