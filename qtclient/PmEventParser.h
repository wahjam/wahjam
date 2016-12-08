/*
    Copyright (C) 2016 Stefan Hajnoczi <stefanha@gmail.com>

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

#ifndef _PMEVENTPARSER_H_
#define _PMEVENTPARSER_H_

#include <stdlib.h>
#include <string.h>
#include <portmidi.h>

/* Parse Apple MIDIPacket into PmEvents
 *
 * Packets have one or more complete MIDI messages.  SysEx messages may be
 * incomplete.  For details on MIDIPacket semantics, see
 * <CoreMIDI/MIDIServices.h>.
 */
class PmEventParser
{
public:
  PmEventParser()
    : in(NULL), len(0), state(STATE_READY),
      sysexLen(0)
  {
    memset(sysex, 0, sizeof(sysex));
  }

  void feed(const uint8_t *in, size_t len);
  bool next(PmEvent *event);

private:
  const uint8_t *in;
  size_t len;
  enum {
    STATE_READY,
    STATE_SYSEX,
    STATE_ERROR,
  } state;
  uint8_t sysex[4];
  unsigned int sysexLen;
};

#endif /* _PMEVENTPARSER_H_ */
