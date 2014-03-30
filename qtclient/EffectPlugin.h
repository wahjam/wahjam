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

#ifndef _EFFECTPLUGIN_H_
#define _EFFECTPLUGIN_H_

#include <portmidi.h>
#include <QWidget>
#include "../vestige/aeffectx.h"
#include "common/ConcurrentQueue.h"

/*
 * Base class for effect plugins (VST, AudioUnit, etc)
 */
class EffectPlugin : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QString name READ getName)

public:
  virtual ~EffectPlugin() {}

  virtual bool load() = 0;
  virtual void unload() = 0;

  virtual QString getName() const = 0;

  virtual void openEditor(QWidget *parent) = 0;
  virtual void closeEditor() = 0;
  virtual void queueResizeEditor(int width, int height) = 0;
  virtual void queueIdle() = 0;
  virtual void outputEvents(VstEvents *vstEvents) = 0;

  virtual int numInputs() const = 0;
  virtual int numOutputs() const = 0;
  virtual void setSampleRate(int rate) = 0;
  virtual void setTempo(int tempo) = 0;
  virtual void setMidiOutput(ConcurrentQueue<PmEvent> *midiOutput) = 0;
  virtual void changeMains(bool enable) = 0;
  virtual void processEvents(VstEvents *vstEvents) = 0;
  virtual void process(float **inbuf, float **outbuf, int ns) = 0;

signals:
  void onResizeEditor(int width, int height);
  void onIdle();

public slots:
  virtual void idle() = 0;
};

#endif /* _EFFECTPLUGIN_H_ */
