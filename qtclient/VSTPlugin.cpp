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

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdio.h>
#include <QMutexLocker>
#include "VSTPlugin.h"

VSTPlugin::VSTPlugin(const QString &filename)
  : library(filename), plugin(NULL), editorDialog(NULL), inProcess(false)
{
  /* audioMasterCallback may be invoked from any thread.  Use signal/slot to
   * dispatch in the GUI thread.
   */
  connect(this, SIGNAL(onResizeEditor(int, int)),
          this, SLOT(editorDialogResize(int, int)));
  connect(this, SIGNAL(onIdle()),
          this, SLOT(idle()));

  memset(&timeInfo, 0, sizeof(timeInfo));
  timeInfo.timeSigNumerator = 4;
  timeInfo.timeSigDenominator = 4;
  timeInfo.flags |= kVstTimeSigValid;
}

VSTPlugin::~VSTPlugin()
{
  unload();
}

intptr_t VSTPlugin::vstAudioMasterCallback(AEffect *plugin, int32_t op,
    int32_t intarg, intptr_t intptrarg, void *ptrarg, float floatarg)
{
  VSTPlugin *vst = NULL;
  if (plugin) {
    vst = (VSTPlugin*)plugin->user;
  }

  switch (op) {
  case audioMasterAutomate:
    return 0; /* ignore */

  case audioMasterVersion:
    return 2300;

  case audioMasterIdle:
    if (vst) {
      vst->queueIdle();
    }
    return 0;

  case audioMasterWantMidi:
    return 1; /* success */

  case audioMasterGetTime:
    if (vst) {
      return (intptr_t)&vst->timeInfo;
    }
    return 0;

  case audioMasterSizeWindow:
    if (!vst) {
      return 0;
    }
    vst->queueResizeEditor(intarg, intptrarg);
    return 1;

  case audioMasterGetCurrentProcessLevel:
    if (vst) {
      return vst->inProcess ? 2 : 1;
    }
    return 0;

  case audioMasterGetVendorString:
    strcpy((char*)ptrarg, ORGNAME);
    return 1;

  case audioMasterGetProductString:
    strcpy((char*)ptrarg, APPNAME);
    return 1;

  case audioMasterGetVendorVersion:
    return 1;

  case audioMasterCanDo:
    if (strcmp((const char*)ptrarg, "sizeWindow") == 0) {
      return 1;
    }
    return 0;

  case audioMasterUpdateDisplay:
    if (vst) {
      vst->queueIdle();
    }
    return 0;

  case audioMasterBeginEdit:
    return 1; /* ignore */

  case audioMasterEndEdit:
    return 1; /* ignore */

  default:
    /* qDebug() is not thread-safe */
    fprintf(stderr, "%s plugin %p op %d intarg %d intptrarg %" PRIdPTR " ptrarg %p floatarg %f\n",
           __func__, plugin, op, intarg, intptrarg, ptrarg, floatarg);
    return 0;
  }
}

bool VSTPlugin::load()
{
  if (plugin) {
    return false;
  }

  qDebug("VST filename: %s", library.fileName().toLatin1().constData());

  typedef AEffect *(VSTMainFn)(audioMasterCallback);
  VSTMainFn *mainfn = (VSTMainFn*)library.resolve("main");
  if (!mainfn) {
    qDebug("failed to resolve VST plugin main symbol");
    library.unload();
    return false;
  }

  lock.lock();
  plugin = mainfn(VSTPlugin::vstAudioMasterCallback);
  if (!plugin) {
    qDebug("mainfn returned NULL plugin");
    lock.unlock();
    return false;
  }
  if (plugin->magic != kEffectMagic) {
    qDebug("invalid VST plugin magic number %#x", plugin->magic);
    lock.unlock();
    return false;
  }

  plugin->user = this;
  dispatchUnlocked(effOpen);
  dispatchUnlocked(effSetBlockSize, 0, 4096);

  qDebug("VST version: %" PRIuPTR,
         dispatchUnlocked(effGetVstVersion));
  qDebug("VST unique ID: %#x", plugin->uniqueID);

  char vendor[65] = {};
  dispatchUnlocked(effGetVendorString, 0, 0, (void*)vendor);
  qDebug("VST vendor: %s", vendor);

  char effectName[65] = {};
  dispatchUnlocked(effGetEffectName, 0, 0, (void*)effectName);
  qDebug("VST effect name: %s", effectName);
  name = QString::fromUtf8(effectName);

  char product[65] = {};
  dispatchUnlocked(effGetProductString, 0, 0, (void*)product);
  qDebug("VST product string: %s", product);

  lock.unlock();

  qDebug("VST has editor: %s",
         plugin->flags & effFlagsHasEditor ? "yes" : "no");
  qDebug("VST is synth: %s",
         plugin->flags & effFlagsIsSynth ? "yes" : "no");
  qDebug("VST can replacing: %s",
         plugin->flags & effFlagsCanReplacing ? "yes" : "no");

  qDebug("VST number of inputs: %d", plugin->numInputs);
  qDebug("VST number of outputs: %d", plugin->numOutputs);

  if (!(plugin->flags & effFlagsHasEditor)) {
    qDebug("Refusing to load plugin without editor");
    unload();
    return false;
  }
  return true;
}

void VSTPlugin::unload()
{
  if (plugin) {
    closeEditor();
    lock.lock();
    dispatchUnlocked(effMainsChanged, 0, 0);
    dispatchUnlocked(effClose);
    plugin = NULL;
    lock.unlock();
  }
  if (library.isLoaded()) {
    library.unload();
  }
}

QString VSTPlugin::getName() const
{
  return name;
}

void VSTPlugin::openEditor(QWidget *parent)
{
  if (!plugin) {
    return;
  }
  if (editorDialog) {
    editorDialog->show();
    return;
  }
  if (!(plugin->flags & effFlagsHasEditor)) {
    return;
  }

  struct {
    short top;
    short left;
    short bottom;
    short right;
  } *rect = NULL;
  dispatch(effEditGetRect, 0, 0, &rect);
  if (!rect) {
    qDebug("VST plugin returned NULL edit rect");
    return;
  }

  QDialog *dialog = new QDialog(parent, Qt::Window);
  connect(dialog, SIGNAL(finished(int)),
          this, SLOT(editorDialogFinished(int)));
  dialog->setWindowTitle(name + " [VST Plugin]");
  dialog->resize(rect->right - rect->left, rect->bottom - rect->top);
  dialog->show(); /* must show before calling effectiveWinId() */

  dispatch(effEditOpen, 0, 0, (void*)dialog->effectiveWinId());

  /* Some plugins don't return the real size until after effEditOpen */
  dispatch(effEditGetRect, 0, 0, &rect);
  if (rect) {
    dialog->resize(rect->right - rect->left, rect->bottom - rect->top);
  }

  editorDialog = dialog;
}

void VSTPlugin::closeEditor()
{
  if (editorDialog) {
    editorDialog->deleteLater();
    editorDialog = NULL;
    dispatch(effEditClose);
  }
}

void VSTPlugin::editorDialogFinished(int)
{
  closeEditor();
}

void VSTPlugin::idle()
{
  /* If the audio thread and GUI thread contend on the lock there can be audio
   * drop-outs.  The idle method is invoked from a timer callback in the GUI
   * thread or when the plugin invokes the audioMasterCallback and is
   * particularly prone to contending on the lock.
   *
   * So make an exception here and don't lock against the audio thread.  If we
   * hit VSTs that cannot handle processReplacing() and effEditIdle concurrency
   * then we need to rethink this.
   */
  if (editorDialog) {
    dispatchUnlocked(effEditIdle);
  }
}

void VSTPlugin::setSampleRate(int rate)
{
  timeInfo.sampleRate = rate;

  dispatch(effSetSampleRate, 0, 0, 0, rate);
}

void VSTPlugin::setTempo(int tempo)
{
  timeInfo.tempo = tempo;
  timeInfo.flags |= kVstTempoValid;
}

int VSTPlugin::numInputs() const
{
  return plugin->numInputs;
}

int VSTPlugin::numOutputs() const
{
  return plugin->numOutputs;
}

void VSTPlugin::changeMains(bool enable)
{
  dispatch(effMainsChanged, 0, enable);

  /* Restart sample timer */
  if (enable) {
    timeInfo.samplePos = 0;
    timeInfo.flags |= kVstTransportPlaying;
  } else {
    timeInfo.flags &= ~kVstTransportPlaying;
  }
}

void VSTPlugin::process(float **inbuf, float **outbuf, int ns)
{
  int i;

  /* Silence output buffer, some plugins mix into the output */
  for (i = 0; i < numOutputs(); i++) {
    memset(outbuf[i], 0, sizeof(float) * ns);
  }

  if (!lock.tryLock()) {
    timeInfo.samplePos += ns;
    return;
  }

  inProcess = true;

  if (plugin->flags & effFlagsCanReplacing) {
    plugin->processReplacing(plugin, inbuf, outbuf, ns);
  } else {
    plugin->process(plugin, inbuf, outbuf, ns);
  }

  inProcess = false;

  lock.unlock();

  timeInfo.samplePos += ns;
}

intptr_t VSTPlugin::dispatchUnlocked(int op, int intarg, intptr_t intptrarg, void *ptrarg, float floatarg)
{
  if (!plugin) {
    return 0;
  }
  return plugin->dispatcher(plugin, op, intarg, intptrarg, ptrarg, floatarg);
}

intptr_t VSTPlugin::dispatch(int op, int intarg, intptr_t intptrarg, void *ptrarg, float floatarg)
{
  QMutexLocker locker(&lock);
  return dispatchUnlocked(op, intarg, intptrarg, ptrarg, floatarg);
}

/* May be invoked from non-GUI thread */
void VSTPlugin::queueResizeEditor(int width, int height)
{
  emit onResizeEditor(width, height);
}

/* Invoked from GUI thread */
void VSTPlugin::editorDialogResize(int width, int height)
{
  if (editorDialog) {
    editorDialog->resize(width, height);
  }
}

/* May be invoked from non-GUI thread */
void VSTPlugin::queueIdle()
{
  emit onIdle();
}
