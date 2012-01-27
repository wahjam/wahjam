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

  Some utility functions common to clients.

*/


#include <string.h>
#include <stdio.h>
#include <math.h>
#include <float.h>

#include "njmisc.h"

// dB related utilities

double DB2SLIDER(double x)
{
  double d=pow(2110.54*fabs(x),1.0/3.0);
  if (x < 0.0) d=-d;
  return d + 63.0;
}

double SLIDER2DB(double y)
{
  return pow(y-63.0,3.0) * (1.0/2110.54);
}

double VAL2DB(double x)
{
  static double g_ilog2x6;
  static int a;
  if (!a)
  {
    a++;
    g_ilog2x6 = 6.0/log10(2.0);
  }
  double v=(log10(x)*g_ilog2x6);
  if (v < -120.0) v=-120.0;
  return v;
}

void mkvolpanstr(char *str, double vol, double pan)
{
  mkvolstr(str,vol);
  char *p=str+strlen(str);
  *p++=' ';
  mkpanstr(p,pan);
}

void mkpanstr(char *str, double pan)
{
  if (fabs(pan) < 0.0001) strcpy(str,"center");
  else sprintf(str,"%d%%%s", (int)fabs(pan*100.0),(pan>0.0 ? "R" : "L"));
}

void mkvolstr(char *str, double vol)
{
  double v=VAL2DB(vol);
  if (vol < 0.0000001 || v < -120.0) v=-120.0;
  sprintf(str,"%s%2.1fdB",v>0.0?"+":"",v);   
}
