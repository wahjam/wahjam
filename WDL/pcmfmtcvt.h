/*
    WDL - pcmfmtcvt.h
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

  This file provides some simple functions for dealing with PCM audio.
  Specifically: 
    + mix (and optionally resample, using low quality linear interpolation) a block of floats to another.
 
*/

#ifndef _PCMFMTCVT_H_
#define _PCMFMTCVT_H_

static int resampleLengthNeeded(int src_srate, int dest_srate, int dest_len, double *state)
{
  // safety
  if (!src_srate) src_srate=48000;
  if (!dest_srate) dest_srate=48000;
  if (src_srate == dest_srate) return dest_len;
  return (int) (((double)src_srate*(double)dest_len/(double)dest_srate)+*state);

}

static void mixFloatsNIOutput(float *src, int src_srate, int src_nch,  // lengths are sample pairs. input is interleaved samples, output not
                            float **dest, int dest_srate, int dest_nch, 
                            int dest_len, float vol, float pan, double *state)
{
  // fucko: better resampling, this is shite
  int x;
  if (pan < -1.0f) pan=-1.0f;
  else if (pan > 1.0f) pan=1.0f;
  if (vol > 4.0f) vol=4.0f;
  if (vol < 0.0f) vol=0.0f;

  if (!src_srate) src_srate=48000;
  if (!dest_srate) dest_srate=48000;

  double vol1=vol,vol2=vol;
  float *dest1=dest[0];
  float *dest2=NULL;
  if (dest_nch > 1)
  {
    dest2=dest[1];
    if (pan < 0.0f)  vol2 *= 1.0f+pan;
    else if (pan > 0.0f) vol1 *= 1.0f-pan;
  }
  

  double rspos=*state;
  double drspos = 1.0;
  if (src_srate != dest_srate) drspos=(double)src_srate/(double)dest_srate;

  for (x = 0; x < dest_len; x ++)
  {
    double ls,rs;
    if (src_srate != dest_srate)
    {
      int ipos = (int)rspos;
      double fracpos=rspos-ipos; 
      if (src_nch == 2)
      {
        ipos+=ipos;
        ls=src[ipos]*(1.0-fracpos) + src[ipos+2]*fracpos;
        rs=src[ipos+1]*(1.0-fracpos) + src[ipos+3]*fracpos;
      }
      else 
      {
        rs=ls=src[ipos]*(1.0-fracpos) + src[ipos+1]*fracpos;
      }
      rspos+=drspos;

    }
    else
    {
      if (src_nch == 2)
      {
        int t=x+x;
        ls=src[t];
        rs=src[t+1];
      }
      else
      {
        rs=ls=src[x];
      }
    }

    ls *= vol1;
    if (ls > 1.0) ls=1.0;
    else if (ls<-1.0) ls=-1.0;

    *dest1++ +=(float) ls;

    if (dest_nch > 1)
    {
      rs *= vol2;
      if (rs > 1.0) rs=1.0;
      else if (rs<-1.0) rs=-1.0;

      *dest2++ += (float) rs;
    }
  }
  *state = rspos - (int)rspos;
}


#endif //_PCMFMTCVT_H_
