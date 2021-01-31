/*
    Copyright (C) 2005 Cockos Incorporated
    Copyright (C) 2020 Stefan Hajnoczi <stefanha@jammr.net>

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

  This file defines the interface for the NJClient class, which handles 
  the bulk of logic and state of the client. 

  The basic premise of the NJClient class is, the UI code tells NJClient
  when the user tweaks something, and NJClient tells the UI code when
  it needs to update something.

  Additionally, NJClient::AudioProc() needs to be called from the audio thread.
  It is not necessary to do any sort of mutex protection around these calls
  as they are done internally.


  Some other notes:

    + Currently only OGG Vorbis is supported. There's hooks in there to add support
      for more formats, but the requirements for the formats are a little high, so
      currently OGG Vorbis is the only thing we've seen that would work well. And it
      really rocks for this application.

    + OK maybe that's it for now? :)

*/

#ifndef _NJCLIENT_H_
#define _NJCLIENT_H_

#ifdef _WIN32
#include <windows.h>
#else
#include <stdlib.h>
#include <memory.h>
#endif
#include <stdio.h>
#include <time.h>
#include <portaudio.h>
#include <QObject>

#include "../WDL/string.h"
#include "../WDL/ptrlist.h"
#include "../WDL/mutex.h"

#include "common/netmsg.h"
#include "common/mpb.h"
#include "PortMidiStreamer.h"

class I_NJEncoder;
class RemoteDownload;
class RemoteUser;
class Local_Channel;
class DecodeBuffer;
class DecodeState;
class BufferQueue;

// #define NJCLIENT_NO_XMIT_SUPPORT // might want to do this for njcast :)

class NJClient : public QObject
{
  Q_OBJECT

  friend class RemoteDownload;
public:
  NJClient(QObject *parent = 0);
  ~NJClient();

  void Connect(char *host, char *user, char *pass);
  void Disconnect();
  void Reconnect();

  char *GetErrorStr() { return m_errstr.Get(); }

  int IsAudioRunning() { return m_audio_enable; }
  // call AudioProc, (and only AudioProc) from your audio thread
  void AudioProc(float **inbuf, int innch, float **outbuf, int outnch, int len, const PaStreamCallbackTimeInfo *timeInfo); // len is number of sample pairs or samples

  // basic configuration
  int   config_autosubscribe;
  float config_metronome,config_metronome_pan; // volume of metronome
  bool  config_metronome_mute;
  float config_mastervolume,config_masterpan; // master volume
  bool  config_mastermute;
  int   config_debug_level; 
  int   config_play_prebuffer; // -1 means play instantly, 0 means play when full file is there, otherwise refers to how many
                               // bytes of compressed source to have before play. the default value is 4096.

  float GetOutputPeak();

  enum {
    NJC_STATUS_PRECONNECT,
    NJC_STATUS_CONNECTING,
    NJC_STATUS_CANTCONNECT,
    NJC_STATUS_VERSIONMISMATCH,
    NJC_STATUS_AUTHENTICATING,
    NJC_STATUS_INVALIDAUTH,
    NJC_STATUS_OK,
    NJC_STATUS_DISCONNECTED,
  };
  int GetStatus();

  void SetProtocol(JamProtocol proto) { protocol = proto; }

  char *GetUserName() { return m_user.Get(); }
  char *GetHostName() { return m_host.Get(); }

  float GetActualBPM() { return (float) m_active_bpm; }
  int GetBPI() { return m_active_bpi; }
  void GetPosition(int *pos, int *length);  // positions in samples
  int GetSampleRate() { return m_srate; }
  void SetSampleRate(int srate);

  int GetNumUsers() { return m_remoteusers.GetSize(); }
  char *GetUserState(int idx, float *vol=0, float *pan=0, bool *mute=0);
  void SetUserState(int idx, bool setvol, float vol, bool setpan, float pan, bool setmute, bool mute);

  float GetUserChannelPeak(int useridx, int channelidx);
  char *GetUserChannelState(int useridx, int channelidx, bool *sub=0, float *vol=0, float *pan=0, bool *mute=0, bool *solo=0);
  void SetUserChannelState(int useridx, int channelidx, bool setsub, bool sub, bool setvol, float vol, bool setpan, float pan, bool setmute, bool mute, bool setsolo, bool solo);
  int EnumUserChannels(int useridx, int i); // returns <0 if out of channels. start with i=0, and go upwards

  int GetMaxLocalChannels() { return m_max_localch; }
  void DeleteLocalChannel(int ch);
  int EnumLocalChannels(int i);
  float GetLocalChannelPeak(int ch);
  void SetLocalChannelProcessor(int ch, void (*cbf)(float *, int ns, void *), void *inst);
  void GetLocalChannelProcessor(int ch, void **func, void **inst);
  void SetLocalChannelInfo(int ch, char *name, bool setsrcch, int srcch, bool setbitrate, int bitrate, bool setbcast, bool broadcast);
  char *GetLocalChannelInfo(int ch, int *srcch, int *bitrate, bool *broadcast);
  void SetLocalChannelMonitoring(int ch, bool setvol, float vol, bool setpan, float pan, bool setmute, bool mute, bool setsolo, bool solo);
  int GetLocalChannelMonitoring(int ch, float *vol, float *pan, bool *mute, bool *solo); // 0 on success
  void NotifyServerOfChannelChange(); // call after any SetLocalChannel* that occur after initial connect

  int IsASoloActive() { return m_issoloactive; }

  void SetMidiStreamer(PortMidiStreamer *midiStreamer_)
  {
    midiStreamer = midiStreamer_;
  }
  void SetSendMidiBeatClock(bool enable)
  {
    sendMidiBeatClock = enable;
  }
  void SetSendMidiStartOnInterval(bool enable)
  {
    sendMidiStartOnInterval = enable;
  }

  int LicenseAgreement_User32;
  int (*LicenseAgreementCallback)(int user32, char *licensetext); // return TRUE if user accepts


  // messages you can send:
  // "MSG" "text"  - broadcast "text" to everybody
  // "PRIVMSG" "username" "text"  - send text to "username"
  void ChatMessage_Send(char *parm1, char *parm2, char *parm3=NULL, char *parm4=NULL, char *parm5=NULL);

  // messages you can receive from this:
  // "MSG" "user" "text"   - message from user to everybody (including you!), or if user is empty, from the server
  // "PRIVMSG "user" "text"   - private message from user

  // usernames are not case sensitive, but message names ARE.

  // note that nparms is the MAX number of parms, you still can get NULL parms entries in there (though rarely)
  void (*ChatMessage_Callback)(int user32, NJClient *inst, char **parms, int nparms); 
  int ChatMessage_User32;

signals:
  void userInfoChanged();
  void statusChanged(int newStatus);
  void beatsPerMinuteChanged(int bpi);
  void beatsPerIntervalChanged(int bpi);
  void currentBeatChanged(int currentBeat);

protected:
  double output_peaklevel;

  void _reinit();

  void updateBPMinfo(int bpm, int bpi);
  void processInputChannels(float **inbuf, int innch,
                            float **outbuf, int outnch,
                            int len, bool justmonitor);
  void process_samples(float **outbuf, int outnch, int len, int offset,
                       int justmonitor, double outputBufferDacTime);
  void on_new_interval();
  void updateInterval(int nsamples);

  WDL_String m_errstr;

  int m_status;
  int m_max_localch;
  int m_connection_keepalive;

  WDL_String m_user, m_pass, m_host;

  JamProtocol protocol;

  int m_in_auth;
  int m_bpm,m_bpi;
  int m_beatinfo_updated;
  int m_audio_enable;
  int m_srate;
  int m_issoloactive;

  int m_active_bpm, m_active_bpi;
  int m_interval_length;
  int m_interval_pos, m_metronome_state, m_metronome_tmp,m_metronome_interval;
  double m_metronome_pos;

  bool sendMidiBeatClock;
  bool sendMidiStartOnInterval;
  bool midiStarted;
  PortMidiStreamer *midiStreamer;

  // Values that we watch for changes
  int lastBpm;
  int lastBpi;
  int lastBeat;

  WDL_PtrList<Local_Channel> m_locchannels;

  void mixInChannel(bool muted, float vol, float pan, DecodeState *chan, float **outbuf, int len, int outnch, int offs, double vudecay);

  WDL_Mutex m_users_cs, m_locchan_cs, m_log_cs, m_misc_cs;
  Net_Connection *m_netcon;
  WDL_PtrList<RemoteUser> m_remoteusers;
  WDL_PtrList<RemoteDownload> m_downloads;

  WDL_HeapBuf tmpblock;

private slots:
  void tick();
  void netconDisconnected();
  void netconMessagesReady();

private:
  int Run();// returns nonzero if sleep is OK
  void processMessage(Net_Message *msg);
  void sendMidiMessage(PmMessage msg, PmTimestamp timestamp);
  void sendMidiStop();
};



#define MAX_USER_CHANNELS 32
#define MAX_LOCAL_CHANNELS 32 // probably want to use NJClient::GetMaxLocalChannels() if determining when it's OK to add a channel,etc
#define DOWNLOAD_TIMEOUT 8


#endif//_NJCLIENT_H_
