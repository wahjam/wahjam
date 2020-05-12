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

#ifndef _EFFECTPROCESSOR_H_
#define _EFFECTPROCESSOR_H_

#include <QList>
#include <QTimer>
#include <QMutex>
#include <portmidi.h>
#include "common/ConcurrentQueue.h"
#include "NJClient.h"
#include "EffectPlugin.h"

class EffectProcessor : public QObject
{
  Q_OBJECT

public:
  EffectProcessor(ConcurrentQueue<PmEvent> *midiInput,
                  ConcurrentQueue<PmEvent> *midiOutput,
                  QObject *parent = NULL);
  ~EffectProcessor();

  bool insertPlugin(int idx, EffectPlugin *plugin);
  bool removePlugin(int idx);
  void moveUp(int idx);
  void moveDown(int idx);
  int numPlugins();
  EffectPlugin *getPlugin(int idx);

  void attach(NJClient *client, int ch);
  void detach();

  void process(float *buf, int ns);

private slots:
  void idleTimerTick();

private:
  NJClient *client;

  /* The plugins are accessed from the GUI and audio threads.  The audio thread
   * only traverses the list.  The audio thread may also modify the list.  This
   * lock protects the audio thread against GUI thread list modifications - it
   * is not used for read accesses from the GUI thread since they do not
   * interfere with audio thread read accesses.
   */
  QMutex pluginsLock;
  QList<EffectPlugin*> plugins;

  QTimer idleTimer;
  int localChannel;

  size_t blockSize;
  int lastTempo; /* used for signalling BPM changes */

  /* Plugins may have differing numbers of inputs/outputs, we keep around
   * scratch buffers that we fill with silence.
   */
  float **scratchBufs;
  int maxInputsOutputs;

  ConcurrentQueue<PmEvent> *midiInput;
  ConcurrentQueue<PmEvent> *midiOutput;
  VstEvent vstEventBuffer[128];
  VstEvents *vstEvents;

  bool attached();
  float **newScratchBufs(int nbufs);
  void deleteScratchBufs(float **bufs, int nbufs);
  void allocVstEvents();
  void fillVstEvents();
  void activatePlugin(EffectPlugin *plugin);
};

#endif /* _EFFECTPROCESSOR_H_ */
