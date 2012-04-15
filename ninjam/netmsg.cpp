/*
    Copyright (C) 2005-2007 Cockos Incorporated

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

/*

  This file provides the implementations of the Net_Messsage class, and 
  Net_Connection class (handles sending and receiving Net_Messages)

*/

#include <stdint.h>


#ifdef _WIN32
#include <windows.h>
#else
#include <stdlib.h>
#include <memory.h>
#endif

#include "netmsg.h"

int Net_Message::parseBytesNeeded()
{
  return get_size()-m_parsepos;
}

int Net_Message::parseAddBytes(void *data, int len)
{
  char *p=(char*)get_data();
  if (!p) return 0;
  if (len > parseBytesNeeded()) len = parseBytesNeeded();
  memcpy(p+m_parsepos,data,len);
  m_parsepos+=len; 
  return len;
}

int Net_Message::parseMessageHeader(void *data, int len) // returns bytes used, if any (or 0 if more data needed) or -1 if invalid
{
	unsigned char *dp=(unsigned char *)data;
  if (len < 5) return 0;

  int type=*dp++;

  int size = *dp++; 
  size |= ((int)*dp++)<<8; 
  size |= ((int)*dp++)<<16; 
  size |= ((int)*dp++)<<24; 
  len -= 5;
  if (type == MESSAGE_INVALID || size < 0 || size > NET_MESSAGE_MAX_SIZE) {
    fprintf(stderr, "len = %d\n", len);
    for (int i = 0; i < len; i += 16) {
      for (int j = 0; j < 16; j++) {
        fprintf(stderr, "%02x ", ((uint8_t*)data)[i + j]);
      }
      fprintf(stderr, "\n");
    }
    fprintf(stderr, "\n");
    return -1;
  }

  m_type=type;
  set_size(size);

  m_parsepos=0;

  return 5;
}

int Net_Message::makeMessageHeader(void *data) // makes message header, data should be at least 16 bytes to be safe
{
	if (!data) return 0;

	unsigned char *dp=(unsigned char *)data;
  *dp++ = (unsigned char) m_type;
  int size=get_size();
  *dp++=size&0xff; size>>=8;
  *dp++=size&0xff; size>>=8;
  *dp++=size&0xff; size>>=8;
  *dp++=size&0xff;

  return (dp-(unsigned char *)data);
}



Net_Message *Net_Connection::Run(int *wantsleep)
{
  if (m_error) {
    return 0;
  }

  time_t now = time(NULL);

  if (now > m_last_send + m_keepalive) {
    Net_Message *keepalive = new Net_Message;
    keepalive->set_type(MESSAGE_KEEPALIVE);
    keepalive->set_size(0);
    Send(keepalive);
    m_last_send = now;
  }

  Net_Message *retv = 0;

  if (!m_recvmsg) {
    m_recvmsg = new Net_Message;
    m_recvstate = 0;
  }

  while (!retv && m_sock->bytesAvailable())
  {
    char buf[8192];
    char buf2[8192];
    int buflen = m_sock->peek(buf, sizeof(buf));
    int a = 0;

    if (!m_recvstate)
    {
      a = m_recvmsg->parseMessageHeader(buf, buflen);
      if (a < 0)
      {
        m_error = -1;
        break;
      }
      if (a == 0) {
        break;
      }
      m_recvstate = 1;
    }
    int b2 = m_recvmsg->parseAddBytes(buf + a, buflen - a);

    int nread = m_sock->read(buf2, b2 + a); // dump our bytes that we used
    if (nread != b2 + a) {
      fprintf(stderr, "m_sock->read() nread %d b2 + a %d\n", nread, b2 + a);
    }
    if (memcmp((const void*)buf, (const void*)buf2, nread) != 0) {
      fprintf(stderr, "peek and read buffer mismatch!  race condition?\n");
    }

    if (m_recvmsg->parseBytesNeeded() < 1)
    {
      retv = m_recvmsg;
      m_recvmsg = 0;
      m_recvstate = 0;
    }
    if (wantsleep) {
      *wantsleep = 0;
    }
  }

  if (retv)
  {
    if (lastmsgs[lastmsgIdx]) {
      lastmsgs[lastmsgIdx]->releaseRef();
    }
    retv->addRef();
    lastmsgs[lastmsgIdx] = retv;
    lastmsgIdx = (lastmsgIdx + 1) % 5;

    m_last_recv = now;
  }
  else if (now > m_last_recv + m_keepalive * 3)
  {
    m_error = -3;
  }

  return retv;
}

int Net_Connection::Send(Net_Message *msg)
{
  if (!msg) {
    return 0;
  }

  char buf[32];
  int hdrlen = msg->makeMessageHeader(buf);
  qint64 ret = m_sock->write(buf, hdrlen);
  if (ret != hdrlen) {
    return -1;
  }

  ret = m_sock->write((const char*)msg->get_data(), msg->get_size());
  if (ret != msg->get_size()) {
    return -1;
  }
  return 0;
}

int Net_Connection::GetStatus()
{
  if (m_error) {
    fprintf(stderr, "m_error %d!\n", m_error);
    return m_error;
  }
  if (!m_sock || !m_sock->isOpen()) {
    fprintf(stderr, "socket closed!\n");
    return 1;
  }
  static int last_state;
  if (last_state != m_sock->state()) {
    fprintf(stderr, "last_state %d state %d\n", last_state, m_sock->state());
    last_state = m_sock->state();
  }
  switch (m_sock->state()) {
  case QAbstractSocket::HostLookupState:
  case QAbstractSocket::ConnectingState:
  case QAbstractSocket::ConnectedState:
  case QAbstractSocket::ClosingState:
    return 0;
  default:
    return 1;
  }
}

QHostAddress Net_Connection::GetRemoteAddr()
{
  return m_sock->peerAddress();
}

Net_Connection::~Net_Connection()
{
  for (int i = 0; i < 5; i++) {
    if (lastmsgs[i]) {
      lastmsgs[i]->releaseRef();
    }
  }

  delete m_recvmsg;
  delete m_sock;
}

void Net_Connection::Kill()
{
  m_sock->close();
}
