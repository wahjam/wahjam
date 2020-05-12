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
#include <QtGlobal>
#include "PortAudioStreamer.h"

static void logPortAudioError(const char *msg, PaError error)
{
  const PaHostErrorInfo *info = Pa_GetLastHostErrorInfo();

  /* The documentation says Pa_GetLastHostErrorInfo() only returns valid data
   * after paUnanticipatedHostError but some hostapis set it even when
   * returning other error codes. Get as much information as we can but be
   * aware that host info may be stale.
   */
  qCritical("%s: %s: %s (%ld)",
            msg, Pa_GetErrorText(error), info->errorText, info->errorCode);
}

const char *PortAudioStreamer::GetChannelName(int idx)
{
  if (idx >= numHWInputChannels) {
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
  Q_UNUSED(timeInfo);
  Q_UNUSED(statusFlags);

  float **inbuf = (float**)input; // const-cast due to SPLPROC prototype
  float **outbuf = static_cast<float**>(output);
  const PaStreamInfo *info = Pa_GetStreamInfo(stream);

  /* Mix down to mono */
  if (numHWInputChannels > 1) {
    if (inputMonoBufFrames < frameCount) {
      /* Allocation should happen rarely so don't worry about real-time */
      delete [] inputMonoBuf;
      inputMonoBuf = new float[frameCount];
      inputMonoBufFrames = frameCount;
    }

    if (numInputChannels > 0) {
      int channel = inputChannels[0];

      for (unsigned long i = 0; i < frameCount; i++) {
        inputMonoBuf[i] = inbuf[channel][i];
      }

      for (size_t chidx = 1; chidx < numInputChannels; chidx++) {
        channel = inputChannels[chidx];

        for (unsigned long i = 0; i < frameCount; i++) {
          inputMonoBuf[i] += inbuf[channel][i];
        }
      }

      for (unsigned long i = 0; i < frameCount; i++) {
        inputMonoBuf[i] /= numInputChannels;
      }
    } else {
      memset(inputMonoBuf, 0, sizeof(float) * frameCount);
    }

    inbuf = &inputMonoBuf;
  }

  splproc(inbuf, 1, outbuf, 1, frameCount, info->sampleRate);

  /* Mix up to multi-channel audio */
  size_t chidx = 0;
  for (int channel = 1; channel < numHWOutputChannels; channel++) {
    while (chidx < numOutputChannels && channel > outputChannels[chidx]) {
      chidx++;
    }

    if (chidx >= numOutputChannels || channel < outputChannels[chidx]) {
      memset(outbuf[channel], 0, sizeof(float) * frameCount);
    } else {
      memcpy(outbuf[channel], outbuf[0], sizeof(float) * frameCount);
      chidx++;
    }
  }
  if (numOutputChannels > 0 && outputChannels[0] != 0) {
    /* Output was put into outbuf[0] but the channel is disabled, silence it */
    memset(outbuf[0], 0, sizeof(float) * frameCount);
  }

  return paContinue;
}

void PortAudioStreamer::streamFinishedCallback()
{
  if (!stopping) {
    emit StoppedUnexpectedly();
  }
}

int PortAudioStreamer::streamCallbackTrampoline(const void *input, void *output,
    unsigned long frameCount, const PaStreamCallbackTimeInfo *timeInfo,
    PaStreamCallbackFlags statusFlags, void *userData)
{
  PortAudioStreamer *streamer = static_cast<PortAudioStreamer*>(userData);

  return streamer->streamCallback(input, output, frameCount, timeInfo, statusFlags);
}

void PortAudioStreamer::streamFinishedCallbackTrampoline(void *userData)
{
  PortAudioStreamer *streamer = static_cast<PortAudioStreamer*>(userData);

  streamer->streamFinishedCallback();
}

PortAudioStreamer::PortAudioStreamer(SPLPROC proc)
  : splproc(proc), stream(NULL), inputMonoBuf(NULL), inputMonoBufFrames(0),
    numHWInputChannels(0), numHWOutputChannels(0), stopping(false),
    numInputChannels(0), inputChannels(NULL),
    numOutputChannels(0), outputChannels(NULL)
{
}

PortAudioStreamer::~PortAudioStreamer()
{
  Stop();
  delete [] inputMonoBuf;
}

/* Used for qsort(3) on ints */
static int compare_int(const void *a, const void *b)
{
  int i = *(const int *)a;
  int j = *(const int *)b;

  return i - j;
}

void PortAudioStreamer::Stop()
{
  if (stream) {
    stopping = true;

    /* Pa_AbortStream() can trigger a deadlock in the PulseAudio ALSA plugin,
     * so call Pa_StopStream() before Pa_CloseStream(). Pa_CloseStream()
     * implicitly aborts the stream if it's not already stopped.
     */
    PaError error = Pa_StopStream(stream);
    if (error != paNoError) {
      logPortAudioError("Pa_StopStream failed", error);
    }

    error = Pa_CloseStream(stream);
    stopping = false;

    if (error != paNoError) {
      logPortAudioError("Pa_CloseStream failed", error);
    }
    stream = NULL;
  }

  cleanupChannels();
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
                                          PaDeviceIndex *index, bool isInput)
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
    if (!deviceInfo) {
      continue;
    }
    if (isInput && deviceInfo->maxInputChannels == 0) {
      continue;
    }
    if (!isInput && deviceInfo->maxOutputChannels == 0) {
      continue;
    }
    if (strcmp(deviceInfo->name, name) == 0) {
      break;
    }
  }
  if (i >= hostAPIInfo->deviceCount) {
    deviceIndex = isInput ? hostAPIInfo->defaultInputDevice :
                            hostAPIInfo->defaultOutputDevice;
    deviceInfo = Pa_GetDeviceInfo(deviceIndex);
    if (deviceInfo) {
      qDebug("Unable to find %s device \"%s\", falling back to \"%s\" (%d)",
             isInput ? "input" : "output",
             name, deviceInfo->name, deviceIndex);
    } else {
      qDebug("Unable to find %s device \"%s\" and no fallback available",
             isInput ? "input" : "output", name);
    }
  }

  if (index) {
    *index = deviceIndex;
  }
  return deviceInfo;
}

void PortAudioStreamer::initChannels(const QList<QVariant> &inputChannels_,
                                     const QList<QVariant> &outputChannels_)
{
  numInputChannels = inputChannels_.count();
  inputChannels = new int[numInputChannels];
  int i = 0;
  for (const QVariant &channel : inputChannels_) {
    inputChannels[i++] = channel.toInt();
  }
  qsort(inputChannels, numInputChannels, sizeof(inputChannels[0]),
        compare_int);
  QStringList inputChannelsStr;
  for (size_t chidx = 0; chidx < numInputChannels; chidx++) {
    inputChannelsStr.append(QString::number(inputChannels[chidx]));
  }
  qDebug("Input channels: %s", inputChannelsStr.join(' ').toLatin1().constData());

  numOutputChannels = outputChannels_.count();
  outputChannels = new int[numOutputChannels];
  i = 0;
  for (const QVariant &channel : outputChannels_) {
    outputChannels[i++] = channel.toInt();
  }
  qsort(outputChannels, numOutputChannels, sizeof(outputChannels[0]),
        compare_int);
  QStringList outputChannelsStr;
  for (size_t chidx = 0; chidx < numOutputChannels; chidx++) {
    outputChannelsStr.append(QString::number(outputChannels[chidx]));
  }
  qDebug("Output channels: %s", outputChannelsStr.join(' ').toLatin1().constData());
}

void PortAudioStreamer::cleanupChannels()
{
  if (inputChannels) {
    delete [] inputChannels;
    inputChannels = nullptr;
    numInputChannels = 0;
  }

  if (outputChannels) {
    delete [] outputChannels;
    outputChannels = nullptr;
    numOutputChannels = 0;
  }
}

static bool setupParameters(const char *hostAPI, const char *inputDevice,
    const char *outputDevice, PaStreamParameters *inputParams,
    PaStreamParameters *outputParams, double latency)
{
  PaHostApiIndex hostAPIIndex;
  if (!findHostAPIInfo(hostAPI, &hostAPIIndex)) {
    return false;
  }

  const PaDeviceInfo *inputDeviceInfo = findDeviceInfo(hostAPIIndex,
      inputDevice, &inputParams->device, true);
  if (!inputDeviceInfo) {
    return false;
  }

  const PaDeviceInfo *outputDeviceInfo = findDeviceInfo(hostAPIIndex,
      outputDevice, &outputParams->device, false);
  if (!outputDeviceInfo) {
    return false;
  }

  /* TODO do all host APIs/devices support this? */
  PaSampleFormat sampleFormat = paFloat32 | paNonInterleaved;
  inputParams->sampleFormat = sampleFormat;
  outputParams->sampleFormat = sampleFormat;

  if (inputDeviceInfo->maxInputChannels == 0 ||
      outputDeviceInfo->maxOutputChannels == 0) {
    return false;
  }
  inputParams->channelCount = inputDeviceInfo->maxInputChannels;
  outputParams->channelCount = outputDeviceInfo->maxOutputChannels;

  inputParams->suggestedLatency = latency;
  outputParams->suggestedLatency = latency;

  inputParams->hostApiSpecificStreamInfo = NULL;
  outputParams->hostApiSpecificStreamInfo = NULL;
  return true;
}

bool PortAudioStreamer::TryToStart(const char *hostAPI,
                                   const char *inputDevice,
                                   const QList<QVariant> &inputChannels_,
                                   const char *outputDevice,
                                   const QList<QVariant> &outputChannels_,
                                   double sampleRate,
                                   double latency)
{
  PaStreamParameters inputParams, outputParams;
  PaError error;

  if (!setupParameters(hostAPI, inputDevice, outputDevice,
                       &inputParams, &outputParams, latency)) {
    return false;
  }

  qDebug("Trying Pa_OpenStream() with sampleRate %g inputLatency %g outputLatency %g innch %d outnch %d",
         sampleRate,
         inputParams.suggestedLatency,
         outputParams.suggestedLatency,
         inputParams.channelCount,
         outputParams.channelCount);

  const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(inputParams.device);
  if (deviceInfo) {
    const PaHostApiInfo *hostApiInfo = Pa_GetHostApiInfo(deviceInfo->hostApi);
    qDebug("Input device: %s (%s)", deviceInfo->name,
           hostApiInfo ? hostApiInfo->name : "<invalid host api>");
  }
  deviceInfo = Pa_GetDeviceInfo(outputParams.device);
  if (deviceInfo) {
    const PaHostApiInfo *hostApiInfo = Pa_GetHostApiInfo(deviceInfo->hostApi);
    qDebug("Output device: %s (%s)", deviceInfo->name,
           hostApiInfo ? hostApiInfo->name : "<invalid host api>");
  }

  initChannels(inputChannels_, outputChannels_);

  error = Pa_OpenStream(&stream, &inputParams, &outputParams,
                        sampleRate, latency * sampleRate + 0.5,
                        paPrimeOutputBuffersUsingStreamCallback,
                        streamCallbackTrampoline, this);
  if (error != paNoError) {
    logPortAudioError("Pa_OpenStream() failed", error);
    Stop();
    return false;
  }

  numHWInputChannels = inputParams.channelCount;
  numHWOutputChannels = outputParams.channelCount;

  error = Pa_SetStreamFinishedCallback(stream, streamFinishedCallbackTrampoline);
  if (error != paNoError) {
    logPortAudioError("Pa_SetStreamFinishedCallback failed", error);
    Stop();
    return false;
  }

  error = Pa_StartStream(stream);
  if (error != paNoError) {
    logPortAudioError("Pa_StartStream() failed", error);
    Stop();
    return false;
  }

  return true;
}

bool PortAudioStreamer::Start(const char *hostAPI,
    const char *inputDevice, const QList<QVariant> &inputChannels_,
    const char *outputDevice, const QList<QVariant> &outputChannels_,
    double sampleRate, double latency)
{
  if (!TryToStart(hostAPI, inputDevice, inputChannels_,
                  outputDevice, outputChannels_,
                  sampleRate, latency)) {
    return false;
  }

  const PaStreamInfo *streamInfo = Pa_GetStreamInfo(stream);
  if (streamInfo) {
    qDebug("Stream started with sampleRate %g inputLatency %g outputLatency %g",
           streamInfo->sampleRate,
           streamInfo->inputLatency,
           streamInfo->outputLatency);
  }
  if (streamInfo->inputLatency == latency &&
      streamInfo->outputLatency == latency) {
    return true;
  }

  /* Try again with the latency that PortAudio got */
  double newLatency = streamInfo->inputLatency;
  if (streamInfo->outputLatency > newLatency) {
    newLatency = streamInfo->outputLatency;
  }

  Stop(); /* streamInfo is freed by PortAudio after this */

  if (!TryToStart(hostAPI, inputDevice, inputChannels_,
                  outputDevice, outputChannels_,
                  sampleRate, newLatency)) {
    return false;
  }

  streamInfo = Pa_GetStreamInfo(stream);
  if (streamInfo) {
    qDebug("Stream started on second try with sampleRate %g inputLatency %g outputLatency %g",
           streamInfo->sampleRate,
           streamInfo->inputLatency,
           streamInfo->outputLatency);
  }
  if (streamInfo->inputLatency <= newLatency &&
      streamInfo->outputLatency <= newLatency) {
    return true;
  }

  Stop(); /* streamInfo is freed by PortAudio after this */

  /* Fall back to first value if latency got worse */
  if (!TryToStart(hostAPI, inputDevice, inputChannels_,
                  outputDevice, outputChannels_,
                  sampleRate, latency)) {
    return false;
  }

  streamInfo = Pa_GetStreamInfo(stream);
  if (streamInfo) {
    qDebug("Stream started on third try with sampleRate %g inputLatency %g outputLatency %g",
           streamInfo->sampleRate,
           streamInfo->inputLatency,
           streamInfo->outputLatency);
  }

  return true;
}

static void logPortAudioInfo()
{
  qDebug("%s", Pa_GetVersionText());

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
    logPortAudioError("Pa_Initialize() failed", error);
    return false;
  }
  atexit(portAudioCleanup);

  logPortAudioInfo();
  return true;
}
