/*
    Copyright (C) 2012-2020 Stefan Hajnoczi <stefanha@gmail.com>

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

#include "EffectProcessor.h"

void vstProcessorCallback(float *buf, int ns, void *inst)
{
  EffectProcessor *this_ = static_cast<EffectProcessor*>(inst);
  this_->process(buf, ns);
}

EffectProcessor::EffectProcessor(PortMidiStreamer *midiStreamer_,
                                 QObject *parent)
  : QObject(parent), client(NULL), localChannel(-1), blockSize(512),
    scratchBufs(NULL), maxInputsOutputs(0), midiStreamer(midiStreamer_)
{
  connect(&idleTimer, SIGNAL(timeout()),
          this, SLOT(idleTimerTick()));

  memset(vstEventBuffer, 0, sizeof vstEventBuffer);
  allocVstEvents();
}

EffectProcessor::~EffectProcessor()
{
  detach();

  while (numPlugins() > 0) {
    removePlugin(0);
  }

  deleteScratchBufs(scratchBufs, 2 * maxInputsOutputs);
  free(vstEvents);
}

float **EffectProcessor::newScratchBufs(int nbufs)
{
  float **bufs = new float*[nbufs];
  for (int i = 0; i < nbufs; i++) {
    bufs[i] = new float[blockSize];
  }
  return bufs;
}

void EffectProcessor::deleteScratchBufs(float **bufs, int nbufs)
{
  for (int i = 0; i < nbufs; i++) {
    delete [] bufs[i];
  }
  delete [] bufs;
}

void EffectProcessor::allocVstEvents()
{
  size_t nelems = sizeof(vstEventBuffer) / sizeof(vstEventBuffer[0]);
  vstEvents = (VstEvents*)malloc(sizeof(*vstEvents) +
                                 nelems * sizeof(VstEvent));
  if (!vstEvents) {
    qCritical("Unable to allocate VSTEvents");
    return;
  }

  memset(vstEvents, 0, sizeof(*vstEvents));
  for (size_t i = 0; i < nelems; i++) {
    vstEvents->events[i] = &vstEventBuffer[i];
  }
}

bool EffectProcessor::insertPlugin(int idx, EffectPlugin *plugin)
{
  QMutexLocker locker(&pluginsLock);

  // TODO check compatible with inputs/outputs of other plugins

  plugin->setParent(this);
  plugins.insert(idx, plugin);

  /* We may need to grow scratch buffers */
  if (plugin->numInputs() > maxInputsOutputs ||
      plugin->numOutputs() > maxInputsOutputs) {
    deleteScratchBufs(scratchBufs, 2 * maxInputsOutputs);
    maxInputsOutputs = qMax(qMax(plugin->numInputs(), plugin->numOutputs()),
                            maxInputsOutputs);
    scratchBufs = newScratchBufs(2 * maxInputsOutputs);
  }

  if (numPlugins() == 1) {
    idleTimer.setSingleShot(false);
    idleTimer.start(100 /* ms */);
  }

  if (attached()) {
    activatePlugin(plugin);
  }
  return true;
}

bool EffectProcessor::removePlugin(int idx)
{
  QMutexLocker locker(&pluginsLock);
  EffectPlugin *plugin = getPlugin(idx);
  if (!plugin) {
    return false;
  }
  delete plugin;
  plugins.removeAt(idx);

  if (numPlugins() == 0) {
    idleTimer.stop();
  }
  return true;
}

void EffectProcessor::moveUp(int idx)
{
  QMutexLocker locker(&pluginsLock);

  if (idx <= 0 || idx >= numPlugins()) {
    return;
  }

#if (QT_VERSION >= QT_VERSION_CHECK(5, 13, 0))
  plugins.swapItemsAt(idx, idx - 1);
#else
  plugins.swap(idx, idx - 1);
#endif
}

void EffectProcessor::moveDown(int idx)
{
  QMutexLocker locker(&pluginsLock);

  if (idx < 0 || idx + 1 >= numPlugins()) {
    return;
  }

#if (QT_VERSION >= QT_VERSION_CHECK(5, 13, 0))
  plugins.swapItemsAt(idx, idx + 1);
#else
  plugins.swap(idx, idx + 1);
#endif
}

int EffectProcessor::numPlugins()
{
  return plugins.count();
}

EffectPlugin *EffectProcessor::getPlugin(int idx)
{
  return plugins.value(idx);
}

void EffectProcessor::activatePlugin(EffectPlugin *plugin)
{
  plugin->changeMains(false);
  plugin->setSampleRate(client->GetSampleRate());
  plugin->setTempo(client->GetActualBPM());
  plugin->changeMains(true);
}

void EffectProcessor::attach(NJClient *client_, int ch)
{
  if (attached()) {
    return;
  }

  client = client_;
  lastTempo = client->GetActualBPM();

  EffectPlugin *plugin;
  foreach (plugin, plugins) {
    activatePlugin(plugin);
  }

  localChannel = ch;
  client->SetLocalChannelProcessor(ch, vstProcessorCallback, this);
}

void EffectProcessor::detach()
{
  if (!attached()) {
    return;
  }

  client->SetLocalChannelProcessor(localChannel, NULL, NULL);
  localChannel = -1;

  EffectPlugin *plugin;
  foreach (plugin, plugins) {
    plugin->changeMains(false);
  }

  client = NULL;
}

bool EffectProcessor::attached()
{
  return localChannel != -1;
}

void EffectProcessor::fillVstEvents()
{
  int sampleRate = client->GetSampleRate();
  PmEvent pmEvent;
  PmTimestamp firstTimestamp;
  size_t i;

  for (i = 0;
       i < sizeof(vstEventBuffer) / sizeof(vstEventBuffer[0]) &&
       midiStreamer->read(&pmEvent);
       i++) {
    VstMidiEvent *vstEvent = (VstMidiEvent*)&vstEventBuffer[i];

    vstEvent->type = kVstMidiType;
    vstEvent->byteSize = sizeof(*vstEvent);
    vstEvent->midiData[0] = Pm_MessageStatus(pmEvent.message);
    vstEvent->midiData[1] = Pm_MessageData1(pmEvent.message);
    vstEvent->midiData[2] = Pm_MessageData2(pmEvent.message);
    /* TODO SysEx uses all 4 bytes? */

    if (i == 0) {
      firstTimestamp = pmEvent.timestamp;
    }
    vstEvent->deltaFrames = (pmEvent.timestamp - firstTimestamp) *
                            sampleRate / 1000;
  }

  vstEvents->numEvents = i;
}

void EffectProcessor::process(float *buf, int ns)
{
  // Skip if no processing is necessary.  The variable-length array below
  // requires a non-zero length.
  if (maxInputsOutputs == 0) {
    return;
  }

  QMutexLocker locker(&pluginsLock);
  int tempo = client->GetActualBPM();

  if (tempo != lastTempo) {
    foreach (EffectPlugin *plugin, plugins) {
      plugin->setTempo(tempo);
    }
    lastTempo = tempo;
  }

  fillVstEvents();

  if ((size_t)ns > blockSize) {
    deleteScratchBufs(scratchBufs, 2 * maxInputsOutputs);
    blockSize = ns;
    scratchBufs = newScratchBufs(2 * maxInputsOutputs);
  }

  float *inputs[maxInputsOutputs];
  float **a = inputs;
  float **b = &scratchBufs[maxInputsOutputs];

  memcpy(inputs, scratchBufs, sizeof(float*) * maxInputsOutputs);
  a[0] = buf; // TODO real stereo
  for (int i = 1; i < maxInputsOutputs; i++) {
    memset(a[i], 0, sizeof(float) * ns);
  }
  for (int i = 0; i < maxInputsOutputs; i++) {
    memset(b[i], 0, sizeof(float) * ns);
  }

  foreach (EffectPlugin *plugin, plugins) {
    if (plugin->getReceiveMidi()) {
      plugin->processEvents(vstEvents);
    }
    plugin->process(a, b, ns);

    plugin->processOutputEvents([=](const PmEvent *event) {
        midiStreamer->write(event);
    });

    float dry = qMin(2 * (1.0f - plugin->getWetDryMix()), 1.0f);
    float wet = qMin(2 * plugin->getWetDryMix(), 1.0f);
    for (int i = 0; i < maxInputsOutputs; i++) {
      for (int j = 0; j < ns; j++) {
        b[i][j] = a[i][j] * dry + b[i][j] * wet;
      }
    }

    float **swap = a;
    a = b;
    b = swap;
  }

  /* Copy final result back into buf */
  if (a[0] != buf) {
    memcpy(buf, a[0], sizeof(float) * ns);
  }
}

void EffectProcessor::idleTimerTick()
{
  EffectPlugin *plugin;

  foreach (plugin, plugins) {
    plugin->idle();
  }
}
