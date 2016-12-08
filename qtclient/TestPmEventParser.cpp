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
#include "TestPmEventParser.h"

void TestPmEventParser::noteOnOff()
{
  uint8_t input[] = {
    0x90, 0x40, 0x7f, /* Note On */
    0x90, 0x41, 0x32, /* Note On */
    0x80, 0x40, 0x7f, /* Note Off */
    0x80, 0x41, 0x32, /* Note Off */
  };
  PmEventParser parser;
  PmEvent event;

  parser.feed(input, sizeof(input));

  QVERIFY(parser.next(&event));
  QVERIFY(event.message == Pm_Message(0x90, 0x40, 0x7f));
  QVERIFY(parser.next(&event));
  QVERIFY(event.message == Pm_Message(0x90, 0x41, 0x32));
  QVERIFY(parser.next(&event));
  QVERIFY(event.message == Pm_Message(0x80, 0x40, 0x7f));
  QVERIFY(parser.next(&event));
  QVERIFY(event.message == Pm_Message(0x80, 0x41, 0x32));
  QVERIFY(!parser.next(&event));
}

void TestPmEventParser::programChange()
{
  uint8_t input[] = {
    0xc0, 0x7f, /* Program Change */
    0xc1, 0x01, /* Program Change */
  };
  PmEventParser parser;
  PmEvent event;

  parser.feed(input, sizeof(input));

  QVERIFY(parser.next(&event));
  QVERIFY(event.message == Pm_Message(0xc0, 0x7f, 0x00));
  QVERIFY(parser.next(&event));
  QVERIFY(event.message == Pm_Message(0xc1, 0x01, 0x00));
  QVERIFY(!parser.next(&event));
}

void TestPmEventParser::sysex()
{
  uint8_t input[] = {
    0xf0, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0xf7, /* SysEx */
  };
  PmEventParser parser;
  PmEvent event;

  parser.feed(input, sizeof(input));

  QVERIFY(parser.next(&event));
  QVERIFY(event.message == 0x020100f0);
  QVERIFY(parser.next(&event));
  QVERIFY(event.message == 0x06050403);
  QVERIFY(parser.next(&event));
  QVERIFY(event.message == 0x000000f7);
  QVERIFY(!parser.next(&event));
}
