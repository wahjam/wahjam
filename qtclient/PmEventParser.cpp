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

#include "PmEventParser.h"

static bool midiStatusIsData(uint8_t status)
{
  return status < 0x80;
}

static bool midiStatusIsSysExStart(uint8_t status)
{
  return status == 0xf0;
}

static bool midiStatusIsSysExEnd(uint8_t status)
{
  return status == 0xf7;
}

static size_t midiMessageLength(uint8_t status)
{
  if (status < 0x80) { /* Data byte */
    return 0;
  } else if (status >= 0x80 && status < 0xc0) {
    return 3;
  } else if (status < 0xe0) {
    return 2;
  } else if (status < 0xf0) {
    return 3;
  } else if (status == 0xf0) { /* Start of SysEx */
    return 0;
  } else if (status == 0xf1) {
    return 2;
  } else if (status == 0xf2) {
    return 3;
  } else if (status == 0xf3) {
    return 2;
  } else { /* System Common or Real-Time */
    return 1;
  }
}

void PmEventParser::feed(const uint8_t *in_, size_t len_)
{
  if (len != 0) {
    state = STATE_ERROR;
    return;
  }

  in = in_;
  len = len_;
}

bool PmEventParser::next(PmEvent *event)
{
  while (len > 0) {
    if (state == STATE_READY) {
      uint8_t status = *in;
      uint8_t param1 = 0;
      uint8_t param2 = 0;
      size_t msgLen;

      if (midiStatusIsData(status)) {
        /* Unexpected data byte */
        state = STATE_ERROR;
        return false;
      }

      if (midiStatusIsSysExStart(status)) {
        in++;
        len--;

        sysex[0] = status;
        sysexLen = 1;

        state = STATE_SYSEX;
        continue;
      }

      msgLen = midiMessageLength(status);

      /* No incomplete messages are allowed (except SysEx) */
      if (msgLen > len) {
        state = STATE_ERROR;
        return false;
      }

      in++;
      len--;

      if (msgLen > 1) {
        param1 = *in;
        in++;
        len--;
      }
      if (msgLen > 2) {
        param2 = *in;
        in++;
        len--;
      }

      *event = (PmEvent){
        .message = Pm_Message(status, param1, param2),
        .timestamp = 0,
      };
      return true;
    } else if (state == STATE_SYSEX) {
      uint8_t data = *in;

      sysex[sysexLen] = data;
      sysexLen++;
      in++;
      len--;

      if (midiStatusIsSysExEnd(data) ||
          sysexLen == 4) {
        *event = (PmEvent){
          .message = (sysex[3] << 24) |
               (sysex[2] << 16) |
               (sysex[1] << 8) |
               sysex[0],
          .timestamp = 0,
        };
        memset(sysex, 0, sizeof(sysex));
        sysexLen = 0;
        if (midiStatusIsSysExEnd(data)) {
          state = STATE_READY;
        }
        return true;
      }
    } else {
      state = STATE_ERROR;
      return false;
    }
  }
  return false;
}
