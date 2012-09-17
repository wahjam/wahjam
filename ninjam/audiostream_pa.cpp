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
#include <string.h>
#include <stdlib.h>
#include <portaudio.h>
#include <QtGlobal>
#include "audiostream.h"

class PortAudioStreamer : public audioStreamer
{
public:
  PortAudioStreamer(SPLPROC proc);
  ~PortAudioStreamer();

  bool Start(const PaStreamParameters *inputParams,
             const PaStreamParameters *outputParams);
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
bool PortAudioStreamer::Start(const PaStreamParameters *inputParams,
                              const PaStreamParameters *outputParams)
{
  PaError error;

  error = Pa_OpenStream(&stream, inputParams, outputParams,
                        m_srate, paFramesPerBufferUnspecified,
                        paPrimeOutputBuffersUsingStreamCallback,
                        streamCallbackTrampoline, this);
  if (error != paNoError) {
    qCritical("Pa_OpenStream() failed: %s", Pa_GetErrorText(error));
    stream = NULL;
    return false;
  }

  m_innch = inputParams->channelCount;
  m_outnch = outputParams->channelCount;
  m_bps = 32;

  error = Pa_StartStream(stream);
  if (error != paNoError) {
    qCritical("Pa_StartStream() failed: %s", Pa_GetErrorText(error));
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

static const PaHostApiInfo *findHostAPIInfo(const char *hostAPI, PaHostApiIndex *index)
{
  const PaHostApiInfo *hostAPIInfo = NULL;
  PaHostApiIndex i;

  for (i = 0; i < Pa_GetHostApiCount(); i++) {
    hostAPIInfo = Pa_GetHostApiInfo(i);

    if (hostAPIInfo && strcmp(hostAPI, hostAPIInfo->name) == 0) {
      break;
    }
  }
  if (i >= Pa_GetHostApiCount()) {
    i = Pa_GetDefaultHostApi();
    hostAPIInfo = Pa_GetHostApiInfo(i);
  }

  if (index) {
    *index = i;
  }
  return hostAPIInfo;
}

static const PaDeviceInfo *findDeviceInfo(PaHostApiIndex hostAPI, const char *name,
                                          PaDeviceIndex *index)
{
  const PaHostApiInfo *hostAPIInfo = Pa_GetHostApiInfo(hostAPI);
  if (!hostAPIInfo) {
    return NULL;
  }

  const PaDeviceInfo *deviceInfo;
  PaDeviceIndex deviceIndex;
  int i;
  for (i = 0; i < hostAPIInfo->deviceCount; i++) {
    deviceIndex = Pa_HostApiDeviceIndexToDeviceIndex(hostAPI, i);
    if (deviceIndex < 0) {
      continue;
    }

    deviceInfo = Pa_GetDeviceInfo(deviceIndex);
    if (deviceInfo && strcmp(name, deviceInfo->name) == 0) {
      break;
    }
  }
  if (i >= hostAPIInfo->deviceCount) {
    deviceIndex = hostAPIInfo->defaultInputDevice;
    deviceInfo = Pa_GetDeviceInfo(deviceIndex);
  }

  if (index) {
    *index = deviceIndex;
  }
  return deviceInfo;
}

static bool setupParameters(const char *hostAPI, const char *inputDevice,
    const char *outputDevice, PaStreamParameters *inputParams,
    PaStreamParameters *outputParams)
{
  PaHostApiIndex hostAPIIndex;
  if (!findHostAPIInfo(hostAPI, &hostAPIIndex)) {
    return false;
  }

  const PaDeviceInfo *inputDeviceInfo = findDeviceInfo(hostAPIIndex,
      inputDevice, &inputParams->device);
  if (!inputDeviceInfo) {
    return false;
  }

  const PaDeviceInfo *outputDeviceInfo = findDeviceInfo(hostAPIIndex,
      outputDevice, &outputParams->device);
  if (!outputDeviceInfo) {
    return false;
  }

  /* TODO do all host APIs/devices support this? */
  PaSampleFormat sampleFormat = paFloat32 | paNonInterleaved;
  inputParams->sampleFormat = sampleFormat;
  outputParams->sampleFormat = sampleFormat;

  /* TODO support stereo and user-defined channel configuration */
  inputParams->channelCount = 1;
  outputParams->channelCount = 1;

  inputParams->suggestedLatency = inputDeviceInfo->defaultLowInputLatency;
  outputParams->suggestedLatency = outputDeviceInfo->defaultLowOutputLatency;

  inputParams->hostApiSpecificStreamInfo = NULL;
  outputParams->hostApiSpecificStreamInfo = NULL;
  return true;
}

audioStreamer *create_audioStreamer_PortAudio(const char *hostAPI,
    const char *inputDevice, const char *outputDevice, SPLPROC proc)
{
  PaStreamParameters inputParams, outputParams;
  if (!setupParameters(hostAPI, inputDevice, outputDevice,
                       &inputParams, &outputParams)) {
    return NULL;
  }

  PortAudioStreamer *streamer = new PortAudioStreamer(proc);
  if (!streamer->Start(&inputParams, &outputParams)) {
    delete streamer;
    return NULL;
  }
  return streamer;
}

static void logPortAudioInfo()
{
  qDebug(Pa_GetVersionText());

  PaHostApiIndex api = 0;
  const PaHostApiInfo *apiInfo;
  while ((apiInfo = Pa_GetHostApiInfo(api))) {
    qDebug("Host API: %s (%d devices)", apiInfo->name, apiInfo->deviceCount);

    int device;
    for (device = 0; device < apiInfo->deviceCount; device++) {
      PaDeviceIndex devIdx = Pa_HostApiDeviceIndexToDeviceIndex(api, device);
      if (devIdx < 0) {
        qDebug("[%02d] Error: %s", device, Pa_GetErrorText(devIdx));
        continue;
      }

      const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(devIdx);
      if (!deviceInfo) {
        qDebug("[%02d] Invalid device index", device);
        continue;
      }

      qDebug("[%02d] \"%s\" (%d)", device, deviceInfo->name, devIdx);
      qDebug("     Channels: %d in, %d out",
          deviceInfo->maxInputChannels,
          deviceInfo->maxOutputChannels);
      qDebug("     Default sample rate: %g Hz",
          deviceInfo->defaultSampleRate);
      qDebug("     Input latency: %g low, %g high",
          deviceInfo->defaultLowInputLatency,
          deviceInfo->defaultHighInputLatency);
      qDebug("     Output latency: %g low, %g high",
          deviceInfo->defaultLowOutputLatency,
          deviceInfo->defaultHighOutputLatency);
    }
    api++;
  }
}

static void portAudioCleanup()
{
  Pa_Terminate();
}

/* Initialize PortAudio once for the whole application */
bool portAudioInit()
{
  PaError error = Pa_Initialize();
  if (error != paNoError) {
    qCritical("Pa_Initialize() failed: %s", Pa_GetErrorText(error));
    return false;
  }
  atexit(portAudioCleanup);

  logPortAudioInfo();
  return true;
}
