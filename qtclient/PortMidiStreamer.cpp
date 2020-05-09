/*
    Copyright (C) 2013 Stefan Hajnoczi <stefanha@gmail.com>

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

#include "PortMidiStreamer.h"

enum {
  BUFFER_SIZE = 128, /* number of MIDI messages */
};

static void logPortMidiError(const char *msg, PmError error)
{
  if (error == pmHostError) {
    char err[512] = {0};
    Pm_GetHostErrorText(err, sizeof err);
    qCritical("%s: host error: %s", msg, err);
  } else {
    qCritical("%s: %s", msg, Pm_GetErrorText(error));
  }
}

static int findDeviceIdByName(const QString &name, bool output)
{
  for (int i = 0; i < Pm_CountDevices(); i++) {
    const PmDeviceInfo *deviceInfo = Pm_GetDeviceInfo(i);
    if (deviceInfo &&
        name == QString::fromLocal8Bit(deviceInfo->name) &&
        deviceInfo->output == output) {
      return i;
    }
  }
  return -1;
}

static void closeStream(PortMidiStream **stream)
{
  if (*stream) {
    Pm_Close(*stream);
    *stream = NULL;
  }
}

static void processMidiTrampoline(PtTimestamp timestamp, void *opaque)
{
  PortMidiStreamer *self = (PortMidiStreamer *)opaque;
  self->process(timestamp);
}

PortMidiStreamer::PortMidiStreamer(QObject *parent)
  : QObject(parent), inputStream(NULL), outputStream(NULL),
    outputQueue(BUFFER_SIZE)
{
  outputQueue.setDiscardWrites(true);
}

static PmTimestamp timeProc(void*)
{
  return Pt_Time();
}

void PortMidiStreamer::start(const QString &inputDeviceName,
                             const QString &outputDeviceName,
                             int latencyMilliseconds)
{
  if (inputStream || outputStream) {
    return;
  }
  if (inputDeviceName.isEmpty() && outputDeviceName.isEmpty()) {
    return;
  }

  PtError ptError = Pt_Start(1, processMidiTrampoline, this);
  if (ptError != ptNoError) {
    qCritical("Pt_Start failed (%d)", ptError);
    return;
  }

  if (!inputDeviceName.isEmpty()) {
    int inputDeviceId = findDeviceIdByName(inputDeviceName, false);
    if (inputDeviceId < 0) {
      qCritical("Input device \"%s\" not found",
                inputDeviceName.toLatin1().constData());
      goto err;
    }
    qDebug("Trying Pm_OpenInput() with inputDeviceId %d for \"%s\"",
           inputDeviceId, inputDeviceName.toLatin1().constData());
    PmError pmError = Pm_OpenInput(&inputStream, inputDeviceId,
                                   NULL, 0, timeProc, NULL);
    if (pmError != pmNoError) {
      logPortMidiError("Pm_OpenInput", pmError);
      goto err;
    }
  }

  if (!outputDeviceName.isEmpty()) {
    int outputDeviceId = findDeviceIdByName(outputDeviceName, true);
    if (outputDeviceId < 0) {
      qCritical("Output device \"%s\" not found",
                outputDeviceName.toLatin1().constData());
      goto err;
    }
    qDebug("Trying Pm_OpenOutput() with outputDeviceId %d for \"%s\"",
           outputDeviceId, outputDeviceName.toLatin1().constData());
    PmError pmError = Pm_OpenOutput(&outputStream, outputDeviceId,
                                    NULL, BUFFER_SIZE, timeProc, NULL,
                                    latencyMilliseconds);
    if (pmError != pmNoError) {
      logPortMidiError("Pm_OpenOutput", pmError);
      goto err;
    }

    outputQueue.setDiscardWrites(false);
  }

  return;

err:
  Pt_Stop();
  closeStream(&inputStream);
}

void PortMidiStreamer::stop()
{
  if (!inputStream && !outputStream) {
    return;
  }

  outputQueue.setDiscardWrites(true);
  Pt_Stop();
  closeStream(&inputStream);
  closeStream(&outputStream);
}

void PortMidiStreamer::addInputQueue(ConcurrentQueue<PmEvent> *queue)
{
  if (inputStream) {
    qCritical("Cannot add MIDI read queue while started");
    return;
  }
  if (inputQueues.contains(queue)) {
    qCritical("Cannot add same MIDI read queue multiple times");
    return;
  }

  inputQueues.append(queue);
}

void PortMidiStreamer::removeInputQueue(ConcurrentQueue<PmEvent> *queue)
{
  inputQueues.removeOne(queue);
}

void PortMidiStreamer::processInput()
{
  if (!inputStream) {
    return;
  }

  while (Pm_Poll(inputStream) == pmGotData) {
    PmEvent event;
    int n = Pm_Read(inputStream, &event, 1);
    if (n != 1) {
      continue;
    }

    ConcurrentQueue<PmEvent> *inputQueue;
    foreach (inputQueue, inputQueues) {
      inputQueue->write(&event, 1);
    }
  }
}

void PortMidiStreamer::processOutput()
{
  QQueue<PmEvent> &queue = outputQueue.getReadQueue();

  while (!queue.isEmpty()) {
    PmEvent event = queue.dequeue();
    if (outputStream) {
      Pm_Write(outputStream, &event, 1);
    }
  }
}

void PortMidiStreamer::process(PtTimestamp timestamp)
{
  Q_UNUSED(timestamp);
  processInput();
  processOutput();
}

static void logPortMidiInfo()
{
  int i;

  qDebug("PortMidi devices:");

  for (i = 0; i < Pm_CountDevices(); i++) {
    const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
    if (!info) {
      continue;
    }

    qDebug("[%02d] \"%s\" %s%s[%s]",
           i, info->name,
           info->input ? "(IN) " : "",
           info->output ? "(OUT) " : "",
           info->interf);
  }
}

static void portMidiCleanup()
{
  Pm_Terminate();
}

/* Initialize PortMidi once for the whole application */
bool portMidiInit()
{
  PmError error = Pm_Initialize();
  if (error != pmNoError) {
    logPortMidiError("Pm_Initialize() failed", error);
    return false;
  }
  atexit(portMidiCleanup);

  logPortMidiInfo();
  return true;
}
