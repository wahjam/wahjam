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

#ifndef _USERPRIVS_H_
#define _USERPRIVS_H_

#include <QString>

enum {
  PRIV_TOPIC = 1,       // may set topic
  PRIV_CHATSEND = 2,    // may send chat messages
  PRIV_BPM = 4,         // may set bpm/bpi (overrides vote)
  PRIV_KICK = 8,        // may kick users
  PRIV_RESERVE = 16,    // user does not count towards slot limit
  PRIV_ALLOWMULTI = 32, // may connect multiple times with same name (subsequent users append -X to them)
  PRIV_HIDDEN = 64,     // hidden user, doesn't count for a slot, too
  PRIV_VOTE = 128,      // may vote
};

unsigned int privsFromString(const QString &s);
QString privsToString(unsigned int privs);

#endif /* _USERPRIVS_H_ */
