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

#ifndef _PORTMIDISTREAMER_H_
#define _PORTMIDISTREAMER_H_

#include <portmidi.h>
#include <QObject>

class PortMidiStreamer : public QObject
{
  Q_OBJECT

public:
  PortMidiStreamer(QObject *parent = 0);
  void start(const QString &inputDeviceName,
             const QString &outputDeviceName,
             int latencyMilliseconds);
  void stop();

  /* Called from audio processing thread */
  bool read(PmEvent *event);
  void write(const PmEvent *event);

private:
  PortMidiStream *inputStream;
  PortMidiStream *outputStream;
};

bool portMidiInit();

#endif /* _PORTMIDISTREAMER_H_ */
