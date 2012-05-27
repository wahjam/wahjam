/*
    WDL - sha.cpp
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

  This file provides the implementation to the WDL_SHA1 object, which performs SHA-1 hashes
  on data.

*/

#include "sha.h"

/// sha

WDL_SHA1::WDL_SHA1()
{
  reset();
}

void WDL_SHA1::reset()
{
  lenW = 0;
  size[0] = size[1] = 0;
  H[0] = 0x67452301L;
  H[1] = 0xefcdab89L;
  H[2] = 0x98badcfeL;
  H[3] = 0x10325476L;
  H[4] = 0xc3d2e1f0L;
  unsigned int x;
  for (x = 0; x < sizeof(W)/sizeof(W[0]); x ++) W[x]=0;
}


#define SHA_ROTL(X,n) (((X) << (n)) | ((X) >> (32-(n))))
#define SHUFFLE() E = D; D = C; C = SHA_ROTL(B, 30); B = A; A = TEMP

void WDL_SHA1::add(const void *data, int datalen)
{
  int i;
  for (i = 0; i < datalen; i++) 
  {
    W[lenW / 4] <<= 8;
    W[lenW / 4] |= (uint32_t)((const unsigned char *)data)[i];
    if (!(++lenW & 63)) 
    {
      int t;

      uint32_t A = H[0];
      uint32_t B = H[1];
      uint32_t C = H[2];
      uint32_t D = H[3];
      uint32_t E = H[4];


      for (t = 16; t < 80; t++) W[t] = SHA_ROTL(W[t-3] ^ W[t-8] ^ W[t-14] ^ W[t-16], 1);

      for (t = 0; t < 20; t++) 
      {
        uint32_t TEMP = SHA_ROTL(A,5) + E + W[t] + 0x5a827999L + (((C^D)&B)^D);
        SHUFFLE();
      }
      for (; t < 40; t++) 
      {
        uint32_t TEMP = SHA_ROTL(A,5) + E + W[t] + 0x6ed9eba1L + (B^C^D);
        SHUFFLE();
      }
      for (; t < 60; t++) 
      {
        uint32_t TEMP = SHA_ROTL(A,5) + E + W[t] + 0x8f1bbcdcL + ((B&C)|(D&(B|C)));
        SHUFFLE();
      }
      for (; t < 80; t++) 
      {
        uint32_t TEMP = SHA_ROTL(A,5) + E + W[t] + 0xca62c1d6L + (B^C^D);
        SHUFFLE();
      }

      H[0] += A;
      H[1] += B;
      H[2] += C;
      H[3] += D;
      H[4] += E;

      lenW = 0;
    }
    size[0] += 8;
    if (size[0] < 8) size[1]++;
  }
}

void WDL_SHA1::result(void *out)
{
  unsigned char pad0x80 = 0x80;
  unsigned char pad0x00 = 0x00;
  unsigned char padlen[8];
  int i;
  padlen[0] = (unsigned char)((size[1] >> 24) & 0xff);
  padlen[1] = (unsigned char)((size[1] >> 16) & 0xff);
  padlen[2] = (unsigned char)((size[1] >> 8) & 0xff);
  padlen[3] = (unsigned char)((size[1]) & 0xff);
  padlen[4] = (unsigned char)((size[0] >> 24) & 0xff);
  padlen[5] = (unsigned char)((size[0] >> 16) & 0xff);
  padlen[6] = (unsigned char)((size[0] >> 8) & 0xff);
  padlen[7] = (unsigned char)((size[0]) & 0xff);
  add(&pad0x80, 1);
  while (lenW != 56) add(&pad0x00, 1);
  add(padlen, 8);
  for (i = 0; i < 20; i++) 
  {
    ((unsigned char *)out)[i] = (unsigned char)(H[i / 4] >> 24);
    H[i / 4] <<= 8;
  }
  reset();
}
