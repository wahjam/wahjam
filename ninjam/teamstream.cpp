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
	For a full description of everything here, see teamstream.h
*/

#include "teamstream.h"

using namespace std ;


// TeamStream users array (private)
WDL_PtrList<TeamStreamUser> TeamStream::m_teamstream_users ; int TeamStream::m_next_id = 0 ;
TeamStreamUser* TeamStream::m_bogus_user = new TeamStreamUser(USERNAME_NOBODY , USERID_NOBODY , CHAT_COLOR_DEFAULT , USERNAME_NOBODY) ;


/* helpers */

char* TeamStream::TrimUsername(char* username)
{
	if (username == NULL) return "" ;

	string name = username ; int idx = name.find("@") ;
	if (idx != -1) name.resize(idx) ; else return username ;

	char* trimmedUsername = new char [name.size() + 1] ; strcpy(trimmedUsername , name.c_str()) ;

	return trimmedUsername ;
}


/* user creation/destruction/queries */

int TeamStream::GetNUsers() { return m_teamstream_users.GetSize() ; }

void TeamStream::InitTeamStream()
{
	m_teamstream_users.Add(new TeamStreamUser(USERNAME_NOBODY , USERID_NOBODY , CHAT_COLOR_DEFAULT , USERNAME_NOBODY)) ;
	m_teamstream_users.Add(new TeamStreamUser(USERNAME_TEAMSTREAM , USERID_TEAMSTREAM , CHAT_COLOR_TEAMSTREAM , USERNAME_TEAMSTREAM)) ;
	m_teamstream_users.Add(new TeamStreamUser(USERNAME_SERVER , USERID_SERVER , CHAT_COLOR_SERVER , USERNAME_SERVER)) ;
}

bool TeamStream::IsLocalTeamStreamUserExist() { return (GetNUsers() > N_PHANTOM_USERS) ; }

bool TeamStream::IsUserIdReal(int userId) { return (GetUserById(userId)->m_id >= USERID_LOCAL) ; }

TeamStreamUser* TeamStream::GetUserById(int userId)
{
	// m_teamstream_users.Get(0) -> USERID_NOBODY , m_teamstream_users.Get(1) -> USERID_TEAMSTREAM
	// m_teamstream_users.Get(2) -> USERID_SERVER , , m_teamstream_users.Get(3) -> USERID_LOCAL
	int i = GetNUsers() ; TeamStreamUser* aUser = m_bogus_user ;
	while (i-- && (aUser = m_teamstream_users.Get(i))->m_id != userId) ;

	return aUser ;
}

int TeamStream::GetUserIdByName(char* username)
{
	int i = GetNUsers() ; TeamStreamUser* aUser = m_bogus_user ;
	while (i-- && strcmp((aUser = m_teamstream_users.Get(i))->m_name , username)) ;

	return aUser->m_id ;
}


/* user state getters/setters */

int TeamStream::GetChatColorIdx(int userId) { return GetUserById(userId)->m_chat_color_idx ; }

void TeamStream::SetChatColorIdx(int userId , int chatColorIdx)
{
	if (!IsUserIdReal(userId)) return ;

	GetUserById(userId)->m_chat_color_idx = chatColorIdx ;
	if (userId == USERID_LOCAL) { SendChatColorChatMsg(false , NULL) ; SendChatMsg(" likes this color better") ; }
}


/* chat messages */

void TeamStream::SendChatMsg(char* chatMsg) { Send_Chat_Message(chatMsg) ; }

void TeamStream::SendChatPvtMsg(char* chatMsg , char* destFullUserName) { Send_Chat_Pvt_Message(destFullUserName , chatMsg) ; }

void TeamStream::SendChatColorChatMsg(bool isPrivate , char* destFullUserName)
{
	char chatMsg[255] ; sprintf(chatMsg , "%s%d" , COLOR_CHAT_TRIGGER , GetChatColorIdx(USERID_LOCAL)) ;
	if (isPrivate) SendChatPvtMsg(chatMsg , destFullUserName) ; else SendChatMsg(chatMsg) ;
}


/* GUI delegates */

COLORREF (*TeamStream::Get_Chat_Color)(int idx) = NULL ;

void (*TeamStream::Send_Chat_Message)(char* chatMsg) = NULL ;

void (*TeamStream::Send_Chat_Pvt_Message)(char* destFullUserName , char* chatMsg) = NULL ;