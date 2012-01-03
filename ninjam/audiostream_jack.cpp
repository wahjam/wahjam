/*
    NINJAM - audiostream_alsa.cpp
    Copyright (C) 2004-2005 Cockos Incorporated

    NINJAM is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    NINJAM is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with NINJAM; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*

  This file implements a audioStreamer that uses ALSA.
  It only exposes the following functions:

    audioStreamer *create_audioStreamer_ALSA(char *cfg, SPLPROC proc);
  
    cfg is a string that has a list of parameter/value pairs (space delimited) 
    for the config:
      in     - input device i.e. hw:0,0
      out    - output device i.e. hw:0,0
      srate  - sample rate i.e. 48000
      bps    - bits/sample i.e. 16
      nch    - channels i.e. 2
      bsize  - block size (bytes) i.e. 2048
      nblock - number of blocks i.e. 16


  (everything else in this file is used internally)

*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>

#include <jack/jack.h>

#include "../WDL/pcmfmtcvt.h"

#include "../WDL/ptrlist.h"
#include "../WDL/mutex.h"

#include "njclient.h"

#include "audiostream.h"

static void audiostream_onunder() { }
static void audiostream_onover() { }


const int c_ppqn = 192;


class audioStreamer_JACK : public audioStreamer
{
public:
  audioStreamer_JACK();
  ~audioStreamer_JACK();

  bool init(const char* clientName,
	    int nInputChannels,
	    int nOutputChannels,
	    SPLPROC proc);
  int process( jack_nframes_t nframes );
  void timebase_cb(jack_transport_state_t state, jack_nframes_t nframes, jack_position_t *pos, int new_pos );
  
  
  const char *GetChannelName(int idx) {
    return GetInputChannelName(idx);
  }
  
  const char *GetInputChannelName(int idx)
  {
    if ((idx >= 0)&&(idx < m_innch)) {
      return jack_port_short_name(_in[idx]);
    } else
      return NULL;
  }
  
  const char *GetOutputChannelName(int idx)
  {
    if ((idx >= 0)&&(idx < m_outnch)) {
      return jack_port_short_name(_out[idx]);
    } else
      return NULL;
  }
private:
  jack_client_t *client;
  jack_port_t **_in;
  jack_port_t **_out;
  float **_inports;
  float **_outports;
  SPLPROC splproc;
  NJClient *njc;
  WDL_Mutex _process_lock;

public:
  void set_njclient( NJClient *njclient ) { njc = njclient; }
  virtual bool addInputChannel();
  virtual bool addOutputChannel();
};


int
process_cb( jack_nframes_t nframes, audioStreamer_JACK *as ) {
    return as->process( nframes );
}

void jack_timebase_cb(jack_transport_state_t state,
		      jack_nframes_t nframes,
		      jack_position_t *pos,
		      int new_pos,
		      audioStreamer_JACK *as)
{
    as->timebase_cb( state, nframes, pos, new_pos );
}



//////////////// JACK driver
audioStreamer_JACK::audioStreamer_JACK()
  : client(NULL),
    _in(NULL),
    _out(NULL),
    _inports(NULL),
    _outports(NULL),
    splproc(NULL),
    njc(NULL)
{
}

audioStreamer_JACK::~audioStreamer_JACK() 
{
  // jack_deactivate(client);
    jack_client_close(client);
    sleep(1);
    delete[] _in;
    delete[] _inports;
    delete[] _out;
    delete[] _outports;
}

bool audioStreamer_JACK::init(const char* clientName,
			      int nInputChannels,
			      int nOutputChannels,
			      SPLPROC proc)
{

    njc = NULL;
    splproc = proc;

    if (client) {
      jack_client_close(client);
      client = NULL;
    }
    if ((client = jack_client_new(clientName)) == 0) {
      fprintf (stderr, "jack server not running?\n");
      return false;
      // exit(20);
    }
    
    jack_set_process_callback (client, (JackProcessCallback) process_cb, this);

    jack_set_timebase_callback( client, 0, (JackTimebaseCallback) jack_timebase_cb, this );
    
    if (_out) {
      delete[] _out;
      _out = NULL;
    }
    _out = new jack_port_t*[nOutputChannels];
    if (_outports) {
      delete[] _outports;
      _outports = NULL;
    }
    _outports = new float*[nOutputChannels];
    for (unsigned i=0; i<nOutputChannels; i++) {
      char name[10];
      snprintf(name, sizeof(name), "out%d", i+1);
      _out[i] = jack_port_register (client, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    }

    if (_in) {
      delete[] _in;
      _in = NULL;
    }
    _in = new jack_port_t*[nInputChannels];
    if (_inports) {
      delete[] _inports;
      _inports = NULL;
    }
    _inports = new float*[nInputChannels];
    for (unsigned i=0; i<nInputChannels; i++) {
      char name[10];
      snprintf(name, sizeof(name), "in%d", i+1);
      _in[i] = jack_port_register (client, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
    }

    if (jack_activate (client)) {
      fprintf (stderr, "cannot activate client\n");
      return false;
      //exit(20);
    }

    m_innch = nInputChannels;
    m_outnch = nOutputChannels;
    m_srate = jack_get_sample_rate( client );
    m_bps = 32;
    return true;
}

bool audioStreamer_JACK::addInputChannel()
{
  char name[10];
  snprintf(name, sizeof(name), "in%d", m_innch+1);
  jack_port_t* port = jack_port_register (client, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
  if (port) {
    jack_port_t** tmp = _in;
    jack_port_t** tmp2 = new jack_port_t*[m_innch+1];
    for (unsigned i=0; i < m_innch; i++)
      tmp2[i] = tmp[i];
    tmp2[m_innch] = port;
    _process_lock.Enter();
    _in = tmp2;
    _process_lock.Leave();
    delete[] tmp;
    float **tmp3 = _inports;
    float **tmp4 = new float*[m_innch+1];
    _process_lock.Enter();
    _inports = tmp4;
    m_innch++;
    _process_lock.Leave();
    delete[] tmp3;
    return true;
  } else {
    return false;
  }
}

bool audioStreamer_JACK::addOutputChannel()
{
  char name[10];
  snprintf(name, sizeof(name), "out%d", m_outnch+1);
  jack_port_t* port = jack_port_register (client, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
  if (port) {
    jack_port_t** tmp = _out;
    jack_port_t** tmp2 = new jack_port_t*[m_outnch+1];
    for (unsigned i=0; i < m_outnch; i++)
      tmp2[i] = tmp[i];
    tmp2[m_outnch] = port;
    _process_lock.Enter();
    _out = tmp2;
    _process_lock.Leave();
    delete[] tmp;
    float **tmp3 = _outports;
    float **tmp4 = new float*[m_outnch+1];
    _process_lock.Enter();
    _outports = tmp4;
    m_outnch++;
    _process_lock.Leave();
    delete[] tmp3;
    return true;
  } else {
    return false;
  }
}

int
audioStreamer_JACK::process( jack_nframes_t nframes ) {
  _process_lock.Enter();
  for (unsigned i=0; i<m_innch; i++) {
    _inports[i] = (float *) jack_port_get_buffer(_in[i], nframes);
  }
  for (unsigned i=0; i<m_outnch; i++) {
    _outports[i] = (float *) jack_port_get_buffer(_out[i], nframes);
  }

  splproc(_inports,
	  m_innch,
	  _outports,
	  m_outnch,
	  nframes,
	  jack_get_sample_rate(client));
  _process_lock.Leave();
  return 0;
}


void
audioStreamer_JACK::timebase_cb(jack_transport_state_t state, jack_nframes_t nframes, jack_position_t *pos, int new_pos ) {

    static double jack_tick;
    static jack_nframes_t last_frame;
    static jack_nframes_t current_frame;
    jack_position_t cur_pos;
    static jack_transport_state_t state_current;
    static jack_transport_state_t state_last;
    static int locating = 0;

#if 0
    if( state != JackTransportRolling )
    {
      jack_transport_locate( client, 0 );
      jack_transport_start( client );
    }
#endif

    if( !njc ) return;

    int posi, len;
    njc->GetPosition(&posi,&len);
    float bpm = njc->GetActualBPM();

    //posi = njc->GetSessionPosition() * m_srate / 1000;
    //len = (int)(njc->GetBPI() * m_srate * 60 / bpm);

    // sync jack_transport_frame to njframe
    
    //current_frame = jack_get_current_transport_frame( client );
    jack_transport_query(client, &cur_pos);
    current_frame = cur_pos.frame;

    // FIXME: This will not work right, if there are slow-sync clients....

    int diff = abs(current_frame % len) - (posi % len);

    if( diff > nframes ) {
#if 1
	jack_transport_locate( client, (current_frame / len) * len + (posi%len) + 2*nframes );
	
	//printf( "no:  current= %d diff = %d\n", (current_frame % len) -  (posi % len), diff );
#endif
    }

    // taken from seq24-0.7.0 perform.cpp

    state_current = state;


    //printf( "jack_timebase_callback() [%d] [%d] [%d]", state, new_pos, current_frame);
    
    pos->valid = JackPositionBBT;
    pos->beats_per_bar = 4;
    pos->beat_type = 4;
    pos->ticks_per_beat = c_ppqn * 10;    
    pos->beats_per_minute = bpm;
    
    
    /* Compute BBT info from frame number.  This is relatively
     * simple here, but would become complex if we supported tempo
     * or time signature changes at specific locations in the
     * transport timeline. */

    // if we are in a new position
    if (  state_last    ==  JackTransportStarting &&
          state_current ==  JackTransportRolling ){

        //printf ( "Starting [%d] [%d]\n", last_frame, current_frame );
        
        jack_tick = 0.0;
        last_frame = current_frame;
    }

    if ( current_frame > last_frame ){

        double jack_delta_tick =
            (current_frame - last_frame) *
            pos->ticks_per_beat *
            pos->beats_per_minute / (pos->frame_rate * 60.0);
        
        jack_tick += jack_delta_tick;

        last_frame = current_frame;
    }
    
    long ptick = 0, pbeat = 0, pbar = 0;
    
    pbar  = (long) ((long) jack_tick / (pos->ticks_per_beat *  pos->beats_per_bar ));
    
    pbeat = (long) ((long) jack_tick % (long) (pos->ticks_per_beat *  pos->beats_per_bar ));
    pbeat = pbeat / (long) pos->ticks_per_beat;
    
    ptick = (long) jack_tick % (long) pos->ticks_per_beat;
    
    pos->bar = pbar + 1;
    pos->beat = pbeat + 1;
    pos->tick = ptick;;
    pos->bar_start_tick = pos->bar * pos->beats_per_bar *
        pos->ticks_per_beat;

    //printf( " bbb [%2d:%2d:%4d]\n", pos->bar, pos->beat, pos->tick );

    state_last = state_current;

}

audioStreamer *create_audioStreamer_JACK(const char* clientName,
					 int nInputChannels,
					 int nOutputChannels,
					 SPLPROC proc,
					 NJClient *njclient)
{
  audioStreamer_JACK *au = new audioStreamer_JACK();
  if (!au->init(clientName,
		nInputChannels,
		nOutputChannels,
		proc)) {
    delete au;
    return NULL;
  } else {
    au->set_njclient(njclient);
    return au;
  }
}
