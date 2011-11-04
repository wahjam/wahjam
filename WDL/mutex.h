/*
    WDL - mutex.h
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


  This file provides a simple class that abstracts a mutex or critical section object.
  On Windows it uses CRITICAL_SECTION, on everything else it uses pthread's mutex library.
  
  NOTE: the behavior of the two supported modes is very different, when it comes to code 
  where a single thread locks the mutex twice.
  Take the code below:

    WDL_Mutex m;
    m.Enter();
    m.Enter();
    m.Leave();
    m.Leave();

  On Windows, this code will finish. On non-windows systems, the above code will hang, as the second 
  Enter() will wait infinitely.
*/

#ifndef _WDL_MUTEX_H_
#define _WDL_MUTEX_H_

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif


class WDL_Mutex {
  public:
    WDL_Mutex() 
    {
#ifdef _WIN32
      InitializeCriticalSection(&m_cs);
#else
      pthread_mutex_init(&m_mutex,NULL);
#endif
    }
    ~WDL_Mutex()
    {
#ifdef _WIN32
      DeleteCriticalSection(&m_cs);
#else
      pthread_mutex_destroy(&m_mutex);
#endif
    }

    void Enter()
    {
#ifdef _WIN32
      EnterCriticalSection(&m_cs);
#else
      pthread_mutex_lock(&m_mutex);
#endif
    }

    void Leave()
    {
#ifdef _WIN32
      LeaveCriticalSection(&m_cs);
#else
      pthread_mutex_unlock(&m_mutex);
#endif
    }

  private:
#ifdef _WIN32
  CRITICAL_SECTION m_cs;
#else
  pthread_mutex_t m_mutex;
#endif

};

#endif