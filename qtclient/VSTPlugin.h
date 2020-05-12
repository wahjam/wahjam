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

#ifndef _VSTPLUGIN_H_
#define _VSTPLUGIN_H_

#include <QLibrary>
#include <QDialog>
#include <QMutex>
#include "PluginScanner.h"
#include "EffectPlugin.h"

class VSTScanner : public PluginScanner
{
public:
  QStringList scan(const QStringList &searchPaths) const;
  QString displayName(const QString &fullName) const;
  QString tag() const;
};

class VSTPlugin : public EffectPlugin
{
  Q_OBJECT

public:
  VSTPlugin(const QString &filename);
  ~VSTPlugin();

  bool load();
  void unload();

  QString getName() const;

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

private slots:
  void editorDialogFinished(int result);
  void editorDialogResize(int width, int height);

public slots:
  void idle();

private:
  QLibrary library;
  AEffect *plugin;
  QMutex lock;
  QString name;
  QDialog *editorDialog;
  VstTimeInfo timeInfo;
  bool inProcess;
  bool receiveVstMidiEvents;

  static intptr_t vstAudioMasterCallback(AEffect *plugin, int32_t op,
      int32_t intarg, intptr_t intptrarg, void *ptrarg, float floatarg);
  intptr_t dispatchUnlocked(int op, int intarg = 0, intptr_t intptrarg = 0, void *ptrarg = 0, float floatarg = 0);
  intptr_t dispatch(int op, int intarg = 0, intptr_t intptrarg = 0, void *ptrarg = 0, float floatarg = 0);
};

#endif /* _VSTPLUGIN_H_ */
