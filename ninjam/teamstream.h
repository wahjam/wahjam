/*
    Copyright (C) 2005 Cockos Incorporated

    TeamStream is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    TeamStream is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with TeamStream; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*
  This file defines the interface for the TeamStream class, .... TODO:
*/

#ifndef _TEAMSTREAM_H_
#define _TEAMSTREAM_H_

// USER_ID #defines
#define N_PHANTOM_USERS 3
/*
#define N_STATIC_USERS 4
*/
#define USERID_NOBODY -4				// phantom 'Nobody' TeamStream user
#define USERNAME_NOBODY "Nobody"
#define USERID_TEAMSTREAM -3		// phantom 'TeamStream' chat user
#define USERNAME_TEAMSTREAM "TeamStream"
#define USERID_SERVER -2				// phantom 'Server' chat user
#define USERNAME_SERVER "Server"
#define USERID_LOCAL -1					// local user

// license.cpp
#define TEAMSTREAM_CONFSEC "teamstream"
#define LICENSE_CHANGED_LBL "This license has ch&anged"
#define AGREE_ALWAYS_LBL "&Always agree for this jam room"

/* chat.cpp defines */
#define N_LINKS 8
#define N_CHAT_COLORS 18
#define CHAT_COLOR_DEFAULT 9
#define CHAT_COLOR_DEFAULT_WIN32 0x00888888
#define CHAT_COLOR_TEAMSTREAM 16
#define CHAT_COLOR_TEAMSTREAM_WIN32 0x00880088
#define CHAT_COLOR_SERVER 12
#define CHAT_COLOR_SERVER_WIN32 0x00008888
#define COLOR_CHAT_TRIGGER "!color "
#define COLOR_CHAT_TRIGGER_LEN 7

/* teamstream.cpp includes */
#include "windows.h"
#include <string>
#include "../WDL/ptrlist.h"

/* license.cpp includes */
//#include <string>

/* chat.cpp includes */
//#include "windows.h"
//#include <string>


class TeamStreamUser
{
	public:
		TeamStreamUser() : m_teamstream_enabled (0) , m_link_idx(N_LINKS) { }
		TeamStreamUser(char* username , int id , int chatColor , char* fullName)
		{
			m_name = username ; m_id = id ;
			m_link_idx = N_LINKS ; m_teamstream_enabled = false ;
			m_chat_color_idx = chatColor ;
			m_full_name = fullName ;
		}
		~TeamStreamUser() { }

		int m_id ;										// autoincremented UID (TeamStream::m_next_id)
		char* m_name ;								// short username "me" vs "me@127.0.0.xxx"
		bool m_teamstream_enabled ;		// false == NinJam mode , true == TeamStream mode
		int m_link_idx ;								// position in the TeamStream chain
		HWND m_gui_handle ;					// handle to user's channels child window
		int m_chat_color_idx ;					// index into per client chat color array
char* m_full_name ; // TODO: lose this (see AddUser() implementation)
} ;


class TeamStream
{
	public:
		TeamStream(void);
		~TeamStream(void);

		// helpers
		static char* TrimUsername(char* username) ;

		// user creation/destruction/query
		static int GetNUsers() ;
		static void InitTeamStream() ;
		static void AddLocalUser(char* username , int chatColor , char* fullUserName) ;
		static bool IsLocalTeamStreamUserExist() ;
		static bool IsUserIdReal(int userId) ;
		static TeamStreamUser* GetUserById(int userId) ;
		static int GetUserIdByName(char* username) ;

		// user state getters/setters
		static int GetChatColorIdx(int userId) ;
		static void SetChatColorIdx(int userId , int chatColorIdx) ;

		// chat messages
		static void SendChatMsg(char* chatMsg) ;
		static void SendChatPvtMsg(char* chatMsg , char* destUserName) ;
		static void SendChatColorChatMsg(bool isPrivate , char* destFullUserName) ;

		// GUI delegates
		static COLORREF (*Get_Chat_Color)(int idx) ;
		static void (*Send_Chat_Message)(char* chatMsg) ; // merged
		static void (*Send_Chat_Pvt_Message)(char* destFullUserName , char* chatMsg) ; // merged

	private:
		static WDL_PtrList<TeamStreamUser> m_teamstream_users ; static int m_next_id ;
		static TeamStreamUser* TeamStream::m_bogus_user ;
} ;

#endif _TEAMSTREAM_H_
