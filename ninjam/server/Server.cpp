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

#ifdef _WIN32
#include <io.h>
#endif

#include <QTcpSocket>

#include "ninjamsrv.h"
#include "Server.h"

/* Takes ownership of group_ */
Server::Server(User_Group *group_)
  : group(group_), nextSessionUpdateTime(0)
{
  connect(&listener, SIGNAL(newConnection()),
          this, SLOT(acceptNewConnection()));
}

Server::~Server()
{
  delete group;
}

void AccessControlList::add(unsigned long addr, unsigned long mask, int flags)
{
  addr = ntohl(addr);
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
  addr = ntohl(addr);

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
    if (config->acl.lookup(c->m_netcon.GetConnection()->get_remote()) == ACL_FLAG_DENY)
    {
      c->m_netcon.Kill();
      killcnt++;
    }
  }
  if (killcnt) logText("killed %d users by enforcing ACL\n",killcnt);
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

  /* Do not change back to default BPM/BPI if already running */
  if (!listener.isListening()) {
    group->SetConfig(config->defaultBPI, config->defaultBPM);
  }

  enforceACL();
  group->SetLicenseText(config->license.Get());

  listener.close();
  if (!listener.listen(QHostAddress::Any, config->port)) {
    logText("Error listening on port %d!\n", config->port);
    return false;
  }

  logText("Port: %d\n", config->port);
  return true;
}

void Server::acceptNewConnection()
{
  QTcpSocket *sock = listener.nextPendingConnection();
  if (!sock) {
    return;
  }

  uint32_t addr = htonl(sock->peerAddress().toIPv4Address());

  int flag = config->acl.lookup(addr);
  QString ipstr = sock->peerAddress().toString();
  logText("Incoming connection from %s!\n", ipstr.toLocal8Bit().constData());

  if (flag == ACL_FLAG_DENY)
  {
    logText("Denying connection (via ACL)\n");
    delete sock;
    return;
  }

  JNL_Connection *con = new JNL_Connection(JNL_CONNECTION_AUTODNS, 2*65536, 65536);

  struct sockaddr_in sa;
  sa.sin_family = AF_INET;
  sa.sin_port = htons(sock->peerPort());
  sa.sin_addr.s_addr = htonl(sock->peerAddress().toIPv4Address());

  /* Clone the file descriptor so the QTcpSocket can be safely closed.  This is
   * necessary so the QTcpSocket does not read incoming data and interfere with
   * the JNL_Connection.
   */
  con->connect(dup(sock->socketDescriptor()), &sa);
  delete sock;

  group->AddConnection(con, flag == ACL_FLAG_RESERVE);
}

/* Run an iteration of the main loop
 *
 * Return true if the main loop should sleep.
 */
bool Server::run()
{
  bool wantSleep = group->Run();

  time_t now;
  time(&now);
  if (now >= nextSessionUpdateTime)
  {
    group->SetLogDir(NULL);

    int len=30; // check every 30 seconds if we aren't logging

    if (config->logPath.Get()[0])
    {
      int x;
      for (x = 0; x < group->m_users.GetSize() && group->m_users.Get(x)->m_auth_state < 1; x ++);

      if (x < group->m_users.GetSize())
      {
        WDL_String tmp;

        int cnt=0;
        while (cnt < 16)
        {
          char buf[512];
          struct tm *t=localtime(&now);
          sprintf(buf,"/%04d%02d%02d_%02d%02d",t->tm_year+1900,t->tm_mon+1,t->tm_mday,t->tm_hour,t->tm_min);
          if (cnt)
            sprintf(buf+strlen(buf),"_%d",cnt);
          strcat(buf,".wahjam");

          tmp.Set(config->logPath.Get());
          tmp.Append(buf);

#ifdef _WIN32
          if (CreateDirectory(tmp.Get(),NULL)) break;
#else
          if (!mkdir(tmp.Get(),0755)) break;
#endif

          cnt++;
        }

        if (cnt < 16 )
        {
          logText("Archiving session '%s'\n",tmp.Get());
          group->SetLogDir(tmp.Get());
        }
        else
        {
          logText("Error creating a session archive directory! Gave up after '%s' failed!\n",tmp.Get());
        }
        // if we succeded, don't check until configured time
        len=config->logSessionLen*60;
        if (len < 60) len=30;
      }

    }
    nextSessionUpdateTime=now+len;
  }

  return wantSleep;
}
