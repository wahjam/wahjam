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

  This header provides the declarations for the Net_Messsage class, and
  Net_Connection class (handles sending and receiving Net_Messages)

*/



#ifndef _NETMSG_H_
#define _NETMSG_H_

#include <QQueue>
#include <QTimer>
#include <QTcpSocket>
#include <QHostAddress>

#include "../WDL/queue.h"

#define NET_MESSAGE_MAX_SIZE 16384

#define NET_CON_MAX_MESSAGES 512

#define MESSAGE_KEEPALIVE 0xfd
#define MESSAGE_EXTENDED 0xfe
#define MESSAGE_INVALID 0xff

#define NET_CON_KEEPALIVE_RATE 3


class Net_Message
{
	public:
		Net_Message() : m_parsepos(0), m_refcnt(0), m_type(MESSAGE_INVALID)
		{
		}
		~Net_Message()
		{
		}


		void set_type(int type)	{ m_type=type; }
		int  get_type() { return m_type; }

		void set_size(int newsize) { m_hb.Resize(newsize); }
		int get_size() { return m_hb.GetSize(); }

		void *get_data() { return m_hb.Get(); }


		int parseMessageHeader(void *data, int len); // returns bytes used, if any (or 0 if more data needed), or -1 if invalid
    int parseBytesNeeded();
    int parseAddBytes(void *data, int len); // returns bytes actually added

		int makeMessageHeader(void *data); // makes message header, returns length. data should be at least 16 bytes to be safe


		void addRef() { ++m_refcnt; }
		void releaseRef() { if (--m_refcnt < 1) delete this; }

	private:
    		int m_parsepos;
		int m_refcnt;
		int m_type;
		WDL_HeapBuf m_hb;
};


class Net_Connection : public QObject
{
  Q_OBJECT

  public:
    Net_Connection(QTcpSocket *sock, QObject *parent = 0);
    ~Net_Connection();

    bool hasMessagesAvailable();
    Net_Message *nextMessage();
    Net_Message *Run(int *wantsleep=0);
    int Send(Net_Message *msg); // -1 on error

    QHostAddress GetRemoteAddr();

    void SetKeepAlive(int interval);

    void Kill();

  signals:
    void messagesReady();
    void disconnected();

  private slots:
    void socketError(QAbstractSocket::SocketError socketError);
    void readyRead();
    void sendKeepaliveMessage();
    void recvTimedOut();

  private:
    void setStatus(int s);

    int status;
    QTimer sendKeepaliveTimer;
    QTimer recvKeepaliveTimer;
    int m_recvstate;
    Net_Message *m_recvmsg;
    QQueue<Net_Message*> recvq;
    QTcpSocket *m_sock;
    QHostAddress remoteAddr;
};


#endif
