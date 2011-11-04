/*
    WDL - queue.h
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

  This file provides a simple class for a FIFO queue of bytes. It uses a simple buffer,
  so should not generally be used for large quantities of data (it can advance the queue 
  pointer, but Compact() needs to be called regularly to keep memory usage down, and when
  it is called, there's a memcpy() penalty for the remaining data. oh well, is what it is).

*/

#ifndef _WDL_QUEUE_H_
#define _WDL_QUEUE_H_

#include "heapbuf.h"

class WDL_Queue 
{
public:
  WDL_Queue() : m_pos(0)
  {
  }
  ~WDL_Queue()
  {
  }

  void *Add(const void *buf, int len)
  {
    int olen=m_hb.GetSize();
    void *obuf=m_hb.Resize(olen+len,false);
    if (!obuf) return 0;
    if (buf) memcpy((char*)obuf+olen,buf,len);
    return (char*)obuf+olen;
  }

  int GetSize()
  {
    return m_hb.GetSize()-m_pos;
  }

  void *Get()
  {
    void *buf=m_hb.Get();
    if (buf && m_pos >= 0 && m_pos < m_hb.GetSize()) return (char *)buf+m_pos;
    return NULL;
  }

  int Available()
  {
    return m_hb.GetSize() - m_pos;
  }

  void Clear()
  {
    m_pos=0;
    m_hb.Resize(0,false);
  }

  void Advance(int bytecnt)
  {
    m_pos+=bytecnt;
  }

  void Compact(bool allocdown=false)
  {
    if (m_pos > 0)
    {
      int olen=m_hb.GetSize();
      if (m_pos < olen)
      {
        void *a=m_hb.Get();
        if (a) memmove(a,(char*)a+m_pos,olen-m_pos);
        m_hb.Resize(olen-m_pos,allocdown);
      }
      else m_hb.Resize(0,allocdown);
      m_pos=0;
    }
  }

  void SetGranul(int granul) 
  {
    m_hb.SetGranul(granul);
  }


private:
  WDL_HeapBuf m_hb;
  int m_pos;
};


#endif

