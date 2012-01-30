/*
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

#include <stdio.h>
#include <stdlib.h>
#include <portaudio.h>
#include "audiostream.h"

class PortAudioStreamer : public audioStreamer
{
public:
  PortAudioStreamer(SPLPROC proc);
  ~PortAudioStreamer();

  bool Start();
  void Stop();

  const char *GetChannelName(int idx);

  int streamCallback(const void *input, void *output,
    unsigned long frameCount, const PaStreamCallbackTimeInfo *timeInfo,
    PaStreamCallbackFlags statusFlags);

  static int streamCallbackTrampoline(const void *input, void *output,
      unsigned long frameCount, const PaStreamCallbackTimeInfo *timeInfo,
      PaStreamCallbackFlags statusFlags, void *userData);

private:
  SPLPROC splproc;
  PaStream *stream;
};

const char *PortAudioStreamer::GetChannelName(int idx)
{
  if (idx >= m_innch) {
    return NULL;
  }

  /* TODO make GetChannelName() reentrancy-safe */
  static char name[64];
  snprintf(name, sizeof name, "Channel %d", idx);
  return name;
}

int PortAudioStreamer::streamCallback(const void *input, void *output,
    unsigned long frameCount, const PaStreamCallbackTimeInfo *timeInfo,
    PaStreamCallbackFlags statusFlags)
{
  float **inbuf = (float**)input; // const-cast due to SPLPROC prototype
  float **outbuf = static_cast<float**>(output);

  splproc(inbuf, m_innch, outbuf, m_outnch, frameCount, m_srate);
  return paContinue;
}

int PortAudioStreamer::streamCallbackTrampoline(const void *input, void *output,
    unsigned long frameCount, const PaStreamCallbackTimeInfo *timeInfo,
    PaStreamCallbackFlags statusFlags, void *userData)
{
  PortAudioStreamer *streamer = static_cast<PortAudioStreamer*>(userData);

  return streamer->streamCallback(input, output, frameCount, timeInfo, statusFlags);
}

PortAudioStreamer::PortAudioStreamer(SPLPROC proc)
  : splproc(proc), stream(NULL)
{
}

PortAudioStreamer::~PortAudioStreamer()
{
  Stop();
}

/* Returns true on success, false on failure */
bool PortAudioStreamer::Start()
{
  PaError error;

  // TODO works for all host APIs/devices?
  PaSampleFormat sampleFormat = paFloat32 | paNonInterleaved;

  m_innch = m_outnch = 1;
  m_bps = 32;
  error = Pa_OpenDefaultStream(&stream, m_innch, m_outnch,
                               sampleFormat, m_srate,
                               paFramesPerBufferUnspecified,
                               streamCallbackTrampoline, this);
  if (error != paNoError) {
    fprintf(stderr, "Pa_OpenDefaultStream: %s\n", Pa_GetErrorText(error));
    stream = NULL;
    return false;
  }

  error = Pa_StartStream(stream);
  if (error != paNoError) {
    fprintf(stderr, "Pa_StartStream: %s\n", Pa_GetErrorText(error));
    Pa_CloseStream(stream);
    stream = NULL;
    return false;
  }

  return true;
}

void PortAudioStreamer::Stop()
{
  if (stream) {
    Pa_CloseStream(stream);
    stream = NULL;
  }
}

audioStreamer *create_audioStreamer_PortAudio(SPLPROC proc)
{
  PortAudioStreamer *streamer = new PortAudioStreamer(proc);

  if (!streamer->Start()) {
    delete streamer;
    return NULL;
  }
  return streamer;
}
