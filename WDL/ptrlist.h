/*
    WDL - ptrlist.h
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

  This file provides a simple templated class for a list of pointers. Nothing fancy, and it
  utilizes an internal WDL_HeapBuf for storage.
 
*/

#ifndef _WDL_PTRLIST_H_
#define _WDL_PTRLIST_H_

#include "heapbuf.h"

template<class PTRTYPE> class WDL_PtrList 
{
  public:
    WDL_PtrList()
    {
    }
    ~WDL_PtrList()
    {
    }

    PTRTYPE **GetList() { return (PTRTYPE**)m_hb.Get(); }
    PTRTYPE *Get(int index) { if (GetList() && index >= 0 && index < GetSize()) return GetList()[index]; return NULL; }
    int GetSize(void) { return m_hb.GetSize()/sizeof(PTRTYPE *); }

    PTRTYPE *Add(PTRTYPE *item)
    {
      int s=GetSize();
      m_hb.Resize((s+1)*sizeof(PTRTYPE*));
      return Set(s,item);
    }

    PTRTYPE *Set(int index, PTRTYPE *item) 
    { 
      if (index >= 0 && index < GetSize() && GetList()) return GetList()[index]=item;
      return NULL;
    }

    PTRTYPE *Insert(int index, PTRTYPE *item)
    {
      int s=GetSize();
      m_hb.Resize((s+1)*sizeof(PTRTYPE*));

      if (index<0) index=0;
      else if (index > s) index=s;
      
      int x;
      for (x = s; x > index; x --)
      {
        GetList()[x]=GetList()[x-1];
      }
      return (GetList()[x] = item);
    }

    void Delete(int index)
    {
      PTRTYPE **list=GetList();
      int size=GetSize();
      if (list && index >= 0 && index < size)
      {
        if (index < --size) memcpy(list+index,list+index+1,sizeof(PTRTYPE *)*(size-index));
        m_hb.Resize(size * sizeof(PTRTYPE*));
      }
    }
    void Empty()
    {
      m_hb.Resize(0);
    }

  private:
    WDL_HeapBuf m_hb;
};

#endif

