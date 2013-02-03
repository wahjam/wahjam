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

void Net_Connection::socketError(QAbstractSocket::SocketError)
{
  /* Normally QTcpSocket emits disconnected() for us, but if the socket is not
   * connected yet we need to emit the signal manually because QTcpSocket will
   * not. */
  if (m_sock->state() == QAbstractSocket::UnconnectedState) {
    emit disconnected();
  }

  Kill();
}

void Net_Connection::readyRead()
{
  bool msgEnqueued = false;

  while (m_sock->bytesAvailable())
  {
    char buf[8192];
    int buflen = m_sock->peek(buf, sizeof(buf));
    int a = 0;

    if (!m_recvmsg) {
      m_recvmsg = new Net_Message;
      m_recvstate = 0;
    }

    if (!m_recvstate)
    {
      a = m_recvmsg->parseMessageHeader(buf, buflen);
      if (a < 0)
      {
        Kill();
        return;
      }
      if (a == 0) {
        return;
      }
      m_recvstate = 1;
    }
    int b2 = m_recvmsg->parseAddBytes(buf + a, buflen - a);

    m_sock->read(buf, b2 + a); // dump our bytes that we used

    if (m_recvmsg->parseBytesNeeded() < 1)
    {
      recvq.enqueue(m_recvmsg);
      m_recvmsg = 0;
      m_recvstate = 0;
      recvKeepaliveTimer.start();
      msgEnqueued = true;
    }
  }

  /* Only emit signal once per readyRead() */
  if (msgEnqueued) {
    emit messagesReady();
  }
}

void Net_Connection::sendKeepaliveMessage()
{
  Net_Message *keepalive = new Net_Message;
  keepalive->set_type(MESSAGE_KEEPALIVE);
  keepalive->set_size(0);
  Send(keepalive);
}

void Net_Connection::recvTimedOut()
{
  Kill();
}

Net_Message *Net_Connection::nextMessage()
{
  if (recvq.isEmpty()) {
    return 0;
  }

  return recvq.dequeue();
}

Net_Message *Net_Connection::Run(int *wantsleep)
{
  Net_Message *retv = nextMessage();
  if (retv && wantsleep) {
    *wantsleep = 0;
  }
  return retv;
}

bool Net_Connection::hasMessagesAvailable()
{
  return !recvq.isEmpty();
}

int Net_Connection::Send(Net_Message *msg, bool deleteAfterSend)
{
  if (!msg) {
    return 0;
  }

  int ret = -1;
  char buf[32];
  int hdrlen = msg->makeMessageHeader(buf);
  qint64 nbytes = m_sock->write(buf, hdrlen);
  if (nbytes != hdrlen) {
    goto err;
  }

  nbytes = m_sock->write((const char*)msg->get_data(), msg->get_size());
  if (nbytes != msg->get_size()) {
    goto err;
  }

  sendKeepaliveTimer.start();
  ret = 0;
err:
  if (deleteAfterSend) {
    delete msg;
  }
  return ret;
}

QHostAddress Net_Connection::GetRemoteAddr()
{
  /* Cache remote address since QTcpSocket clears it on disconnect */
  if (remoteAddr != QHostAddress::Null) {
    return remoteAddr;
  }
  remoteAddr = m_sock->peerAddress();
  return remoteAddr;
}

Net_Connection::Net_Connection(QTcpSocket *sock, QObject *parent)
  : QObject(parent), m_recvstate(0), m_recvmsg(0), m_sock(sock),
    remoteAddr(sock->peerAddress())
{
  m_sock->setParent(this);

  connect(sock, SIGNAL(error(QAbstractSocket::SocketError)),
          this, SLOT(socketError(QAbstractSocket::SocketError)));
  connect(sock, SIGNAL(readyRead()), this, SLOT(readyRead()));
  connect(sock, SIGNAL(disconnected()), this, SIGNAL(disconnected()));

  connect(&sendKeepaliveTimer, SIGNAL(timeout()),
          this, SLOT(sendKeepaliveMessage()));
  connect(&recvKeepaliveTimer, SIGNAL(timeout()),
          this, SLOT(recvTimedOut()));

  SetKeepAlive(0);
}

Net_Connection::~Net_Connection()
{
  Kill();
}

void Net_Connection::SetKeepAlive(int interval)
{
  if (interval == 0) {
    interval = NET_CON_KEEPALIVE_RATE;
  }

  sendKeepaliveTimer.start(interval * 1000 /* milliseconds */);
  recvKeepaliveTimer.start(3 * interval * 1000 /* milliseconds */);
}

void Net_Connection::Kill()
{
  sendKeepaliveTimer.stop();
  recvKeepaliveTimer.stop();
  m_sock->disconnectFromHost();

  while (!recvq.isEmpty()) {
    delete recvq.dequeue();
  }

  delete m_recvmsg;
  m_recvmsg = NULL;
}
