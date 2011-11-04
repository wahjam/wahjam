/*
    WDL - dirscan.h
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

  This file provides the interface and implementation for WDL_DirScan, a simple 
  (and somewhat portable) directory reading class. On non-Win32 systems it wraps
  opendir()/readdir()/etc. On Win32, it uses FindFirst*, and supports wildcards as 
  well.
 
*/


#ifndef _WDL_DIRSCAN_H_
#define _WDL_DIRSCAN_H_

#include "string.h"

#ifndef _WIN32
#include <sys/types.h>
#include <dirent.h>
#endif

class WDL_DirScan
{
  public:
    WDL_DirScan() : 
#ifdef _WIN32
       m_h(INVALID_HANDLE_VALUE)
#else
       m_h(NULL), m_ent(NULL)
#endif
    {
    }

    ~WDL_DirScan()
    {
      Close();
    }

    int First(const char *dirname
#ifdef _WIN32
                , int isExactSpec=0
#endif
     ) // returns 0 if success
    {
      int l=strlen(dirname);
      if (l < 1) return -1;

      WDL_String scanstr(dirname);
#ifdef _WIN32
      if (!isExactSpec) 
#endif
	if (dirname[l-1] == '\\' || dirname[l-1] == '/') scanstr.SetLen(l-1);

      m_leading_path.Set(scanstr.Get());

#ifdef _WIN32
      if (!isExactSpec) scanstr.Append("\\*");
      else
      {
        // remove trailing shit from m_leading_path
        char *p=m_leading_path.Get();
        while (*p) p++;
        while (p > m_leading_path.Get() && *p != '/' && *p != '\\') p--;
        if (p > m_leading_path.Get()) *p=0;
      }
#endif

      Close();
#ifdef _WIN32
      m_h=FindFirstFile(scanstr.Get(),&m_fd);
      return (m_h == INVALID_HANDLE_VALUE);
#else
      m_ent=0;
      m_h=opendir(scanstr.Get());
      return !m_h;
#endif
    }
    int Next() // returns 0 on success
    {
#ifdef _WIN32
      if (m_h == INVALID_HANDLE_VALUE) return -1;
      return !FindNextFile(m_h,&m_fd);
#else
      if (!m_h) return -1;
      return !(m_ent=readdir(m_h));
#endif
    }
    void Close()
    {
#ifdef _WIN32
      if (m_h != INVALID_HANDLE_VALUE) FindClose(m_h);
      m_h=INVALID_HANDLE_VALUE;
#else
      if (m_h) closedir(m_h);
      m_h=0; m_ent=0;
#endif
    }

#ifdef _WIN32
    char *GetCurrentFN() { return m_fd.cFileName; }
#else
    char *GetCurrentFN() { return m_ent?m_ent->d_name : (char *)""; }
#endif
    void GetCurrentFullFN(WDL_String *str) 
    { 
      str->Set(m_leading_path.Get()); 
#ifdef _WIN32
      str->Append("\\"); 
#else
      str->Append("/"); 
#endif
      str->Append(GetCurrentFN()); 
    }
    int GetCurrentIsDirectory() { 
#ifdef _WIN32
       return !!(m_fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY); 
#else
       return m_ent && (m_ent->d_type & DT_DIR);
#endif
    }

    // these are somewhat windows specific calls, eh
#ifdef _WIN32
    DWORD GetCurrentFileSize(DWORD *HighWord=NULL) { if (HighWord) *HighWord = m_fd.nFileSizeHigh; return m_fd.nFileSizeLow; }
    void GetCurrentLastWriteTime(FILETIME *ft) { *ft = m_fd.ftLastWriteTime; }
    void GetCurrentLastAccessTime(FILETIME *ft) { *ft = m_fd.ftLastAccessTime; }
    void GetCurrentCreationTime(FILETIME *ft) { *ft = m_fd.ftCreationTime; }
#endif

  private:
#ifdef _WIN32
    WIN32_FIND_DATA m_fd;
    HANDLE m_h;
#else
    DIR *m_h;
    struct dirent *m_ent;
#endif
    WDL_String m_leading_path;
};

#endif
