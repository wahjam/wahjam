/*
    Copyright (C) 2005 Cockos Incorporated

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

  This header is used by Wahjam clients to define an abstract audio streamer interface, as
  well as declare functions for creating instances of these audio streamers. 

  The basic structure is:

  The client runs, creates an audiostreamer (below), giving it a SPLPROC, which is it's
  own function that then in turn calls NJClient::AudioProc. 

  But this is just the interface declaration etc.

*/

#ifndef _AUDIOSTREAM_H_
#define _AUDIOSTREAM_H_

class audioStreamer
{
	public:
		audioStreamer() { m_srate=48000; m_outnch=m_innch=2; m_bps=16; }
		virtual ~audioStreamer() { }

    virtual const char *GetChannelName(int idx)=0;

		int m_srate, m_innch, m_outnch, m_bps;
};

typedef void (*SPLPROC)(float **inbuf, int innch, float **outbuf, int outnch, int len, int srate);

audioStreamer *create_audioStreamer_PortAudio(const char *hostAPI,
    const char *inputDevice, const char *outputDevice,
    double sampleRate, double latency, SPLPROC proc);
bool portAudioInit();

#endif
