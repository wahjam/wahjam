/*
    Copyright (C) 2012-2017 Stefan Hajnoczi <stefanha@gmail.com>

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

#ifndef _AUDIOSTREAM_PA_H_
#define _AUDIOSTREAM_PA_H_

#include "common/audiostream.h"

audioStreamer *create_audioStreamer_PortAudio(const char *hostAPI,
    const char *inputDevice, const char *outputDevice,
    double sampleRate, double latency, SPLPROC proc);
bool portAudioInit();

#endif /* _AUDIOSTREAM_PA_H_ */
