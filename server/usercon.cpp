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

  This header provides the implementations of User_Group and User_Connection etc.
  These is where the core logic of the server (message routing and upload/download
  management) happens.

*/

#ifdef _WIN32
#include <windows.h>
#else
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#endif

#include <ctype.h>

#include <QHostAddress>
#include <QCryptographicHash>
#include <QUuid>

#include "ninjamsrv.h"
#include "usercon.h"
#include "../common/mpb.h"
#include "common/UserPrivs.h"

#ifdef _WIN32
#define strncasecmp strnicmp
#endif

static void guidtostr(unsigned char *guid, char *str)
{
  int x;
  for (x = 0; x < 16; x ++) sprintf(str+x*2,"%02x",guid[x]);
}



static int is_type_char_valid(int c)
{
  c&=0xff;
  return (c >= 'a' && c <= 'z') ||
         (c >= 'A' && c <= 'Z') ||
         (c >= '0' && c <= '9') ||
         c == ' ' || c == '-' || 
         c == '.' || c == '_';
}

static int is_type_valid(unsigned int t)
{
  return (t&0xff) != ' ' &&
          is_type_char_valid(t>>24) &&
          is_type_char_valid(t>>16) &&
          is_type_char_valid(t>>8) &&
          is_type_char_valid(t);
}


static void type_to_string(unsigned int t, char *out)
{
  if (is_type_valid(t))
  {
    out[0]=(t)&0xff;
    out[1]=(t>>8)&0xff;
    out[2]=(t>>16)&0xff;
    out[3]=' ';//(t>>24)&0xff;
    out[4]=0;
    int x=3;
    while (out[x]==' ' && x > 0) out[x--]=0;
  }
  else *out=0;
}



#define MAX_NICK_LEN 128 // not including null term

#define TRANSFER_TIMEOUT 8

User_Connection::User_Connection(QTcpSocket *sock, User_Group *grp) : group(grp), m_netcon(sock), m_auth_state(0), m_clientcaps(0), m_auth_privs(0), m_reserved(0), m_max_channels(0),
      m_vote_bpm(0), m_vote_bpm_lasttime(0), m_vote_bpi(0), m_vote_bpi_lasttime(0)
{
  name = QString("%1:%2").arg(sock->peerAddress().toString()).arg(sock->peerPort());
  qDebug("%s: Connected", name.toLatin1().constData());

  connect(&m_netcon, SIGNAL(messagesReady()), this, SLOT(netconMessagesReady()));
  connect(&m_netcon, SIGNAL(disconnected()), this, SIGNAL(disconnected()));

  /* An RFC4122 UUID has some non-random bits so apply SHA1 so that no bit can
   * be predicted.
   */
  QByteArray challenge =
    QCryptographicHash::hash(QUuid::createUuid().toRfc4122(),
                             QCryptographicHash::Sha1);
  memcpy(m_challenge, challenge.constData(), sizeof(m_challenge));

  mpb_server_auth_challenge ch;
  memcpy(ch.challenge,m_challenge,sizeof(ch.challenge));

  switch (group->GetProtocol()) {
  case JAM_PROTO_NINJAM:
    ch.protocol_version = PROTO_NINJAM_VER_CUR;
    break;

  case JAM_PROTO_JAMMR:
    ch.protocol_version = PROTO_JAMMR_VER_CUR;
    break;
  }

  int ka=grp->m_keepalive;

  if (ka < 0)ka=0;
  else if (ka > 255) ka=255;
  ch.server_caps=ka<<8;

  if (grp->m_licensetext.Get()[0])
  {
    m_netcon.SetKeepAlive(45); // wait a max of 45s * 3
    ch.license_agreement=grp->m_licensetext.Get();
  }
  else m_netcon.SetKeepAlive(grp->m_keepalive);
  Send(ch.build());

  m_lookup=0;

  /* Kill connection if there is no authentication response, but give enough
   * time so the license can be read.
   */
  connect(&authenticationTimer, SIGNAL(timeout()),
          this, SLOT(authenticationTimeout()));
  authenticationTimer.setSingleShot(true);
  authenticationTimer.start(120 * 1000 /* milliseconds */);
}

void User_Connection::Send(Net_Message *msg, bool deleteAfterSend)
{
  if (m_netcon.Send(msg, deleteAfterSend))
  {
    qWarning("%s: Error sending message to user '%s', queue full!",
             name.toLatin1().constData(), m_username.Get());
  }
}

void User_Connection::SendChatMessage(const QStringList &list)
{
  mpb_chat_message newmsg;
  QByteArray parmsUtf8[list.size()]; // holds string memory until method returns

  Q_ASSERT(list.size() <= sizeof(newmsg.parms) / sizeof(newmsg.parms[0]));

  for (int i = 0; i < list.size(); i++) {
    parmsUtf8[i] = list.at(i).toUtf8();
    newmsg.parms[i] = parmsUtf8[i].data();
  }
  Send(newmsg.build());
}

User_Connection::~User_Connection()
{

  int x;
  for (x = 0; x < m_sublist.GetSize(); x ++)
    delete m_sublist.Get(x);
  m_sublist.Empty();
  for (x = 0; x < m_recvfiles.GetSize(); x ++)
    delete m_recvfiles.Get(x);
  m_recvfiles.Empty();
  for (x = 0; x < m_sendfiles.GetSize(); x ++)
    delete m_sendfiles.Get(x);
  m_sendfiles.Empty();

  delete m_lookup;
  m_lookup=0;
}


void User_Connection::SendConfigChangeNotify(int bpm, int bpi)
{
  if (m_auth_state > 0)
  {
    mpb_server_config_change_notify mk;
    mk.beats_interval=bpi;
    mk.beats_minute=bpm;
    Send(mk.build());
  }
}

int User_Connection::OnRunAuth()
{
  {
    // fix any invalid characters in username
    char *p=m_lookup->username.Get();
    int l=MAX_NICK_LEN;
    while (*p)
    {
      char c=*p;
      if (!isalnum(c) && c != '-' && c != '_' && c != '@' && c != '.') c='_';
      *p++=c;

      if (!--l) *p=0;
    }
  }

  /* Update logging prefix with the attempted username */
  name.append(QString(" [%1]").arg(m_lookup->username.Get()));

  {
    QCryptographicHash shatmp(QCryptographicHash::Sha1);

    shatmp.addData((const char *)m_lookup->sha1buf_user, sizeof(m_lookup->sha1buf_user));
    shatmp.addData((const char *)m_challenge, sizeof(m_challenge));

    QByteArray result = shatmp.result();
    if ((m_lookup->reqpass && memcmp(result.constData(), m_lookup->sha1buf_request, result.length())) || !m_lookup->user_valid)
    {
      qDebug("%s: Refusing user, invalid login/password", name.toLatin1().constData());
      mpb_server_auth_reply bh;
      bh.errmsg="invalid login/password";
      Send(bh.build());
      return 0;
    }
  }

  if (m_lookup->is_status)
  {
    SendUserList();

    {
      mpb_server_config_change_notify mk;
      mk.beats_interval=group->m_last_bpi;
      mk.beats_minute=group->m_last_bpm;
      Send(mk.build());
    }

    SendChatMessage(QStringList("TOPIC") << "" << group->m_topictext.Get());

    {
      int cnt=0;
      int user;
      for (user = 0; user < group->m_users.GetSize(); user ++)
      {
        User_Connection *u=group->m_users.Get(user);
        if (u != this && u->m_auth_state > 0 && !(u->m_auth_privs & PRIV_HIDDEN))
          cnt++;
      }
      char buf[64],buf2[64];
      sprintf(buf,"%d",cnt);
      sprintf(buf2,"%d",group->m_max_users);

      SendChatMessage(QStringList("USERCOUNT") << buf << buf2);
    }

    return 0;
  }

  m_auth_privs=m_lookup->privs;
  m_max_channels = m_lookup->max_channels;
  m_username.Set(m_lookup->username.Get());
  // disconnect any user by the same name
  // in allowmulti mode, append -<idx>
  {
    int user=0;
    int uw_pos=0;

    while (user < group->m_users.GetSize())
    {
      User_Connection *u=group->m_users.Get(user);
      if (u != this && !strcasecmp(u->m_username.Get(),m_username.Get()))
      {

        if ((m_auth_privs & PRIV_ALLOWMULTI) && ++uw_pos < 16)
        {
          char buf[64];
          sprintf(buf,".%d",uw_pos+1);
          m_username.Set(m_lookup->username.Get());
          m_username.Append(buf);
          user=0;
          continue; // start over
        }
        else
        {
          delete u;
          group->m_users.Delete(user);
          break;
        }
      }
      user++;
    }   
  }


  if (group->m_max_users && !m_reserved && !(m_auth_privs & PRIV_RESERVE))
  {
    int user;
    int cnt=0;
    for (user = 0; user < group->m_users.GetSize(); user ++)
    {
      User_Connection *u=group->m_users.Get(user);
      if (u != this && u->m_auth_state > 0 && !(u->m_auth_privs & PRIV_HIDDEN))
        cnt++;
    }
    if (cnt >= group->m_max_users)
    {
      qDebug("%s: Refusing user %s, server full",
             name.toLatin1().constData(), m_username.Get());
      // sorry, gotta kill this connection
      mpb_server_auth_reply bh;
      bh.errmsg="server full";
      Send(bh.build());
      return 0;
    }
  }



  qDebug("%s: Accepted user: %s",
         name.toLatin1().constData(), m_username.Get());

  {
    mpb_server_auth_reply bh;
    bh.flag=1;
    int ch=m_max_channels;
    if (ch > MAX_USER_CHANNELS) ch=MAX_USER_CHANNELS;
    if (ch < 0) ch = 0;

    bh.maxchan = ch;

    bh.errmsg=m_username.Get();
    Send(bh.build());
  }

  m_auth_state=1;

  SendConfigChangeNotify(group->m_last_bpm,group->m_last_bpi);


  SendUserList();

  if (group->GetProtocol() == JAM_PROTO_JAMMR) {
    SendChatMessage(QStringList("PRIVS") << privsToString(m_auth_privs));
  }

  SendChatMessage(QStringList("TOPIC") << "" << group->m_topictext.Get());

  {
    mpb_chat_message newmsg;
    newmsg.parms[0]="JOIN";
    newmsg.parms[1]=m_username.Get();
    group->Broadcast(newmsg.build(),this);
  }

  emit authenticated();
  return 1;
}

// send user list to user
void User_Connection::SendUserList()
{
  mpb_server_userinfo_change_notify bh;

  int user;
  for (user = 0; user < group->m_users.GetSize(); user++)
  {
    User_Connection *u=group->m_users.Get(user);
    int channel;
    if (u && u->m_auth_state>0 && u != this) 
    {
      int acnt=0;
      for (channel = 0; channel < u->m_max_channels && channel < MAX_USER_CHANNELS; channel ++)
      {
        if (u->m_channels[channel].active)
        {
          bh.build_add_rec(1,channel,u->m_channels[channel].volume,u->m_channels[channel].panning,u->m_channels[channel].flags,
                            u->m_username.Get(),u->m_channels[channel].name.Get());
          acnt++;
        }
      }
      if (!acnt && !group->m_allow_hidden_users && u->m_max_channels && !(u->m_auth_privs & PRIV_HIDDEN)) // give users at least one channel
      {
          bh.build_add_rec(1,0,0,0,0,u->m_username.Get(),"");
      }
    }
  }       
  Send(bh.build());
}

void User_Connection::processMessage(Net_Message *msg)
{
  if (!m_auth_state)
  {
    mpb_client_auth_user authrep;
    int ver_min, ver_max;

    switch (group->GetProtocol()) {
    case JAM_PROTO_NINJAM:
      ver_min = PROTO_NINJAM_VER_MIN;
      ver_max = PROTO_NINJAM_VER_MAX;
      break;

    case JAM_PROTO_JAMMR:
      ver_min = PROTO_JAMMR_VER_MIN;
      ver_max = PROTO_JAMMR_VER_MAX;
      break;
    }

    // verify everything
    int          err_st = ( msg->get_type() != MESSAGE_CLIENT_AUTH_USER || authrep.parse(msg) || !authrep.username || !authrep.username[0] ) ? 1 : 0;
    if (!err_st) err_st = ( authrep.client_version < ver_min ||
                            authrep.client_version > ver_max ) ? 2 : 0;
    if (!err_st) err_st = ( group->m_licensetext.Get()[0] && !(authrep.client_caps & 1) ) ? 3 : 0;

    if (err_st)
    {
      static char *tab[] = { "invalid authorization reply", "incorrect client version", "license not agreed to" };
      mpb_server_auth_reply bh;
      bh.errmsg=tab[err_st-1];

      qDebug("%s: Refusing user, %s", name.toLatin1().constData(), bh.errmsg);

      Send(bh.build());
      m_netcon.Kill();
      return;
    }

    m_netcon.SetKeepAlive(group->m_keepalive); // restore default keepalive behavior since we got a response

    m_clientcaps=authrep.client_caps;

    delete m_lookup;
    m_lookup=group->CreateUserLookup?group->CreateUserLookup(authrep.username):NULL;

    m_auth_state = -1;

    if (m_lookup)
    {
      m_lookup->hostmask.Set(m_netcon.GetRemoteAddr().toString().toLocal8Bit().constData());
      memcpy(m_lookup->sha1buf_request,authrep.passhash,sizeof(m_lookup->sha1buf_request));
      connect(m_lookup, SIGNAL(completed()), this, SLOT(userLookupCompleted()));
      m_lookup->start();
    }
    return;
  } // !m_auth_state

  if (m_auth_state < 1) {
    return;
  }

  switch (msg->get_type())
  {
    case MESSAGE_CLIENT_SET_CHANNEL_INFO:
      {
        mpb_client_set_channel_info chi;
        if (!chi.parse(msg))
        {
          // update our channel list

          mpb_server_userinfo_change_notify mfmt;
          int mfmt_changes=0;
        
          int offs=0;
          short v;
          int p,f;
          int whichch=0;
          char *chnp=0;
          while ((offs=chi.parse_get_rec(offs,&chnp,&v,&p,&f))>0 && whichch < MAX_USER_CHANNELS && whichch < m_max_channels)
          {
            if (!chnp) chnp=""; 

            int doactive=!(f&0x80);

            // only if something changes, do we add it to the rec
            int hadch=!m_channels[whichch].active != !doactive;
            if (!hadch) hadch = strcmp(chnp,m_channels[whichch].name.Get());
            if (!hadch) hadch = m_channels[whichch].volume!=v;
            if (!hadch) hadch = m_channels[whichch].panning!=p;
            if (!hadch) hadch = m_channels[whichch].flags!=f;

            m_channels[whichch].active=doactive;
            m_channels[whichch].name.Set(chnp);
            m_channels[whichch].volume=v;
            m_channels[whichch].panning=p;
            m_channels[whichch].flags=f;

            if (hadch)
            {
              mfmt_changes++;
              mfmt.build_add_rec(m_channels[whichch].active,whichch,
                                m_channels[whichch].volume,
                                m_channels[whichch].panning,
                                m_channels[whichch].flags,
                                m_username.Get(),
                                m_channels[whichch].name.Get());
            }

            whichch++;
          }
          while (whichch < MAX_USER_CHANNELS)
          {
            m_channels[whichch].name.Set("");

            if (m_channels[whichch].active) // only send deactivate if it was previously active
            {
              m_channels[whichch].active=0;
              mfmt_changes++;
              mfmt.build_add_rec(0,whichch,
                                m_channels[whichch].volume,
                                m_channels[whichch].panning,
                                m_channels[whichch].flags,
                                m_username.Get(),
                                m_channels[whichch].name.Get());
            }

            whichch++;
          }
          if (!group->m_allow_hidden_users && m_max_channels && !(m_auth_privs&PRIV_HIDDEN))
          {
            for (whichch = 0; whichch < MAX_USER_CHANNELS && !m_channels[whichch].active; whichch ++);


            if (whichch == MAX_USER_CHANNELS) // if empty, tell the user about one channel
            {
              mfmt.build_add_rec(1,0,0,0,0,m_username.Get(),"");
              mfmt_changes++;
            }
          }


          if (mfmt_changes) group->Broadcast(mfmt.build(),this);
        }         
      }
    break;
    case MESSAGE_CLIENT_SET_USERMASK:
      {
        mpb_client_set_usermask umi;
        if (!umi.parse(msg))
        {
          int offs=0;
          char *unp=0;
          unsigned int fla=0;
          while ((offs=umi.parse_get_rec(offs,&unp,&fla))>0)
          {
            if (unp)
            {
              int x;
              for (x = 0; x < m_sublist.GetSize() && strcasecmp(unp,m_sublist.Get(x)->username.Get()); x ++);
              if (x == m_sublist.GetSize()) // add new
              {
                if (fla) // only add if we need to subscribe
                {
                  User_SubscribeMask *n=new User_SubscribeMask;
                  n->username.Set(unp);
                  n->channelmask = fla;
                  m_sublist.Add(n);
                }
              }
              else
              {
                if (fla) // update flag
                {
                  m_sublist.Get(x)->channelmask=fla;
                }
                else // remove
                {
                  delete m_sublist.Get(x);
                  m_sublist.Delete(x);
                }
              }
            }
          }
        }
      }
    break;
    case MESSAGE_CLIENT_UPLOAD_INTERVAL_BEGIN:
      {
        mpb_client_upload_interval_begin mp;
        if (!mp.parse(msg) && mp.chidx < m_max_channels)
        {
          char *myusername=m_username.Get();

          mpb_server_download_interval_begin nmb;
          nmb.chidx=mp.chidx;
          nmb.estsize=mp.estsize;
          nmb.fourcc=mp.fourcc;
          memcpy(nmb.guid,mp.guid,sizeof(nmb.guid));
          nmb.username = myusername;

          Net_Message *newmsg=nmb.build();

          static unsigned char zero_guid[16];


          if (mp.fourcc && memcmp(mp.guid,zero_guid,sizeof(zero_guid))) // zero = silence, so simply rebroadcast
          {
            User_TransferState *newrecv=new User_TransferState;
            newrecv->bytes_estimated=mp.estsize;
            newrecv->fourcc=mp.fourcc;
            memcpy(newrecv->guid,mp.guid,sizeof(newrecv->guid));

            if (group->m_logdir.Get()[0])
            {
              char fn[512];
              char guidstr[64];
              guidtostr(mp.guid,guidstr);

              char ext[8];
              type_to_string(mp.fourcc,ext);
              sprintf(fn,"%c/%s.%s",guidstr[0],guidstr,ext);

              WDL_String tmp(group->m_logdir.Get());                
              tmp.Append(fn);

              newrecv->fp = fopen(tmp.Get(),"wb");

              if (group->m_logfp)
              {
                // decide when to write new interval
                char *chn="?";
                if (mp.chidx >= 0 && mp.chidx < MAX_USER_CHANNELS) chn=m_channels[mp.chidx].name.Get();
                fprintf(group->m_logfp,"user %s \"%s\" %d \"%s\"\n",guidstr,myusername,mp.chidx,chn);
              }
            }
          
            m_recvfiles.Add(newrecv);
          }


          int user;
          for (user=0;user<group->m_users.GetSize(); user++)
          {
            User_Connection *u=group->m_users.Get(user);
            if (u && u != this)
            {
              int i;
              for (i=0; i < u->m_sublist.GetSize(); i ++)
              {
                User_SubscribeMask *sm=u->m_sublist.Get(i);
                if (!strcasecmp(sm->username.Get(),myusername))
                {
                  if (sm->channelmask & (1<<mp.chidx))
                  {
                    if (memcmp(mp.guid,zero_guid,sizeof(zero_guid))) // zero = silence, so simply rebroadcast
                    {
                      // add entry in send list
                      User_TransferState *nt=new User_TransferState;
                      memcpy(nt->guid,mp.guid,sizeof(nt->guid));
                      nt->bytes_estimated = mp.estsize;
                      nt->fourcc = mp.fourcc;
                      u->m_sendfiles.Add(nt);
                    }

                    u->Send(newmsg, false);
                  }
                  break;
                }
              }
            }
          }
          delete newmsg;
        }
      }
      //m_recvfiles
    break;
    case MESSAGE_CLIENT_UPLOAD_INTERVAL_WRITE:
      {
        mpb_client_upload_interval_write mp;
        if (!mp.parse(msg))
        {
          time_t now;
          time(&now);
          msg->set_type(MESSAGE_SERVER_DOWNLOAD_INTERVAL_WRITE); // we rely on the fact that the upload/download write messages are identical
                                                                 // though we may need to update this at a later date if we change things.

          int user,x;


          for (x = 0; x < m_recvfiles.GetSize(); x ++)
          {
            User_TransferState *t=m_recvfiles.Get(x);
            if (!memcmp(t->guid,mp.guid,sizeof(mp.guid)))
            {
              t->last_acttime=now;

              if (t->fp) fwrite(mp.audio_data,1,mp.audio_data_len,t->fp);

              t->bytes_sofar+=mp.audio_data_len;
              if (mp.flags & 1)
              {
                delete t;
                m_recvfiles.Delete(x);
              }
              break;
            }
            if (now-t->last_acttime > TRANSFER_TIMEOUT)
            {
              delete t;
              m_recvfiles.Delete(x--);
            }
          }


          for (user=0;user<group->m_users.GetSize(); user++)
          {
            User_Connection *u=group->m_users.Get(user);
            if (u && u != this)
            {
              int i;
              for (i=0; i < u->m_sendfiles.GetSize(); i ++)
              {
                User_TransferState *t=u->m_sendfiles.Get(i);
                if (t && !memcmp(t->guid,mp.guid,sizeof(t->guid)))
                {
                  t->last_acttime=now;
                  t->bytes_sofar += mp.audio_data_len;
                  u->Send(msg, false);
                  if (mp.flags & 1)
                  {
                    delete t;
                    u->m_sendfiles.Delete(i);
                    // remove from transfer list
                  }
                  break;
                }
                if (now-t->last_acttime > TRANSFER_TIMEOUT)
                {
                  delete t;
                  u->m_sendfiles.Delete(i--);
                }
              }
            }
          }
        }
      }
    break;

    case MESSAGE_CHAT_MESSAGE:
      {
        mpb_chat_message poo;
        if (!poo.parse(msg))
        {
          group->onChatMessage(this,&poo);
        }
      }
    break;

    default:
    break;
  }
}

void User_Connection::netconMessagesReady()
{
  while (m_netcon.hasMessagesAvailable()) {
    Net_Message *msg = m_netcon.nextMessage();
    processMessage(msg);
    delete msg;
  }
}

void User_Connection::authenticationTimeout()
{
  qDebug("%s: Got an authorization timeout", name.toLatin1().constData());
  mpb_server_auth_reply bh;
  bh.errmsg = "authorization timeout";
  Send(bh.build());
  m_netcon.Kill();
}

void User_Connection::userLookupCompleted()
{
  authenticationTimer.stop();
  if (!OnRunAuth())
  {
    m_netcon.Kill();
  }
  delete m_lookup;
  m_lookup=0;
}


User_Group::User_Group(CreateUserLookupFn *CreateUserLookup_, QObject *parent)
  : QObject(parent), CreateUserLookup(CreateUserLookup_), m_max_users(0),
    m_last_bpm(120), m_last_bpi(32), m_keepalive(0), m_voting_threshold(110),
    m_voting_timeout(120), m_allow_hidden_users(0), m_logfp(0),
    protocol(JAM_PROTO_NINJAM), m_loopcnt(0)
{
  connect(&signalMapper, SIGNAL(mapped(QObject*)),
          this, SLOT(userConDisconnected(QObject*)));

  connect(&intervalTimer, SIGNAL(timeout()),
          this, SLOT(intervalExpired()));
}

User_Group::~User_Group()
{
  int x;
  for (x = 0; x < m_users.GetSize(); x ++)
  {
    delete m_users.Get(x);
  }
  m_users.Empty();
  SetLogDir(NULL);
}


void User_Group::SetLogDir(const char *path) // NULL to not log
{
  m_loopcnt = 0;
  intervalTimer.stop();

  if (m_logfp) {
    fprintf(m_logfp, "end\n");
    fclose(m_logfp);
  }
  m_logfp = NULL;

  if (m_logdir.Get()[0]) {
    qDebug("Finished archiving session '%s'", m_logdir.Get());
  }
  m_logdir.Set("");

  if (!path || !*path)
  {
    return;
  }

#ifdef _WIN32
    CreateDirectory(path,NULL);
#else
    mkdir(path,0755);
#endif

  m_logdir.Set(path);
  m_logdir.Append("/");

  WDL_String cl(path);
  cl.Append("/clipsort.log");
  m_logfp=fopen(cl.Get(),"at");

  int a;
  for (a = 0; a < 16; a ++)
  {
    WDL_String tmp(path);
    char buf[5];
    sprintf(buf,"/%x",a);
    tmp.Append(buf);
#ifdef _WIN32
    CreateDirectory(tmp.Get(),NULL);
#else
    mkdir(tmp.Get(),0755);
#endif
  }

  intervalExpired();
  intervalTimer.start();

  qDebug("Archiving session '%s'", path);
}

void User_Group::intervalExpired()
{
  m_loopcnt++;

  if (m_logfp) {
    fprintf(m_logfp, "interval %d %d %d\n", m_loopcnt, m_last_bpm, m_last_bpi);
    fflush(m_logfp);
  }
  intervalTimer.setInterval(m_last_bpi * 1000 * 60 / m_last_bpm);
}

void User_Group::SetProtocol(JamProtocol proto)
{
  protocol = proto;
}

JamProtocol User_Group::GetProtocol() const
{
  return protocol;
}

void User_Group::Broadcast(Net_Message *msg, User_Connection *nosend)
{
  if (msg)
  {
    int x;
    for (x = 0; x < m_users.GetSize(); x ++)
    {
      User_Connection *p=m_users.Get(x);
      if (p && p->m_auth_state > 0 && p != nosend)
      {
        p->Send(msg, false);
      }
    }

    delete msg;
  }
}

void User_Group::userConDisconnected(QObject *userObj)
{
  User_Connection *p = (User_Connection*)userObj;

  Q_ASSERT(p);

  // broadcast to other users that this user is no longer present
  if (p->m_auth_state>0) 
  {
    mpb_chat_message newmsg;
    newmsg.parms[0]="PART";
    newmsg.parms[1]=p->m_username.Get();
    Broadcast(newmsg.build(),p);

    mpb_server_userinfo_change_notify mfmt;
    int mfmt_changes=0;

    int whichch=0;
    while (whichch < MAX_USER_CHANNELS)
    {
      p->m_channels[whichch].name.Set("");

      if (!whichch || p->m_channels[whichch].active) // only send deactivate if it was previously active
      {
        p->m_channels[whichch].active=0;
        mfmt_changes++;
        mfmt.build_add_rec(0,whichch,
            p->m_channels[whichch].volume,
            p->m_channels[whichch].panning,
            p->m_channels[whichch].flags,
            p->m_username.Get(),
            p->m_channels[whichch].name.Get());
      }

      whichch++;
    }

    if (mfmt_changes) Broadcast(mfmt.build(),p);
  }

  qDebug("%s: Disconnected", p->name.toLatin1().constData());

  int idx = m_users.Find(p);
  Q_ASSERT(idx != -1);
  m_users.Delete(idx);

  p->deleteLater();
  emit userDisconnected();
}

void User_Group::SetConfig(int bpi, int bpm)
{
  m_last_bpi=bpi;
  m_last_bpm=bpm;
  mpb_server_config_change_notify mk;
  mk.beats_interval=bpi;
  mk.beats_minute=bpm;
  Broadcast(mk.build());
}

void User_Group::AddConnection(QTcpSocket *sock, int isres)
{
  User_Connection *p = new User_Connection(sock, this);
  if (isres) {
    p->m_reserved = 1;
  }
  m_users.Add(p);
  signalMapper.setMapping(p, p);
  connect(p, SIGNAL(disconnected()), &signalMapper, SLOT(map()));
  connect(p, SIGNAL(authenticated()), this, SIGNAL(userAuthenticated()));
}

void User_Group::onChatMessage(User_Connection *con, mpb_chat_message *msg)
{
  if (!strcmp(msg->parms[0],"MSG")) // chat message
  {
    WDL_PtrList<Net_Message> need_bcast;
    if (msg->parms[1] && !strncmp(msg->parms[1],"!vote",5)) // chat message
    {
      if (m_voting_threshold > 100 || m_voting_threshold < 1) {
        con->SendChatMessage(QStringList("MSG") << "" << "[voting system] Voting not enabled");
        return;
      } else if (!(con->m_auth_privs & PRIV_VOTE)) {
        con->SendChatMessage(QStringList("MSG") << "" << "[voting system] No vote permission");
        return;
      }
      char *p=msg->parms[1];
      while (*p && *p != ' ') p++;
      while (*p == ' ') p++;
      char *pn=p;
      while (*p && *p != ' ') p++;
      while (*p == ' ') p++;

      if (*p && !strncmp(pn,"bpm",3) && atoi(p) >= MIN_BPM && atoi(p) <= MAX_BPM)
      {
        con->m_vote_bpm=atoi(p);
        con->m_vote_bpm_lasttime=time(NULL);
      }
      else if (*p && !strncmp(pn,"bpi",3) && atoi(p) >= MIN_BPI && atoi(p) <= MAX_BPI)
      {
        con->m_vote_bpi=atoi(p);
        con->m_vote_bpi_lasttime=time(NULL);
      }
      else
      {
        con->SendChatMessage(QStringList("MSG") << "" <<
            "[voting system] !vote requires <bpm|bpi> <value> parameters");
        return;
      }
      // print voting stats
      {
        int bpis[1+MAX_BPI-MIN_BPI]={0,};
        int bpms[1+MAX_BPM-MIN_BPM]={0,};
        int x;
        int maxbpi=0, maxbpm=0;
        int vucnt=0;

        time_t now=time(NULL);
        for (x = 0; x < m_users.GetSize(); x ++)
        {
          User_Connection *p=m_users.Get(x);
          if (p->m_auth_state<=0) continue;
          if (!(p->m_auth_privs & PRIV_HIDDEN)) vucnt++;
          if (p->m_vote_bpi_lasttime >= now-m_voting_timeout && p->m_vote_bpi >= MIN_BPI && p->m_vote_bpi <= MAX_BPI)
          {
              int v=++bpis[p->m_vote_bpi-MIN_BPI];
              if (v > bpis[maxbpi]) maxbpi=p->m_vote_bpi-MIN_BPI;
          }
          if (p->m_vote_bpm_lasttime >= now-m_voting_timeout && p->m_vote_bpm >= MIN_BPM && p->m_vote_bpm <= MAX_BPM)
          {
              int v=++bpms[p->m_vote_bpm-MIN_BPM];
              if (v > bpms[maxbpm]) maxbpm=p->m_vote_bpm-MIN_BPM;
          }
        }
        if (bpms[maxbpm] > 0 && !strncmp(pn,"bpm",3))
        {
          if (bpms[maxbpm] >= (vucnt * m_voting_threshold + 50)/100)
          {
            m_last_bpm=maxbpm+MIN_BPM;
            char buf[512];
            sprintf(buf,"[voting system] setting BPM to %d",m_last_bpm);

            mpb_chat_message newmsg;
            newmsg.parms[0]="MSG";
            newmsg.parms[1]="";
            newmsg.parms[2]=buf;
            need_bcast.Add(newmsg.build());



            mpb_server_config_change_notify mk;
            mk.beats_interval=m_last_bpi;
            mk.beats_minute=m_last_bpm;
            need_bcast.Add(mk.build());
            for (x = 0; x < m_users.GetSize(); x ++)
              m_users.Get(x)->m_vote_bpm_lasttime = 0;
          }
          else
          {
            char buf[512];
            sprintf(buf,"[voting system] leading candidate: %d/%d votes for %d BPM [each vote expires in %ds]",bpms[maxbpm],(vucnt * m_voting_threshold + 50)/100,maxbpm+MIN_BPM,m_voting_timeout);

            mpb_chat_message newmsg;
            newmsg.parms[0]="MSG";
            newmsg.parms[1]="";
            newmsg.parms[2]=buf;
            need_bcast.Add(newmsg.build());
          }
        }
        if (bpis[maxbpi] > 0 && !strncmp(pn,"bpi",3))
        {
          if (bpis[maxbpi] >= (vucnt * m_voting_threshold + 50)/100)
          {
            m_last_bpi=maxbpi+MIN_BPI;
            char buf[512];
            sprintf(buf,"[voting system] setting BPI to %d",m_last_bpi);

            mpb_chat_message newmsg;
            newmsg.parms[0]="MSG";
            newmsg.parms[1]="";
            newmsg.parms[2]=buf;
            need_bcast.Add(newmsg.build());

            mpb_server_config_change_notify mk;
            mk.beats_interval=m_last_bpi;
            mk.beats_minute=m_last_bpm;
            need_bcast.Add(mk.build());
            for (x = 0; x < m_users.GetSize(); x ++)
              m_users.Get(x)->m_vote_bpi_lasttime = 0;
          }
          else
          {
            char buf[512];
            sprintf(buf,"[voting system] leading candidate: %d/%d votes for %d BPI [each vote expires in %ds]",bpis[maxbpi],(vucnt * m_voting_threshold + 50)/100,maxbpi+MIN_BPI,m_voting_timeout);

            mpb_chat_message newmsg;
            newmsg.parms[0]="MSG";
            newmsg.parms[1]="";
            newmsg.parms[2]=buf;
            need_bcast.Add(newmsg.build());
          }
        }
      }
     

    }

    if (!(con->m_auth_privs & PRIV_CHATSEND))
    {
      con->SendChatMessage(QStringList("MSG") << "" << "No MSG Permission");
    }
    else if (msg->parms[1] && *msg->parms[1])
    {
      mpb_chat_message newmsg;
      newmsg.parms[0]="MSG";
      newmsg.parms[1]=con->m_username.Get();
      newmsg.parms[2]=msg->parms[1];
      Broadcast(newmsg.build());
    }
    int x;
    for (x = 0; x < need_bcast.GetSize(); x ++)
      Broadcast(need_bcast.Get(x));
    need_bcast.Empty();
  }
  else if (!strcmp(msg->parms[0],"SESSION")) // session block descriptor message
  {
    mpb_chat_message newmsg;
    newmsg.parms[0]="SESSION";
    newmsg.parms[1]=con->m_username.Get();
    newmsg.parms[2]=msg->parms[1]; // guid
    newmsg.parms[3]=msg->parms[2]; // index
    newmsg.parms[4]=msg->parms[3]; // offset, length
    Broadcast(newmsg.build(),con);
  }
  else if (!strcmp(msg->parms[0],"PRIVMSG")) // chat message
  {
    if (!(con->m_auth_privs & PRIV_CHATSEND))
    {
      con->SendChatMessage(QStringList("MSG") << "" << "No PRIVMSG permission");
    }
    else if (msg->parms[1] && *msg->parms[1] && msg->parms[2] && *msg->parms[2])
    {
      // send a privmsg to user in parm1, and if they don't
      int x;
      for (x = 0; x < m_users.GetSize(); x ++)
      {
        if (!strcasecmp(msg->parms[1],m_users.Get(x)->m_username.Get()))
        {
          m_users.Get(x)->SendChatMessage(QStringList("PRIVMSG") <<
                                          con->m_username.Get() <<
                                          msg->parms[2]);
          return;
        }
      }

      // send a privmsg back to sender, saying shit aint there
      con->SendChatMessage(QStringList("MSG") << "" <<
                           QString("No such user: %1").arg(msg->parms[1]));
    }
  }
  else if (!strcmp(msg->parms[0],"ADMIN")) // admin message
  {
    char *adminerr="ADMIN requires valid parameter, i.e. topic, kick, bpm, bpi";
    if (msg->parms[1] && *msg->parms[1])
    {
      if (!strncasecmp(msg->parms[1],"topic ",6))
      {
        if (!(con->m_auth_privs & PRIV_TOPIC))
        {
          con->SendChatMessage(QStringList("MSG") << "" <<
                               "No TOPIC permission");
        }
        else
        {
          // set topic, notify everybody of topic change
          char *p=msg->parms[1]+6;
          while (*p == ' ') p++;
          if (*p)
          {
            m_topictext.Set(p);
            mpb_chat_message newmsg;
            newmsg.parms[0]="TOPIC";
            newmsg.parms[1]=con->m_username.Get();
            newmsg.parms[2]=m_topictext.Get();
            Broadcast(newmsg.build());
          }
        }

      }
      else if (!strncasecmp(msg->parms[1],"kick ",5))
      {
        if (!(con->m_auth_privs & PRIV_KICK))
        {
          con->SendChatMessage(QStringList("MSG") << "" <<
                               "No KICK permission");
        }
        else
        {
          // set topic, notify everybody of topic change
          char *p=msg->parms[1]+5;
          while (*p == ' ') p++;
          if (*p)
          {
            // try to kick user
            int x;
            int killcnt=0;
            int pl=strlen(p);
            for (x = 0; x < m_users.GetSize(); x ++)
            {
              User_Connection *c=m_users.Get(x);
              if ((p[pl-1] == '*' && !strncasecmp(c->m_username.Get(),p,pl-1)) || !strcasecmp(c->m_username.Get(),p))
              {
                if (c != con)
                {
                  WDL_String buf("User ");
                  buf.Append(c->m_username.Get());
                  buf.Append(" kicked by ");
                  buf.Append(con->m_username.Get());

                  mpb_chat_message newmsg;
                  newmsg.parms[0]="MSG";
                  newmsg.parms[1]="";
                  newmsg.parms[2]=buf.Get();
                  Broadcast(newmsg.build());

                  c->m_netcon.Kill();
                }
                killcnt++;
              }
            }
            if (!killcnt)
            {
              con->SendChatMessage(QStringList("MSG") << "" <<
                                   QString("User \"%1\" not found!").arg(p));
            }

          }
        }

      }
      else if (!strncasecmp(msg->parms[1],"bpm ",4) || !strncasecmp(msg->parms[1],"bpi ",4))
      {
        if (!(con->m_auth_privs & PRIV_BPM))
        {
          con->SendChatMessage(QStringList("MSG") << "" <<
                               "No BPM/BPI permission");
        }
        else
        {
          int isbpm=tolower(msg->parms[1][2])=='m';

          char *p=msg->parms[1]+4;
          while (*p == ' ') p++;
          int v=atoi(p);
          if (isbpm && (v < 20 || v > 400))
          {
            con->SendChatMessage(QStringList("MSG") << "" <<
                                 "BPM parameter must be between 20 and 400");
          }
          else if (!isbpm && (v < 2 || v > 1024))
          {
            con->SendChatMessage(QStringList("MSG") << "" <<
                                 "BPI parameter must be between 2 and 1024");
          }
          else
          {
            if (isbpm) m_last_bpm=v;
            else m_last_bpi=v;

            mpb_server_config_change_notify mk;
            mk.beats_interval=m_last_bpi;
            mk.beats_minute=m_last_bpm;
            Broadcast(mk.build());

            WDL_String str(con->m_username.Get());
            str.Append(" sets ");
            if (isbpm) str.Append("BPM"); else str.Append("BPI");
            str.Append(" to ");
            char buf[64];
            sprintf(buf,"%d",v);
            str.Append(buf);

            mpb_chat_message newmsg;
            newmsg.parms[0]="MSG";
            newmsg.parms[1]="";
            newmsg.parms[2]=str.Get();
            Broadcast(newmsg.build());
          }
        }

      }
      else
      {
        con->SendChatMessage(QStringList("MSG") << "" << adminerr);
      }
    }
    else
    {
      con->SendChatMessage(QStringList("MSG") << "" << adminerr);
    }
  }
  else // unknown message
  {
  }
}

int User_Group::numAuthenticatedUsers()
{
  int n = 0;
  int x;
  for (x = 0; x < m_users.GetSize(); x++) {
    if (m_users.Get(x)->m_auth_state >= 1) {
      n++;
    }
  }
  return n;
}
