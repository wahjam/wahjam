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

#include "UserPrivs.h"

unsigned int privsFromString(const QString &s)
{
  unsigned int privs = 0;
  QString lower = s.toLower();

  for (int i = 0; i < lower.size(); i++) {
    QChar ch = lower.at(i);
    if (ch == '*') privs |= ~PRIV_HIDDEN; // everything but hidden if * used
    else if (ch == 't') privs |= PRIV_TOPIC;
    else if (ch == 'b') privs |= PRIV_BPM;
    else if (ch == 'c') privs |= PRIV_CHATSEND;
    else if (ch == 'k') privs |= PRIV_KICK;
    else if (ch == 'r') privs |= PRIV_RESERVE;
    else if (ch == 'm') privs |= PRIV_ALLOWMULTI;
    else if (ch == 'h') privs |= PRIV_HIDDEN;
    else if (ch == 'v') privs |= PRIV_VOTE;
  }
  return privs;
}

QString privsToString(unsigned int privs)
{
  QString s;
  if (privs & PRIV_TOPIC) s.append("t");
  if (privs & PRIV_BPM) s.append("b");
  if (privs & PRIV_CHATSEND) s.append("c");
  if (privs & PRIV_KICK) s.append("k");
  if (privs & PRIV_RESERVE) s.append("r");
  if (privs & PRIV_ALLOWMULTI) s.append("a");
  if (privs & PRIV_HIDDEN) s.append("h");
  if (privs & PRIV_VOTE) s.append("v");
  return s;
}
