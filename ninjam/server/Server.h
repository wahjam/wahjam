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

#ifndef _SERVER_H_
#define _SERVER_H_

#include <QTcpServer>
#include <QTimer>

#include "../../WDL/string.h"
#include "../../WDL/ptrlist.h"
#include "usercon.h"

class UserPassEntry
{
public:
  UserPassEntry() {priv_flag=0;}
  ~UserPassEntry() {}
  WDL_String name, pass;
  unsigned int priv_flag;
};

#define ACL_FLAG_DENY 1
#define ACL_FLAG_RESERVE 2
typedef struct
{
  unsigned long addr;
  unsigned long mask;
  int flags;
} ACLEntry;

class AccessControlList
{
public:
  void add(unsigned long addr, unsigned long mask, int flags);
  void clear();
  int lookup(unsigned long addr);

private:
  WDL_HeapBuf list;
};

/* Server configuration variables */
struct ServerConfig
{
  bool allowAnonChat;
  bool allowAnonymous;
  bool allowAnonymousMulti;
  bool anonymousMaskIP;
  bool allowHiddenUsers;
  int setuid;
  int defaultBPM;
  int defaultBPI;
  int port;
  int keepAlive;
  int maxUsers;
  int maxchAnon;
  int maxchUser;
  int logSessionLen;
  int votingThreshold;
  int votingTimeout;
  WDL_String logPath;
  WDL_String pidFilename;
  WDL_String logFilename;
  WDL_String statusPass;
  WDL_String statusUser;
  WDL_String license;
  WDL_String defaultTopic;
  AccessControlList acl;
  WDL_PtrList<UserPassEntry> userlist;
};

class Server : public QObject
{
  Q_OBJECT

public:
  Server(CreateUserLookupFn *createUserLookup, QObject *parent=0);
  bool setConfig(ServerConfig *config);

private slots:
  void acceptNewConnection();
  void updateNextSession();

private:
  ServerConfig *config;
  User_Group *group;
  QTcpServer listener;
  QTimer sessionUpdateTimer;

  void enforceACL();
  void setIdleSessionUpdateTimer();
  void setActiveSessionUpdateTimer();
};

#endif /* _SERVER_H_ */
