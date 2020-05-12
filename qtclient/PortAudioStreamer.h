/*
    Copyright (C) 2012-2017 Stefan Hajnoczi <stefanha@gmail.com>

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

#ifndef _AUDIOSTREAM_PA_H_
#define _AUDIOSTREAM_PA_H_

#include <stdlib.h>
#include <portaudio.h>
#include <QObject>
#include <QVariant>
#include <QList>

typedef void (*SPLPROC)(float **inbuf, int innch, float **outbuf, int outnch, int len, int srate);

class PortAudioStreamer : public QObject
{
  Q_OBJECT

public:
  PortAudioStreamer(SPLPROC proc);
  ~PortAudioStreamer();

  /* Returns true on success, false on failure */
  bool Start(const char *hostAPI,
             const char *inputDevice,
             const QList<QVariant> &inputChannels,
             const char *outputDevice,
             const QList<QVariant> &outputChannels,
             double sampleRate, double latency);
  void Stop();

  const char *GetChannelName(int idx);

  int streamCallback(const void *input, void *output,
    unsigned long frameCount, const PaStreamCallbackTimeInfo *timeInfo,
    PaStreamCallbackFlags statusFlags);

  void streamFinishedCallback();

  static int streamCallbackTrampoline(const void *input, void *output,
      unsigned long frameCount, const PaStreamCallbackTimeInfo *timeInfo,
      PaStreamCallbackFlags statusFlags, void *userData);

  static void streamFinishedCallbackTrampoline(void *userData);

signals:
  /* Emitted when the stream fails after starting. Note this signal can be
   * emitted from another thread!
   */
  void StoppedUnexpectedly();

private:
  SPLPROC splproc;
  PaStream *stream;
  float *inputMonoBuf;
  unsigned long inputMonoBufFrames;
  int numHWInputChannels;
  int numHWOutputChannels;
  bool stopping;

  /* Channel routing */
  size_t numInputChannels;
  int *inputChannels;
  size_t numOutputChannels;
  int *outputChannels;

  void initChannels(const QList<QVariant> &inputChannels,
                    const QList<QVariant> &outputChannels);
  void cleanupChannels();
  bool TryToStart(const char *hostAPI,
                  const char *inputDevice,
                  const QList<QVariant> &inputChannels,
                  const char *outputDevice,
                  const QList<QVariant> &outputChannels,
                  double sampleRate, double latency);
};

bool portAudioInit();

#endif /* _AUDIOSTREAM_PA_H_ */
