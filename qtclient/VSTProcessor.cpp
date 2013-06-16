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

#include "VSTProcessor.h"

void vstProcessorCallback(float *buf, int ns, void *inst)
{
  VSTProcessor *this_ = static_cast<VSTProcessor*>(inst);
  this_->process(buf, ns);
}

VSTProcessor::VSTProcessor(ConcurrentQueue<PmEvent> *midiInput_,
                           ConcurrentQueue<PmEvent> *midiOutput_,
                           QObject *parent)
  : QObject(parent), client(NULL), localChannel(-1), blockSize(512),
    scratchInputBufs(NULL), maxInputs(0),
    scratchOutputBufs(NULL), maxOutputs(0),
    midiInput(midiInput_),
    midiOutput(midiOutput_)
{
  connect(&idleTimer, SIGNAL(timeout()),
          this, SLOT(idleTimerTick()));

  memset(vstEventBuffer, 0, sizeof vstEventBuffer);
  allocVstEvents();
}

VSTProcessor::~VSTProcessor()
{
  detach();

  while (numPlugins() > 0) {
    removePlugin(0);
  }

  deleteScratchBufs(scratchInputBufs, maxInputs);
  deleteScratchBufs(scratchOutputBufs, maxOutputs);
  free(vstEvents);
}

float **VSTProcessor::newScratchBufs(int nbufs)
{
  float **bufs = new float*[nbufs];
  for (int i = 0; i < nbufs; i++) {
    bufs[i] = new float[blockSize];
  }
  return bufs;
}

void VSTProcessor::deleteScratchBufs(float **bufs, int nbufs)
{
  for (int i = 0; i < nbufs; i++) {
    delete [] bufs[i];
  }
  delete [] bufs;
}

void VSTProcessor::allocVstEvents()
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

bool VSTProcessor::insertPlugin(int idx, VSTPlugin *plugin)
{
  QMutexLocker locker(&pluginsLock);

  // TODO check compatible with inputs/outputs of other plugins

  plugin->setParent(this);
  plugin->setMidiOutput(midiOutput);
  plugins.insert(idx, plugin);

  /* We may need to grow scratch buffers */
  if (plugin->numInputs() > maxInputs) {
    deleteScratchBufs(scratchInputBufs, maxInputs);
    maxInputs = plugin->numInputs();
    scratchInputBufs = newScratchBufs(maxInputs);
  }
  if (plugin->numOutputs() > maxOutputs) {
    deleteScratchBufs(scratchOutputBufs, maxOutputs);
    maxOutputs = plugin->numOutputs();
    scratchOutputBufs = newScratchBufs(maxOutputs);
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

bool VSTProcessor::removePlugin(int idx)
{
  QMutexLocker locker(&pluginsLock);
  VSTPlugin *plugin = getPlugin(idx);
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

void VSTProcessor::moveUp(int idx)
{
  QMutexLocker locker(&pluginsLock);

  if (idx <= 0 || idx >= numPlugins()) {
    return;
  }

  plugins.swap(idx, idx - 1);
}

void VSTProcessor::moveDown(int idx)
{
  QMutexLocker locker(&pluginsLock);

  if (idx < 0 || idx + 1 >= numPlugins()) {
    return;
  }

  plugins.swap(idx, idx + 1);
}

int VSTProcessor::numPlugins()
{
  return plugins.count();
}

VSTPlugin *VSTProcessor::getPlugin(int idx)
{
  return plugins.value(idx);
}

void VSTProcessor::activatePlugin(VSTPlugin *plugin)
{
  plugin->changeMains(false);
  plugin->setSampleRate(client->GetSampleRate());
  plugin->setTempo(client->GetActualBPM()); // TODO update when bpm changes
  plugin->changeMains(true);
}

void VSTProcessor::attach(NJClient *client_, int ch)
{
  if (attached()) {
    return;
  }

  client = client_;

  VSTPlugin *plugin;
  foreach (plugin, plugins) {
    activatePlugin(plugin);
  }

  localChannel = ch;
  client->SetLocalChannelProcessor(ch, vstProcessorCallback, this);
}

void VSTProcessor::detach()
{
  if (!attached()) {
    return;
  }

  client->SetLocalChannelProcessor(localChannel, NULL, NULL);
  localChannel = -1;

  VSTPlugin *plugin;
  foreach (plugin, plugins) {
    plugin->changeMains(false);
  }

  client = NULL;
}

bool VSTProcessor::attached()
{
  return localChannel != -1;
}

void VSTProcessor::fillVstEvents()
{
  QQueue<PmEvent> &queue = midiInput->getReadQueue();
  int sampleRate = client->GetSampleRate();
  PmTimestamp firstTimestamp;
  bool firstEvent = true;
  size_t i;

  for (i = 0; !queue.isEmpty() &&
       i < sizeof(vstEventBuffer) / sizeof(vstEventBuffer[0]);
       i++) {
    VstMidiEvent *vstEvent = (VstMidiEvent*)&vstEventBuffer[i];
    PmEvent pmEvent = queue.dequeue();

    vstEvent->type = kVstMidiType;
    vstEvent->byteSize = sizeof(*vstEvent);
    vstEvent->midiData[0] = Pm_MessageStatus(pmEvent.message);
    vstEvent->midiData[1] = Pm_MessageData1(pmEvent.message);
    vstEvent->midiData[2] = Pm_MessageData2(pmEvent.message);
    /* TODO SysEx uses all 4 bytes? */

    if (firstEvent) {
      firstTimestamp = pmEvent.timestamp;
      firstEvent = false;
    }
    vstEvent->deltaFrames = (pmEvent.timestamp - firstTimestamp) *
                            sampleRate / 1000;
  }

  vstEvents->numEvents = i;
}

void VSTProcessor::process(float *buf, int ns)
{
  QMutexLocker locker(&pluginsLock);

  fillVstEvents();

  if ((size_t)ns > blockSize) {
    blockSize = ns;
    deleteScratchBufs(scratchInputBufs, maxInputs);
    deleteScratchBufs(scratchOutputBufs, maxOutputs);
    scratchInputBufs = newScratchBufs(maxInputs);
    scratchOutputBufs = newScratchBufs(maxOutputs);
  }

  float *inputs[maxInputs];
  float **a = inputs;
  float **b = scratchOutputBufs;

  memcpy(inputs, scratchInputBufs, sizeof(float*) * maxInputs);
  inputs[0] = buf; // TODO real stereo
  for (int i = 1; i < maxInputs; i++) {
    memset(inputs[i], 0, sizeof(float) * ns);
  }
  for (int i = 0; i < maxOutputs; i++) {
    memset(scratchOutputBufs[i], 0, sizeof(float) * ns);
  }

  foreach (VSTPlugin *plugin, plugins) {
    plugin->processEvents(vstEvents);
    plugin->process(a, b, ns);

    float **swap = a;
    a = b;
    b = swap;
  }

  /* Copy final result back into buf */
  if (a[0] != buf) {
    memcpy(buf, a[0], sizeof(float) * ns);
  }
}

void VSTProcessor::idleTimerTick()
{
  VSTPlugin *plugin;

  foreach (plugin, plugins) {
    plugin->idle();
  }
}
