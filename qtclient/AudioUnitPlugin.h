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

#ifndef _AUDIOUNITPLUGIN_H_
#define _AUDIOUNITPLUGIN_H_

#include <QDialog>
#include <AudioUnit/AudioUnit.h>

#include "PluginScanner.h"
#include "EffectPlugin.h"
#include "PmEventParser.h"

class AudioUnitScanner : public PluginScanner
{
public:
  QStringList scan(const QStringList &searchPaths) const;
  QString displayName(const QString &fullName) const;
  QString tag() const;
};

class AudioUnitPlugin : public EffectPlugin
{
  Q_OBJECT

public:
  AudioUnitPlugin(const QString &componentName);
  ~AudioUnitPlugin();

  virtual bool load();
  virtual void unload();

  virtual QString getName() const;

  void openEditor(QWidget *parent);
  void closeEditor();
  void queueResizeEditor(int width, int height);
  void queueIdle();
  void outputEvents(VstEvents *vstEvents);

  int numInputs() const;
  int numOutputs() const;
  void setSampleRate(int rate);
  void setTempo(int tempo);
  void changeMains(bool enable);
  void processEvents(VstEvents *vstEvents);
  void process(float **inbuf, float **outbuf, int ns);

  OSStatus renderCallback(AudioUnitRenderActionFlags *ioActionFlags,
                          const AudioTimeStamp *inTimeStamp,
                          UInt32 inBusNumber,
                          UInt32 inNumberFrames,
                          AudioBufferList *ioData);

  OSStatus getBeatAndTempoCallback(Float64 *outCurrentBeat,
                                   Float64 *outCurrentTempo);

  OSStatus midiOutputCallback(const AudioTimeStamp *timestamp,
                              UInt32 midiOutNum,
                              const struct MIDIPacketList *pktlist);

public slots:
  void idle();

private slots:
  void editorDialogFinished(int result);

private:
  QString componentName;
  AudioUnit unit;
  QDialog *editorDialog;
  int numInputChannels;
  int numOutputChannels;
  bool hasMidiInput;
  PmEventParser eventParser;

  /* State stashed away for host callbacks during process() */
  AudioTimeStamp timestamp;
  AudioBufferList *bufferList;
  float **inputBuffers;
  int processNumSamples;

  /* HostCallback_GetBeatAndTempo() state */
  int tempo;
  int sampleRate;
  int samplesUntilNextBeat;
  Float64 currentBeat;

  bool setStreamFormat(AudioUnitScope scope, int numChannels, int sampleRate);
  void updateCurrentBeat(int numSamples);
};

#endif /* _AUDIOUNITPLUGIN_H_ */
