/*
    Copyright (C) 2005 Cockos Incorporated

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

/*

  Chat dialog related code...

  */

#include <windows.h>
#include <richedit.h>
#include <commctrl.h>

#include "winclient.h"
#include "resource.h"

HWND g_hwnd ;
bool cfg_color_names_only = false ;


void handleChatColorMsg(char* username , char* msgIn)
{
	int userId = TeamStream::GetUserIdByName(username) ; int idx = atoi(msgIn + COLOR_CHAT_TRIGGER_LEN) ;
	if (userId > USERID_LOCAL) TeamStream::SetChatColorIdx(userId , idx) ;
}

bool parseChatTriggers(char* fullUserName , char* username , char* msgIn , bool isPrivate)
{
	bool isHandleTriggers = (TeamStream::IsLocalTeamStreamUserExist()) ; // initTeamStream() success

	if (!strncmp(msgIn , COLOR_CHAT_TRIGGER , COLOR_CHAT_TRIGGER_LEN))
		{ if (isHandleTriggers) handleChatColorMsg(username , msgIn) ; }
	else return false ; // default chat processing
	return true ; // suppress default chat processing
}

void chatmsg_cb(int user32, NJClient *inst, char **parms, int nparms)
{
  if (!parms[0]) return;

	char* username = TeamStream::TrimUsername(parms[1]) ;	

  if (!strcmp(parms[0],"TOPIC"))
  {
    if (parms[2])
    {
      WDL_String tmp;
      if (parms[1] && *parms[1])
      {
        tmp.Set("<Server> ") ; tmp.Append(username) ;
        if (parms[2][0])
        {
          tmp.Append(" sets topic to: ");
          tmp.Append(parms[2]);
        }
        else
        {
          tmp.Append(" removes topic.");
        }  
      }
      else
      {
        if (parms[2][0])
        {
          tmp.Set("Topic is: ");
          tmp.Append(parms[2]);
        }
        else tmp.Set("No topic is set.");
      }

      g_topic.Set(parms[2]);
      chat_addline("",tmp.Get());
    
    }
  }
  else if (!strcmp(parms[0],"MSG"))
  {
		if (parms[1] && parms[2] && !parseChatTriggers(parms[1] , username , parms[2] , false))
			chat_addline(username , parms[2]) ;
  } 
  else if (!strcmp(parms[0],"PRIVMSG"))
  {
    if (parms[1] && parms[2])
    {
			if (!parseChatTriggers(parms[1] , username , parms[2] , true))
			{
				WDL_String tmp ; tmp.Set("<PM from ") ;
				tmp.Append(username) ; tmp.Append("> ") ; tmp.Append(parms[2]) ;
				chat_addline(NULL,tmp.Get());
			}
    }
  } 
  else if (!strcmp(parms[0],"JOIN") || !strcmp(parms[0],"PART"))
  {
    if (parms[1] && *parms[1])
    {
      WDL_String tmp(username) ;
      tmp.Append(" has ");
      tmp.Append(parms[0][0]=='P' ? "left" : "joined");
      tmp.Append(" the server");
      chat_addline("",tmp.Get());
    }
  } 
}


WNDPROC chatw_oldWndProc,chate_oldWndProc;
BOOL CALLBACK chatw_newWndProc(HWND hwndDlg, UINT uMsg, WPARAM wParam,LPARAM lParam)
{
  if (uMsg == WM_CHAR)
  {
    HWND h=GetDlgItem(GetParent(hwndDlg),IDC_CHATENT);
    SetFocus(h);
    if (wParam == VK_RETURN)
    {
      SendMessage(GetParent(hwndDlg),WM_COMMAND,IDC_CHATOK,0);
      return 0;
    }
    SendMessage(h,uMsg,wParam,lParam);
    return 0;
  }
  return CallWindowProc(chatw_oldWndProc,hwndDlg,uMsg,wParam,lParam);
}
BOOL CALLBACK chate_newWndProc(HWND hwndDlg, UINT uMsg, WPARAM wParam,LPARAM lParam)
{
  if (uMsg == WM_CHAR)
  {
    if (wParam == VK_RETURN)
    {
      SendMessage(GetParent(hwndDlg),WM_COMMAND,IDC_CHATOK,0);
      return 0;
    }
  }
  return CallWindowProc(chatw_oldWndProc,hwndDlg,uMsg,wParam,lParam);
}
void chatInit(HWND hwndDlg)
{
  chatw_oldWndProc=(WNDPROC) SetWindowLong(GetDlgItem(hwndDlg,IDC_CHATDISP),GWL_WNDPROC,(LONG)chatw_newWndProc);
  chate_oldWndProc=(WNDPROC) SetWindowLong(GetDlgItem(hwndDlg,IDC_CHATENT),GWL_WNDPROC,(LONG)chate_newWndProc);

	g_hwnd = hwndDlg ; HWND chatdisplay_hwnd = GetDlgItem(hwndDlg , IDC_CHATDISP) ;
	unsigned mask = SendMessage(chatdisplay_hwnd , EM_GETEVENTMASK , 0 , 0) ;
	SendMessage(chatdisplay_hwnd , EM_SETEVENTMASK , 0 , mask | ENM_LINK) ;
	SendMessage(chatdisplay_hwnd , EM_AUTOURLDETECT , true , 0) ;
// NOTE: Alternatively, lParam can point to a null-terminated string consisting of one or more colon-terminated scheme names that supersede the default scheme name list. For example, the string could be "news:http:ftp:telnet:".
}

WDL_String m_append_text;
extern WDL_Mutex g_client_mutex;

void chat_addline(char *src, char *text)
{
  WDL_String tmp;
  if (src && *src && !strncmp(text,"/me ",4))
  {
    tmp.Set("* ");
    tmp.Append(src);
    tmp.Append(" ");
    char *p=text+3;
    while (*p == ' ') p++;
    tmp.Append(p);
  }
  else
  {
   if (src&&*src)
   {
     tmp.Set("<");
     tmp.Append(src);
     tmp.Append("> ");
   }
		else if (src) tmp.Set("<Server> ") ;

   tmp.Append(text);
  }

  g_client_mutex.Enter();
  if (m_append_text.Get()[0])
    m_append_text.Append("\n");
  m_append_text.Append(tmp.Get());
  g_client_mutex.Leave();

}

void chatRun(HWND hwndDlg)
{
  WDL_String tmp;
  g_client_mutex.Enter();
  tmp.Set(m_append_text.Get());
  m_append_text.Set("");
  g_client_mutex.Leave();

  if (!tmp.Get()[0]) return;
  HWND m_hwnd=GetDlgItem(hwndDlg,IDC_CHATDISP);
  SCROLLINFO si={sizeof(si),SIF_RANGE|SIF_POS|SIF_TRACKPOS,};
  GetScrollInfo(m_hwnd,SB_VERT,&si);

  {
    int oldsels,oldsele;
    SendMessage(m_hwnd,EM_GETSEL,(WPARAM)&oldsels,(LPARAM)&oldsele);
	  char txt[32768];
	  if(strlen(tmp.Get())>sizeof(txt)-1) return;

	  GetWindowText(m_hwnd,txt,sizeof(txt)-1);
	  txt[sizeof(txt)-1]=0;

	  while(strlen(txt)+strlen(tmp.Get())+4>sizeof(txt))
	  {
		  char *p=txt;
		  while(*p!=0 && *p!='\n') p++;
		  if(*p==0) return;
		  while (*p=='\n') p++;
		  strcpy(txt,p);
      oldsels -= p-txt;
      oldsele -= p-txt;
	  }
    if (oldsels < 0) oldsels=0;
    if (oldsele < 0) oldsele=0;

	  if(txt[0]) strcat(txt,"\n");
	  strcat(txt,tmp.Get());

    CHARFORMAT2 cf2;
    cf2.cbSize=sizeof(cf2);
    cf2.dwMask=CFM_LINK;
    cf2.dwEffects=0;
    SendMessage(m_hwnd,EM_SETCHARFORMAT,SCF_ALL,(LPARAM)&cf2);
	  SetWindowText(m_hwnd,txt);

    GetWindowText(m_hwnd,txt,sizeof(txt)-1);
    txt[sizeof(txt)-1]=0;

		char *t = txt ; int sub=0 ; 	char lt = '\n' ;		
    while (*t)
    {
      if (lt == '\n' || lt == '\r')
      {
				int usernameOffset = 0 ; bool isParseTokens ; bool isColorText ;
				if (!strnicmp(t , "<PM from " , 9))	// private chat (choose color per user)
					{ usernameOffset = 9 ; isParseTokens = true ; }
				else if (!strnicmp(t , "<" , 1))			// user chat (choose color per user)
					{ usernameOffset = 1 ; isParseTokens = true ; }
				else isParseTokens = isColorText = false ;

				int lineBegin ; int boldEnd ; int normalBegin ; int lineEnd ; char theUsername[256] ;
				if (isParseTokens)
				{
					// parse username token and set seperator pointers (<bold> normal)
					lineBegin = t - txt - sub ; char* usernameBegin = (t += usernameOffset) ;
					while (*t && *t != '\n' && *t != '\r' && *t != ' ') ++t ;
					boldEnd = t - txt - sub ; normalBegin = boldEnd + 1 ;
					int theUsernameLen = boldEnd - lineBegin - usernameOffset - 1 ;
					if (isColorText = theUsernameLen < 256)
						{ strncpy(theUsername , usernameBegin , theUsernameLen) ; theUsername[theUsernameLen] = '\0' ; }
					while (*t && *t != '\n' && *t != '\r') ++t ; lineEnd = t - txt - sub ;
				}

				COLORREF color ;
				if (isColorText)
				{
					// choose color per user
					int userId = TeamStream::GetUserIdByName(theUsername) ;
					if (isColorText = (userId != USERID_NOBODY))
						color = TeamStream::Get_Chat_Color(TeamStream::GetChatColorIdx(userId)) ;

					// apply char formatting
					CHARFORMAT2 cf2 ; cf2.cbSize = sizeof(cf2) ;
					// bold text - sender name
					cf2.dwMask = CFM_COLOR | CFM_BOLD ; cf2.crTextColor = color ; cf2.dwEffects = CFE_BOLD ;
					SendMessage(m_hwnd , EM_SETSEL , lineBegin , boldEnd) ;
					SendMessage(m_hwnd , EM_SETCHARFORMAT , SCF_SELECTION , (LPARAM)&cf2) ;
					if (!cfg_color_names_only)
					{
						// normal text - everything after bold text
						cf2.dwMask = CFM_COLOR ; cf2.crTextColor = color ;
						SendMessage(m_hwnd , EM_SETSEL , normalBegin , lineEnd) ;
						SendMessage(m_hwnd , EM_SETCHARFORMAT , SCF_SELECTION , (LPARAM)&cf2) ;
					}
				}
      } // if terminator char
      if (*t == '\n') sub++ ;
      if (*t) lt = *t++ ;
		} // while

// NOTE: the original link parser code was unnecessary it is sufficient to
// send EM_AUTOURLDETECT to the richedit20 control (catches most protocols)
// and set its EM_SETEVENTMASK to ENM_LINK to enable event messaging (in chatInit())
// the only advantage to the original approach is catching hand-typed links beginning
// with "www." (but still didn't catch links like "github.com") so for this most part
// it was just wasted cycles (especially seeing as click handling was not implemented)
// still the original code could be built upon to parse hand-typed links that don't
// specify a protocol but ending with ".com" ".org" etc. if anyone thinks this to be useful
// or to allow only certain users to post clickable links (disable EM_AUTOURLDETECT)
// if so we need to reset state here for the 2nd pass
/*
		SendMessage(m_hwnd , EM_SETSEL , oldsels , oldsele) ; t = txt ; lt = ' ' ; sub = 0 ;
		// original link parser code was here ------->
*/

	}

  if (GetFocus() == m_hwnd)      
  {
    SendMessage(m_hwnd, WM_VSCROLL, MAKEWPARAM(SB_THUMBPOSITION,si.nTrackPos),0);
  }
  else
  {
    GetScrollInfo(m_hwnd,SB_VERT,&si);
    SendMessage(m_hwnd, WM_VSCROLL, MAKEWPARAM(SB_THUMBPOSITION,si.nMax),0);
  }
}
