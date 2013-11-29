/*
    Copyright (C) 2013-2016 Stefan Hajnoczi <stefanha@gmail.com>

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

#include <QtGlobal>
#include <QMacCocoaViewContainer>
#include <QVBoxLayout>
#include <CoreMIDI/CoreMIDI.h>

#include "AudioUnitPlugin.h"
#include "createuiwidget.h"

static QString cfstringToQString(CFStringRef str)
{
  if (!str) {
    return QString();
  }

  CFIndex length = CFStringGetLength(str);
  if (length == 0) {
    return QString();
  }

  QString string(length, Qt::Uninitialized);
  CFStringGetCharacters(str, CFRangeMake(0, length),
                        reinterpret_cast<UniChar *>(const_cast<QChar *>(string.unicode())));
  return string;
}

static AudioComponent findAudioUnit(const QString &componentName)
{
  AudioComponent component = NULL;
  AudioComponentDescription description;

  memset(&description, 0, sizeof(description));

  while ((component = AudioComponentFindNext(component, &description))) {
    CFStringRef cfname;
    if (AudioComponentCopyName(component, &cfname) != 0) {
      continue;
    }

    QString name(cfstringToQString(cfname));
    CFRelease(cfname);

    if (name == componentName) {
      return component;
    }
  }
  return NULL;
}

static QStringList enumerateAudioUnits(OSType componentType)
{
  QStringList plugins;
  AudioComponent component = NULL;
  AudioComponentDescription description;

  memset(&description, 0, sizeof(description));
  description.componentType = componentType;

  while ((component = AudioComponentFindNext(component, &description))) {
    CFStringRef cfname;
    if (AudioComponentCopyName(component, &cfname) != 0) {
      continue;
    }

    plugins.append(cfstringToQString(cfname));
    CFRelease(cfname);
  }
  return plugins;
}

QStringList AudioUnitScanner::scan(const QStringList &searchPath) const
{
  Q_UNUSED(searchPath);
  QStringList plugins = enumerateAudioUnits(kAudioUnitType_MusicDevice) +
                        enumerateAudioUnits(kAudioUnitType_MusicEffect);
  plugins.removeDuplicates();
  return plugins;
}

QString AudioUnitScanner::displayName(const QString &fullName) const
{
  return fullName;
}

QString AudioUnitScanner::tag() const
{
  return "AudioUnit";
}

static OSStatus audioUnitGetBeatAndTempoCallbackTrampoline(
    void *inHostUserData,
    Float64 *outCurrentBeat,
    Float64 *outCurrentTempo)
{
  AudioUnitPlugin *plugin = (AudioUnitPlugin*)inHostUserData;
  return plugin->getBeatAndTempoCallback(outCurrentBeat, outCurrentTempo);
}

static OSStatus audioUnitRenderCallbackTrampoline(
    void *inRefCon,
    AudioUnitRenderActionFlags *ioActionFlags,
    const AudioTimeStamp *inTimeStamp,
    UInt32 inBusNumber,
    UInt32 inNumberFrames,
    AudioBufferList *ioData)
{
  AudioUnitPlugin *plugin = (AudioUnitPlugin*)inRefCon;
  return plugin->renderCallback(ioActionFlags,
                                inTimeStamp,
                                inBusNumber,
                                inNumberFrames,
                                ioData);
}

static OSStatus audioUnitMidiOutputCallbackTrampoline(
    void *userData,
    const AudioTimeStamp *timestamp,
    UInt32 midiOutNum,
    const struct MIDIPacketList *pktlist)
{
  AudioUnitPlugin *plugin = (AudioUnitPlugin*)userData;
  return plugin->midiOutputCallback(timestamp, midiOutNum, pktlist);
}

AudioUnitPlugin::AudioUnitPlugin(const QString &componentName_)
  : componentName(componentName_), unit(NULL), editorDialog(NULL),
    bufferList(NULL), tempo(120)
{
}

AudioUnitPlugin::~AudioUnitPlugin()
{
  unload();
}

bool AudioUnitPlugin::load()
{
  HostCallbackInfo hci = {
    .hostUserData = this,
    .beatAndTempoProc = audioUnitGetBeatAndTempoCallbackTrampoline,
  };
  AUMIDIOutputCallbackStruct mocs = {
    .midiOutputCallback = audioUnitMidiOutputCallbackTrampoline,
    .userData = this,
  };

  if (unit) {
    return false;
  }

  qDebug("AudioUnit component name: %s", componentName.toLatin1().constData());

  AudioComponent component = findAudioUnit(componentName);
  if (!component) {
    qDebug("Unable to find AudioUnit component");
    return false;
  }

  AudioComponentDescription description;
  OSStatus result = AudioComponentGetDescription(component, &description);
  if (result != noErr) {
    qDebug("AudioComponentGetDescription failed (OSStatus %#08x)", result);
    return false;
  }
  if (description.componentType == kAudioUnitType_MusicDevice ||
      description.componentType == kAudioUnitType_MusicEffect) {
    hasMidiInput = true;
  } else {
    hasMidiInput = false;
  }
  qDebug("AudioUnit has MIDI input: %s",
         hasMidiInput ? "yes" : "no");

  result = AudioComponentInstanceNew(component, &unit);
  if (result != noErr) {
    qDebug("AudioComponentInstanceNew failed (OSStatus %#08x)", result);
    return false;
  }

  /* Probe stereo/mono support */
  UInt32 numInputElems;
  UInt32 dataSize = sizeof(numInputElems);
  result = AudioUnitGetProperty(unit, kAudioUnitProperty_ElementCount,
                                         kAudioUnitScope_Input,
                                         0,
                                         &numInputElems,
                                         &dataSize);
  if (result != noErr) {
    qDebug("AudioComponentInstanceNew failed (OSStatus %#08x)", result);
    goto err;
  }
  if (numInputElems == 0) {
    numInputChannels = 0;
  } else if (setStreamFormat(kAudioUnitScope_Input, 2, 44100)) {
    numInputChannels = 2;
  } else if (setStreamFormat(kAudioUnitScope_Input, 1, 44100)) {
    numInputChannels = 1;
  } else {
    qDebug("AudioUnit does not support mono or stereo input!");
    goto err;
  }
  qDebug("AudioUnit number of input channels: %d", numInputChannels);

  if (setStreamFormat(kAudioUnitScope_Output, 2, 44100)) {
    numOutputChannels = 2;
  } else if (setStreamFormat(kAudioUnitScope_Output, 1, 44100)) {
    numOutputChannels = 1;
  } else {
    qDebug("AudioUnit does not support mono or stereo output!");
    goto err;
  }
  qDebug("AudioUnit number of output channels: %d", numOutputChannels);

  /* Set render callback */
  if (numInputElems > 0) {
    AURenderCallbackStruct rcs;
    rcs.inputProc = audioUnitRenderCallbackTrampoline;
    rcs.inputProcRefCon = this;
    result = AudioUnitSetProperty(unit,
                                  kAudioUnitProperty_SetRenderCallback,
                                  kAudioUnitScope_Input,
                                  0,
                                  &rcs,
                                  sizeof(rcs));
    if (result != noErr) {
      qDebug("Failed to set RenderCallback property (OSStatus %#08x)",
             result);
      goto err;
    }
  }

  /* Allocate output buffer list now that we know the channel count */
  bufferList = (AudioBufferList*)malloc(sizeof(AudioBufferList) +
                                        (numOutputChannels - 1) * sizeof(AudioBuffer));
  if (!bufferList) {
    qDebug("Failed to allocate AudioUnit output buffer list");
    goto err;
  }

  /* Set host callbacks */
  result = AudioUnitSetProperty(unit,
                                kAudioUnitProperty_HostCallbacks,
                                kAudioUnitScope_Global,
                                0,
                                &hci,
                                sizeof(hci));
  if (result != noErr) {
    qDebug("Failed to set HostCallbacks property (OSStatus %#08x)", result);
    goto err;
  }

  result = AudioUnitSetProperty(unit,
                                kAudioUnitProperty_MIDIOutputCallback,
                                kAudioUnitScope_Global,
                                0,
                                &mocs,
                                sizeof(mocs));
  if (result == noErr) {
    qDebug("AudioUnit has MIDI output: yes");
  } else if (result == kAudioUnitErr_InvalidProperty) {
    qDebug("AudioUnit has MIDI output: no");
  } else {
    qDebug("Failed to set MIDIOutputCallback property (OSStatus %#08x)", result);
    /* Nevermind, it probably doesn't have MIDI output... */
  }

  return true;

err:
  AudioComponentInstanceDispose(unit);
  unit = NULL;
  free(bufferList);
  bufferList = NULL;
  return false;
}

void AudioUnitPlugin::unload()
{
  if (!unit) {
    return;
  }
  closeEditor();
  free(bufferList);
  bufferList = NULL;
  AudioComponentInstanceDispose(unit);
  unit = NULL;
}

QString AudioUnitPlugin::getName() const
{
  return componentName;
}

void AudioUnitPlugin::openEditor(QWidget *parent)
{
  if (!unit) {
    return;
  }
  if (editorDialog) {
    editorDialog->show();
    return;
  }

  AudioUnitCocoaViewInfo viewInfo;
  UInt32 dataSize = sizeof(viewInfo);

  OSStatus result = AudioUnitGetProperty(unit,
                                         kAudioUnitProperty_CocoaUI,
                                         kAudioUnitScope_Global,
                                         0,
                                         &viewInfo,
                                         &dataSize);
  if (result != noErr) {
    qDebug("Failed to get kAudioUnitProperty_CocoaUI (OSStatus %#08x)",
           result);
    return;
  }

  QDialog *dialog = new QDialog(parent, Qt::Window);
  connect(dialog, SIGNAL(finished(int)),
          this, SLOT(editorDialogFinished(int)));
  dialog->setWindowTitle(componentName + " [Plugin]");

  QWidget *ui = createUIWidget(viewInfo.mCocoaAUViewBundleLocation,
                               viewInfo.mCocoaAUViewClass[0],
                               unit,
                               dialog);

  QVBoxLayout *layout = new QVBoxLayout;
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(ui);
  dialog->setLayout(layout);

  CFRelease(viewInfo.mCocoaAUViewBundleLocation);
  CFRelease(viewInfo.mCocoaAUViewClass[0]);

  if (!ui) {
    delete dialog;
    return;
  }

  dialog->resize(800, 600);
  dialog->show();
  editorDialog = dialog;
}

void AudioUnitPlugin::closeEditor()
{
  if (editorDialog) {
    editorDialog->deleteLater();
    editorDialog = NULL;
  }
}

void AudioUnitPlugin::editorDialogFinished(int)
{
  closeEditor();
}

void AudioUnitPlugin::queueResizeEditor(int width, int height)
{
  Q_UNUSED(width);
  Q_UNUSED(height);

  /* VST-specific, not needed for AudioUnit */
}

void AudioUnitPlugin::queueIdle()
{
  /* VST-specific, not needed for AudioUnit */
}

void AudioUnitPlugin::outputEvents(VstEvents *vstEvents)
{
  Q_UNUSED(vstEvents);

  /* VST-specific, not needed for AudioUnit */
}

int AudioUnitPlugin::numInputs() const
{
  return numInputChannels;
}

int AudioUnitPlugin::numOutputs() const
{
  return numOutputChannels;
}

bool AudioUnitPlugin::setStreamFormat(AudioUnitScope scope, int numChannels, int sampleRate)
{
  AudioStreamBasicDescription description;
  memset(&description, 0, sizeof(description));
  description.mSampleRate = sampleRate;
  description.mFormatID = kAudioFormatLinearPCM;
  description.mFormatFlags = kAudioFormatFlagsNativeFloatPacked |
                             kAudioFormatFlagIsNonInterleaved;
  description.mBytesPerPacket = sizeof(float);
  description.mFramesPerPacket = 1;
  description.mBytesPerFrame = sizeof(float);
  description.mBitsPerChannel = 8 * sizeof(float);
  description.mChannelsPerFrame = numChannels;

  OSStatus result = AudioUnitSetProperty(unit,
                                         kAudioUnitProperty_StreamFormat,
                                         scope,
                                         0,
                                         &description,
                                         sizeof(description));
  if (result != noErr) {
    qDebug("Failed to set StreamFormat input %s property on %s (OSStatus %#08x)",
           componentName.toLatin1().constData(),
           numChannels == 1 ? "mono" : "stereo",
           result);
    return false;
  }
  return true;
}

void AudioUnitPlugin::setSampleRate(int rate)
{
  sampleRate = rate;
  setStreamFormat(kAudioUnitScope_Input, numInputChannels, rate);
  setStreamFormat(kAudioUnitScope_Output, numOutputChannels, rate);
}

void AudioUnitPlugin::setTempo(int tempo_)
{
  tempo = tempo_;
}

void AudioUnitPlugin::setMidiOutput(ConcurrentQueue<PmEvent> *midiOutput_)
{
  midiOutput = midiOutput_;
}

void AudioUnitPlugin::changeMains(bool enable)
{
  if (enable) {
    /* Reset timestamp */
    memset(&timestamp, 0, sizeof(timestamp));
    timestamp.mSampleTime = 0;
    timestamp.mFlags = kAudioTimeStampSampleTimeValid;

    /* Reset beat */
    samplesUntilNextBeat = sampleRate / ((double)tempo / 60);
    currentBeat = 0;

    OSStatus result = AudioUnitInitialize(unit);
    if (result != noErr) {
      qDebug("AudioUnitInitialize failed for %s (OSStatus %#08x)",
             componentName.toLatin1().constData(), result);
    }
  } else {
    OSStatus result = AudioUnitUninitialize(unit);
    if (result != noErr) {
      qDebug("AudioUnitInitialize failed for %s (OSStatus %#08x)",
             componentName.toLatin1().constData(), result);
    }
  }
}

void AudioUnitPlugin::processEvents(VstEvents *vstEvents)
{
  int i;

  if (!hasMidiInput) {
    return;
  }

  for (i = 0; i < vstEvents->numEvents; i++) {
    VstMidiEvent *event = (VstMidiEvent *)vstEvents->events[i];
    MusicDeviceMIDIEvent(unit,
                         event->midiData[0],
                         event->midiData[1],
                         event->midiData[2],
                         event->deltaFrames);
  }
}

void AudioUnitPlugin::process(float **inbuf, float **outbuf, int ns)
{
	AudioUnitRenderActionFlags flags = 0;
  int i;

  /* TODO handle plugins where ns > max frames */

  /* Setup output buffer list */
  bufferList->mNumberBuffers = numOutputChannels;
  for (i = 0; i < numOutputChannels; i++) {
    bufferList->mBuffers[i].mNumberChannels = 1; /* always because we use non-interleaved */
    bufferList->mBuffers[i].mDataByteSize = ns * sizeof(float);
    bufferList->mBuffers[i].mData = outbuf[i];
  }

  /* Stash away input buffer info for renderCallback() */
  inputBuffers = inbuf;
  processNumSamples = ns;

  OSStatus result = AudioUnitRender(unit, &flags, &timestamp,
                                    0, ns, bufferList);
  if (result != noErr) {
    goto err;
  }

  /* Clean up output buffer list */
  for (i = 0; i < numOutputChannels; i++) {
    /* Copy data back if plugin used its own buffer */
    if (i < (int)bufferList->mNumberBuffers &&
        bufferList->mBuffers[i].mData != outbuf[i]) {
      memcpy(outbuf[i], bufferList->mBuffers[i].mData, ns * sizeof(float));
    }

    /* Silence unused output buffers */
    if (i >= (int)bufferList->mNumberBuffers) {
      memset(outbuf[i], 0, ns * sizeof(float));
    }
  }

  timestamp.mSampleTime += ns;
  updateCurrentBeat(ns);
  return;

err:
  for (i = 0; i < numOutputChannels; i++) {
    memset(outbuf[i], 0, ns * sizeof(float));
  }
}

void AudioUnitPlugin::idle()
{
  /* VST-specific, not needed for AudioUnit */
}

OSStatus AudioUnitPlugin::renderCallback(
    AudioUnitRenderActionFlags *ioActionFlags,
    const AudioTimeStamp *inTimeStamp,
    UInt32 inBusNumber,
    UInt32 inNumberFrames,
    AudioBufferList *ioData)
{
  int channels = qMin((int)ioData->mNumberBuffers, numInputChannels);
  int i;

  Q_UNUSED(ioActionFlags);
  Q_UNUSED(inTimeStamp);
  Q_UNUSED(inBusNumber);
  Q_UNUSED(inNumberFrames);

  for (i = 0; i < channels; i++) {
    ioData->mBuffers[i].mNumberChannels = 1; /* always because we use non-interleaved */
    ioData->mBuffers[i].mDataByteSize = processNumSamples * sizeof(float);
    ioData->mBuffers[i].mData = inputBuffers[i];
  }
  return noErr;
}

OSStatus AudioUnitPlugin::getBeatAndTempoCallback(Float64 *outCurrentBeat,
                                                  Float64 *outCurrentTempo)
{
  if (outCurrentBeat) {
    *outCurrentBeat = currentBeat;
  }

  if (outCurrentTempo) {
    *outCurrentTempo = tempo;
  }

  return noErr;
}

void AudioUnitPlugin::updateCurrentBeat(int numSamples)
{
  while (numSamples >= samplesUntilNextBeat) {
    numSamples -= samplesUntilNextBeat;
    samplesUntilNextBeat = sampleRate / ((double)tempo / 60);
    currentBeat++;
  }

  samplesUntilNextBeat -= numSamples;
}

OSStatus AudioUnitPlugin::midiOutputCallback(
    const AudioTimeStamp *timestamp,
    UInt32 midiOutNum,
    const struct MIDIPacketList *pktlist)
{
  const MIDIPacket *packet = &pktlist->packet[0];

  Q_UNUSED(timestamp);
  Q_UNUSED(midiOutNum); /* forward MIDI from all outputs */

  if (!midiOutput) {
    return noErr; /* drop packets */
  }

  for (UInt32 i = 0; i < pktlist->numPackets; i++) {
    PmEvent event;

    eventParser.feed(packet->data, packet->length);

    while (eventParser.next(&event)) {
      midiOutput->write(&event, 1);
    }

    packet = MIDIPacketNext(packet);
  }
  return noErr;
}
