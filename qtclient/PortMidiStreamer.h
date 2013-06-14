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

#ifndef _PORTMIDICONTROLLER_H_
#define _PORTMIDICONTROLLER_H_

#include <portmidi.h>
#include <porttime.h>
#include <QObject>
#include "common/ConcurrentQueue.h"

class PortMidiStreamer : public QObject
{
  Q_OBJECT

public:
  PortMidiStreamer(QObject *parent = 0);
  void start(const QString &inputDeviceName,
             const QString &outputDeviceName);
  void stop();

  ConcurrentQueue<PmEvent> *getOutputQueue()
  {
    return &outputQueue;
  }

  /* Must be called while stopped */
  void addInputQueue(ConcurrentQueue<PmEvent> *queue);
  void removeInputQueue(ConcurrentQueue<PmEvent> *queue);

  /* Called internally by trampoline function */
  void process(PtTimestamp timestamp);

private:
  PortMidiStream *inputStream;
  PortMidiStream *outputStream;

  ConcurrentQueue<PmEvent> outputQueue;
  QList<ConcurrentQueue<PmEvent> *> inputQueues;

  void processInput();
  void processOutput();
};

bool portMidiInit();

#endif /* _PORTMIDICONTROLLER_H_ */
