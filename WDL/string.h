/*
    WDL - string.h
    Copyright (C) 2005 Cockos Incorporated

    WDL is dual-licensed. You may modify and/or distribute WDL under either of 
    the following  licenses:
    
      This software is provided 'as-is', without any express or implied
      warranty.  In no event will the authors be held liable for any damages
      arising from the use of this software.

      Permission is granted to anyone to use this software for any purpose,
      including commercial applications, and to alter it and redistribute it
      freely, subject to the following restrictions:

      1. The origin of this software must not be misrepresented; you must not
         claim that you wrote the original software. If you use this software
         in a product, an acknowledgment in the product documentation would be
         appreciated but is not required.
      2. Altered source versions must be plainly marked as such, and must not be
         misrepresented as being the original software.
      3. This notice may not be removed or altered from any source distribution.
      

    or:

      WDL is free software; you can redistribute it and/or modify
      it under the terms of the GNU General Public License as published by
      the Free Software Foundation; either version 2 of the License, or
      (at your option) any later version.

      WDL is distributed in the hope that it will be useful,
      but WITHOUT ANY WARRANTY; without even the implied warranty of
      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
      GNU General Public License for more details.

      You should have received a copy of the GNU General Public License
      along with WDL; if not, write to the Free Software
      Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*

  This file provides a simple class for variable-length string manipulation.
  It provides only the simplest features, and does not do anything confusing like
  operator overloading. It uses a WDL_HeapBuf for internal storage.
 
*/

#ifndef _WDL_STRING_H_
#define _WDL_STRING_H_

#include "heapbuf.h"

class WDL_String
{
public:
  WDL_String(const char *initial=NULL, int initial_len=0)
  {
    if (initial) Set(initial,initial_len);
  }
  WDL_String(WDL_String &s)
  {
    Set(s.Get());
  }
  WDL_String(WDL_String *s)
  {
    if (s && s != this) Set(s->Get());
  }
  ~WDL_String()
  {
  }

  void Set(const char *str, int maxlen=0)
  {
    int s=strlen(str);
    if (maxlen && s > maxlen) s=maxlen;   

    char *newbuf=(char*)m_hb.Resize(s+1);
    if (newbuf) 
    {
      memcpy(newbuf,str,s);
      newbuf[s]=0;
    }
  }

  void Append(const char *str, int maxlen=0)
  {
    int s=strlen(str);
    if (maxlen && s > maxlen) s=maxlen;

    int olds=strlen(Get());

    char *newbuf=(char*)m_hb.Resize(olds + s + 1);
    if (newbuf)
    {
      memcpy(newbuf + olds, str, s);
      newbuf[olds+s]=0;
    }
  }

  void DeleteSub(int position, int len)
  {
	  char *p=Get();
	  if (!p || !*p) return;

	  int l=strlen(p);
	  if (position < 0 || position >= l) return;
	  if (position+len > l) len=l-position;
	  memmove(p+position,p+position+len,l-position-len+1); // +1 for null
  }

  void Insert(const char *str, int position, int maxlen=0)
  {
	  int sl=strlen(Get());
	  if (position > sl) position=sl;

	  int ilen=strlen(str);
	  if (maxlen > 0 && maxlen < ilen) ilen=maxlen;

	  Append(str);
	  char *cur=Get();

      	  memmove(cur+position+ilen,cur+position,sl-position);
	  memcpy(cur+position,str,ilen);
	  cur[sl+ilen]=0;
  }

  void SetLen(int length) // sets number of chars allocated for string, not including null terminator
  {                       // can use to resize down, too, or resize up for a sprintf() etc
    char *b=(char*)m_hb.Resize(length+1);
    if (b) b[length]=0;
  }

  char *Get()
  {
    if (m_hb.Get()) return (char *)m_hb.Get();
    return "";
  }

  private:
    WDL_HeapBuf m_hb;
};

#endif

