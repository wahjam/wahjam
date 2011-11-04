/*
    WDL - wndsize.cpp
    Copyright (C) 2004-2005 Cockos Incorporated

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
For information on how to use this class, see wndsize.h :)
*/

#include <windows.h>
#include "wndsize.h"

void WDL_WndSizer::init(HWND hwndDlg)
{
  m_hwnd=hwndDlg;
  GetClientRect(m_hwnd,&m_orig_rect);
  m_list.Resize(0);
}

void WDL_WndSizer::init_item(int dlg_id, float left_scale, float top_scale, float right_scale, float bottom_scale)
{
  RECT this_r;
  HWND h=GetDlgItem(m_hwnd,dlg_id);
  GetWindowRect(h,&this_r);
  ScreenToClient(m_hwnd,(LPPOINT) &this_r);
  ScreenToClient(m_hwnd,((LPPOINT) &this_r)+1);

  int osize=m_list.GetSize();
  m_list.Resize(osize+sizeof(WDL_WndSizer__rec));

  WDL_WndSizer__rec *rec=(WDL_WndSizer__rec *) ((char *)m_list.Get()+osize);

  rec->hwnd=h;
  rec->scales[0]=left_scale;
  rec->scales[1]=top_scale;
  rec->scales[2]=right_scale;
  rec->scales[3]=bottom_scale;

  rec->real_orig = rec->last = rec->orig = this_r;
}

void WDL_WndSizer::onResize(HWND only, int notouch)
{
  WDL_WndSizer__rec *rec=(WDL_WndSizer__rec *) ((char *)m_list.Get());
  int cnt=m_list.GetSize() / sizeof(WDL_WndSizer__rec);

  HDWP hdwndpos=NULL;
  if (!notouch && !only && GetVersion() < 0x80000000) hdwndpos=BeginDeferWindowPos(cnt);
  RECT new_rect;
  GetClientRect(m_hwnd,&new_rect);

  int x;
  for (x = 0; x < cnt; x ++)
  {

    if (rec->hwnd && (!only || only == rec->hwnd))
    {
      RECT r;
      if (rec->scales[0] <= 0.0) r.left = rec->orig.left;
      else if (rec->scales[0] >= 1.0) r.left = rec->orig.left + new_rect.right - m_orig_rect.right;
      else r.left = rec->orig.left + (int) ((new_rect.right - m_orig_rect.right)*rec->scales[0]);

      if (rec->scales[1] <= 0.0) r.top = rec->orig.top;
      else if (rec->scales[1] >= 1.0) r.top = rec->orig.top + new_rect.bottom - m_orig_rect.bottom;
      else r.top = rec->orig.top + (int) ((new_rect.bottom - m_orig_rect.bottom)*rec->scales[1]);

      if (rec->scales[2] <= 0.0) r.right = rec->orig.right;
      else if (rec->scales[2] >= 1.0) r.right = rec->orig.right + new_rect.right - m_orig_rect.right;
      else r.right = rec->orig.right + (int) ((new_rect.right - m_orig_rect.right)*rec->scales[2]);

      if (rec->scales[3] <= 0.0) r.bottom = rec->orig.bottom;
      else if (rec->scales[3] >= 1.0) r.bottom = rec->orig.bottom + new_rect.bottom - m_orig_rect.bottom;
      else r.bottom = rec->orig.bottom + (int) ((new_rect.bottom - m_orig_rect.bottom)*rec->scales[3]);

      if (r.bottom < r.top) r.bottom=r.top;
      if (r.right < r.left) r.right=r.left;
    
      rec->last = r;

      if (!notouch)
      {
        if (!hdwndpos) SetWindowPos(rec->hwnd, NULL, r.left,r.top,r.right-r.left,r.bottom-r.top, SWP_NOZORDER|SWP_NOACTIVATE);
        else hdwndpos=DeferWindowPos(hdwndpos, rec->hwnd, NULL, r.left,r.top,r.right-r.left,r.bottom-r.top, SWP_NOZORDER|SWP_NOACTIVATE);
      }
    }
    rec++;
  }
  if (hdwndpos) EndDeferWindowPos(hdwndpos);

}

WDL_WndSizer__rec *WDL_WndSizer::get_item(int dlg_id)
{
  HWND h=GetDlgItem(m_hwnd,dlg_id);
  WDL_WndSizer__rec *rec=(WDL_WndSizer__rec *) ((char *)m_list.Get());
  int cnt=m_list.GetSize() / sizeof(WDL_WndSizer__rec);
  while (cnt--)
  {
    if (rec->hwnd == h) return rec;
    rec++;
  }
  return 0;
}