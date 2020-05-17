/*
    Copyright (C) 2020 Stefan Hajnoczi <stefanha@jammr.net>

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

#ifndef _LOUDNOISEDETECTOR_H_
#define _LOUDNOISEDETECTOR_H_

#include <math.h>
#include <stdlib.h>

/* Detect loud noises (like feedback). Ideally only undesirable sounds would be
 * detected, but input signals that are generally loud can also trigger the
 * detector. They are just too loud and input volume should be lowered anyway.
 *
 * The detector triggers when all conditions are met:
 * 1. Peak volume is high.
 * 2. Peaks persist for several milliseconds.
 */
class LoudNoiseDetector
{
public:
  LoudNoiseDetector()
  {
    reset();
  }

  void reset()
  {
    numPeaks = 0;
    lastPeakSamples = 0;
  }

  /* Returns true if a loud noise was detected */
  bool process(float *samples, size_t count, int sampleRate)
  {
    const int oneMillisecond = sampleRate / 1000;

    for (size_t i = 0; i < count; i++) {
      /* Wait a bit before retriggering */
      if (lastPeakSamples < oneMillisecond) {
        size_t skip = oneMillisecond - lastPeakSamples;
        if (skip > count - i) {
          skip = count - i;
        }
        lastPeakSamples += skip;
        i += skip - 1; /* the for loop will also increment it */
        continue;
      }

      if (fabs(samples[i]) > 0.8) {
        if (numPeaks++ > 10) {
          reset();
          return true;
        }

        lastPeakSamples = 0;
        continue;
      }

      /* Start again if no successive peak was detected */
      if (lastPeakSamples > 2 * oneMillisecond) {
        numPeaks = 0;
      }

      lastPeakSamples++;
    }

    return false;
  }

private:
  /* Number of peaks detected in a row */
  int numPeaks;

  /* Number of samples since last peak */
  int lastPeakSamples;
};

#endif /* _LOUDNOISEDETECTOR_H_ */
