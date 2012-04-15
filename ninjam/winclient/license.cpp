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

  License dialog related code...

  */

#include <windows.h>
#include <commctrl.h>
#include <math.h>
#include <fstream>
#include <string>

#include "winclient.h"
#include "resource.h"

using namespace std ;


string makeHostString()
{
	string host(g_client->GetHostName()) ;
	size_t colonIdx = host.find_last_of(':') ; host.replace(colonIdx , 1 , "_") ;

	return host ;
}

string makeLicenceFilename()
{
	string licenceFilename(makeHostString()) ; licenceFilename += "_licence.txt" ;

	return licenceFilename ;
}

string makeAgreeKey() { string agreeKey(makeHostString()) ; return (agreeKey = "Agree_" + agreeKey); }

string stripLineEnds(string aString)
{
	int i = aString.length() ; while (i--) if (aString.at(i) == 10 || aString.at(i) == 13) aString.erase(i , 1) ;

	return aString ;
}

void initLicence(HWND hwndDlg , char* licenceText)
{
	// set licence dialog text and read previously accepted licence text from disk
	SetDlgItemText(hwndDlg , IDC_LICENSETEXT , licenceText) ;
	ifstream licenceFileIn(makeLicenceFilename().c_str()) ; string prevLicenceText("") ; string line ;
	if (licenceFileIn.is_open())
	{
		while (licenceFileIn.good()) { getline(licenceFileIn , line) ; prevLicenceText += line ; }
		licenceFileIn.close();
	}

	// auto-accept or force re-acceptance if licence has changed
	if (stripLineEnds(prevLicenceText).compare(stripLineEnds(string(licenceText))))
	{
		if (!prevLicenceText.empty()) SetDlgItemText(hwndDlg , IDC_AGREE_ALWAYS , LICENCE_CHANGED_LBL) ;
		EnableWindow(GetDlgItem(hwndDlg , IDC_AGREE_ALWAYS) , false) ;
		WritePrivateProfileString(TEAMSTREAM_CONFSEC , makeAgreeKey().c_str() , "0" , g_ini_file.Get()) ;
	}
	else
	{
		int isAgree = GetPrivateProfileInt(TEAMSTREAM_CONFSEC , makeAgreeKey().c_str() , 0 , g_ini_file.Get()) ;
		if (isAgree) EndDialog(hwndDlg , 1) ; // press 'OK' automatically
	}
}

void handleAgree(HWND hwndDlg , bool isChecked)
{
	// enable/disable IDC_AGREE_ALWAYS and IDOK
	if (isChecked) SetDlgItemText(hwndDlg , IDC_AGREE_ALWAYS , AGREE_ALWAYS_LBL) ;
	EnableWindow(GetDlgItem(hwndDlg , IDC_AGREE_ALWAYS) , isChecked) ;
  EnableWindow(GetDlgItem(hwndDlg , IDOK) , isChecked) ;
}

void handleAgreeAlways(HWND hwndDlg , bool isChecked)
{
	// write agree state to config
	WritePrivateProfileString(TEAMSTREAM_CONFSEC , makeAgreeKey().c_str() , (isChecked)? "1": "0" , g_ini_file.Get()) ;
	if (!isChecked) return ;

	// write licenceText to disk
	char licenceText[65536] ; (GetDlgItemText(hwndDlg , IDC_LICENSETEXT , licenceText , 65536)) ;
	ofstream licenceFileOut(makeLicenceFilename().c_str()) ;
	if (licenceFileOut.is_open()) { licenceFileOut << licenceText ; licenceFileOut.close() ; }
}

static BOOL WINAPI LicenseProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_INITDIALOG: initLicence(hwndDlg , (char*)lParam) ; return 0 ;
    case WM_CLOSE: EndDialog(hwndDlg , 0) ; return 0 ;
    case WM_COMMAND:
		{
			bool isChecked = (IsDlgButtonChecked(hwndDlg , LOWORD(wParam))) ;
      switch (LOWORD(wParam))
      {
        case IDC_AGREE:						handleAgree(hwndDlg , (isChecked)) ; break;
				case IDC_AGREE_ALWAYS:		handleAgreeAlways(hwndDlg , (isChecked)) ; break ;
        case IDOK:								EndDialog(hwndDlg , 1) ; break ;
        case IDCANCEL:						EndDialog(hwndDlg , 0) ; break ;
      }
		}
    return 0;

  }
  return 0;
}

static char *g_need_license;
static int g_license_result;

int licensecallback(int user32, char *licensetext)
{
  if (!licensetext || !*licensetext) return 1;

  g_need_license=licensetext;
  g_license_result=0;
  g_client_mutex.Leave();
  while (g_need_license && !g_done) Sleep(100);
  g_client_mutex.Enter();
  return g_license_result;
}



void licenseRun(HWND hwndDlg)
{
  if (g_need_license)
  {
    g_license_result=DialogBoxParam(g_hInst,MAKEINTRESOURCE(IDD_LICENSE),hwndDlg,LicenseProc,(LPARAM)g_need_license);
    g_need_license=0;
  }
}
