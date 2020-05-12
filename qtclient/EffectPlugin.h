/*
    Copyright (C) 2013-2020 Stefan Hajnoczi <stefanha@gmail.com>

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

#include <functional>
#include <QWidget>
#include <QMutex>
#include "../vestige/aeffectx.h"
#include "PortMidiStreamer.h"

enum {
  /* Output buffer size for MIDI messages */
  EFFECT_PLUGIN_OUTPUT_EVENTS = 128,
};

/*
 * Base class for effect plugins (VST, AudioUnit, etc)
 */
class EffectPlugin : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QString name READ getName)

  /* Wet/dry mix level, range [0.0, 1.0]
   *   0.0 - Dry 100%, wet 0%
   *   0.5 - Dry 100%, wet 100%
   *   1.0 - Dry 0%, wet 100%
   *
   * Examples:
   *
   * A delay effect is usually mixed together with the dry signal so a value in
   * the range [0, 0.5] is typical.
   *
   * In order to bypass an effect, set its mix to 0.0.  Its processing function
   * is still called so delay trails or any other time-based signal will still
   * be calculated even while bypassed.
   *
   * In order to blend a virtual instrument like a drum machine with its input,
   * set the mix to 0.5.  This sums the input and the virtual instrument.
   */
  Q_PROPERTY(float wetDryMix READ getWetDryMix WRITE setWetDryMix)

  /* Receive MIDI on/off
   *
   * Some plugins react to all MIDI events even when this is not desired.
   * In order to disable MIDI input set to false.
   */
  Q_PROPERTY(bool receiveMidi READ getReceiveMidi WRITE setReceiveMidi)

public:
  EffectPlugin();
  virtual ~EffectPlugin() {}

  virtual bool load() = 0;
  virtual void unload() = 0;

  virtual QString getName() const = 0;

  float getWetDryMix() const;
  void setWetDryMix(float mix);
  bool getReceiveMidi() const;
  void setReceiveMidi(bool receive);

  virtual void openEditor(QWidget *parent) = 0;
  virtual void closeEditor() = 0;
  virtual void queueResizeEditor(int width, int height) = 0;
  virtual void queueIdle() = 0;
  virtual void outputEvents(VstEvents *vstEvents) = 0;

  /* Invoke fn() for each MIDI output event */
  void processOutputEvents(std::function<void (const PmEvent *event)> fn);

  virtual int numInputs() const = 0;
  virtual int numOutputs() const = 0;
  virtual void setSampleRate(int rate) = 0;
  virtual void setTempo(int tempo) = 0;
  virtual void changeMains(bool enable) = 0;
  virtual void processEvents(VstEvents *vstEvents) = 0;
  virtual void process(float **inbuf, float **outbuf, int ns) = 0;

signals:
  void onResizeEditor(int width, int height);
  void onIdle();

public slots:
  virtual void idle() = 0;

private:
  float wetDryMix;
  bool receiveMidi;

protected:
  /* MIDI output events are stored in a buffer until they are written out by
   * the EffectProcessor. This allows the plugin to add events from any thread.
   */
  QMutex outputEventsLock;
  PmEvent outputEventsBuf[EFFECT_PLUGIN_OUTPUT_EVENTS];
  size_t numOutputEvents;
};

#endif /* _EFFECTPLUGIN_H_ */
