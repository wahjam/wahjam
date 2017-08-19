/*
    NINJAM ClipLogCvt - cliplogcvt.cpp
    Copyright (C) 2005 Cockos Incorporated
    Copyright (C) 2014 Stefan Hajnoczi <stefanha@gmail.com>

    NINJAM is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    NINJAM is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with NINJAM; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <string>
#include <iterator>
#include <sstream>
#include <vector>

#include <libresample.h>

#include "../WDL/vorbisencdec.h"

/* Output audio parameters */
#define SAMPLE_RATE 44100 /* Hz */
#define NCHANNELS 1
#define BITRATE 64 /* kbps */

class AudioBuffer
{
public:
  AudioBuffer(float *samples, int frames, int channels, bool interleaved);
  AudioBuffer(int frames, int channels, bool interleaved);
  ~AudioBuffer();

  void setInterleaved(bool enable);
  void setStereo();
  void setMono();
  void clear();
  void resample(void *resampleState, double factor, bool drain);

  float *getSamples();
  int getFrames();
  int getChannels();

private:
  float *samples;
  int frames;
  int channels;
  bool interleaved;
  bool needDelete;
};

AudioBuffer::AudioBuffer(float *samples_, int frames_, int channels_, bool interleaved_)
  : samples(samples_), frames(frames_), channels(channels_), interleaved(interleaved_), needDelete(false)
{
}

AudioBuffer::AudioBuffer(int frames_, int channels_, bool interleaved_)
  : frames(frames_), channels(channels_), interleaved(interleaved_), needDelete(true)
{
  samples = new float[frames * channels];
}

AudioBuffer::~AudioBuffer()
{
  if (needDelete) {
    delete [] samples;
  }
}

void AudioBuffer::setInterleaved(bool enable)
{
  if (interleaved == enable) {
    return;
  }
  interleaved = enable;
  if (channels == 1) {
    return;
  }

  float *newSamples = new float[frames * channels];

  if (enable) {
    for (int i = 0; i < frames; i++) {
      newSamples[i * 2] = samples[i];
      newSamples[i * 2 + 1] = samples[frames + i];
    }
  } else {
    for (int i = 0; i < frames; i++) {
      newSamples[i] = samples[i * 2];
      newSamples[frames + i] = samples[i * 2 + 1];
    }
  }

  if (needDelete) {
    delete [] samples;
  }
  samples = newSamples;
  needDelete = true;
}

void AudioBuffer::setStereo()
{
  if (channels == 2) {
    return;
  }

  assert(channels == 1);
  channels = 2;
  float *newSamples = new float[frames * channels];

  if (interleaved) {
    for (int i = 0; i < frames; i++) {
      newSamples[i * 2] = newSamples[i * 2 + 1] = samples[i];
    }
  } else {
    memcpy(newSamples, samples, frames * sizeof(float));
    memcpy(&newSamples[frames], samples, frames * sizeof(float));
  }

  if (needDelete) {
    delete [] samples;
  }
  samples = newSamples;
  needDelete = true;
}

void AudioBuffer::setMono()
{
  if (channels == 1) {
    return;
  }

  assert(channels == 2);
  channels = 1;
  float *newSamples = new float[frames * channels];

  if (interleaved) {
    for (int i = 0; i < frames; i++) {
      newSamples[i] = 0.5 * samples[i * 2] + 0.5 * samples[i * 2 + 1];
    }
  } else {
    for (int i = 0; i < frames; i++) {
      newSamples[i] = 0.5 * samples[i] + 0.5 * samples[frames + i];
    }
  }

  if (needDelete) {
    delete [] samples;
  }
  samples = newSamples;
  needDelete = true;
}

void AudioBuffer::clear()
{
  memset(samples, 0, frames * channels * sizeof(float));
}

void AudioBuffer::resample(void *resampleState, double factor, bool drain)
{
  assert(channels == 1);

  int newFrames = (frames + 1) * factor + 1; /* round up */
  float *newSamples = NULL;

  int inFrames = 0;
  int outFrames = 0;
  while (inFrames < frames || outFrames == newFrames) {
    int inUsed = 0;
    int ret;

    /* Resize output buffer.  When draining, samples may be generated beyond
     * the original size we anticipated.
     */
    if (!newSamples || outFrames == newFrames) {
      float *tmp = newSamples;

      newFrames *= 2;
      newSamples = new float[newFrames];

      if (tmp) {
        memcpy(newSamples, tmp, newFrames / 2 * sizeof(newSamples[0]));
        delete [] tmp;
      }
    }

    ret = resample_process(resampleState, factor,
                           &samples[inFrames], frames - inFrames,
                           drain, &inUsed,
                           &newSamples[outFrames], newFrames - outFrames);
    inFrames += inUsed;
    outFrames += ret;
  }

  /* Cap buffer size */
  if (outFrames < newFrames) {
    newFrames = outFrames;
  }

  /* Maybe not all frames were filled by the resampler so cap buffer size */
  frames = newFrames;

  if (needDelete) {
    delete [] samples;
  }
  samples = newSamples;
  needDelete = true;
}

float *AudioBuffer::getSamples()
{
  return samples;
}

int AudioBuffer::getFrames()
{
  return frames;
}

int AudioBuffer::getChannels()
{
  return channels;
}

class UserChannelValueRec
{
public:
  double position;
  double length;
  std::string guidstr;
};

class UserChannelList
{
public:
   std::string user;
   int chidx;

   std::vector<UserChannelValueRec*> items;
};

#define DIRCHAR '/'
#define DIRCHAR_S "/"

bool resolveFile(const char *name, std::string &outpath, const char *path)
{
  const char *p=name;
  while (*p && *p == '0') p++;
  if (!*p) return false;

  const char *exts[] = {".ogg", ".OGG"};
  std::string fnfind;
  size_t x;
  for (x = 0; x < sizeof(exts)/sizeof(exts[0]); x ++)
  {
    fnfind = path;
    fnfind += DIRCHAR_S;
    fnfind += name;
    fnfind += exts[x];

    FILE *tmp=fopen(fnfind.c_str(), "rb");
    if (!tmp)
    {
      fnfind = path;
      fnfind += DIRCHAR_S;
      // try adding guid subdir
      char t[3]={name[0],DIRCHAR,0};
      fnfind += t;

      fnfind += name;
      fnfind += exts[x];
      tmp=fopen(fnfind.c_str(), "rb");
    }
    if (tmp) 
    {
      fseek(tmp,0,SEEK_END);
      int l=ftell(tmp);
      fclose(tmp);
      if (l) 
      {
        outpath = fnfind;
        return true;
      }
    }
  }
  printf("Error resolving guid %s\n",name);
  return false;
}

std::string g_concatdir;

static void fillSilenceSamples(FILE *outfile, VorbisEncoder *encoder, uint64_t framesRemaining)
{
  AudioBuffer zeroes(1024, NCHANNELS, true);
  zeroes.clear();

  while (framesRemaining) {
    uint64_t nframes = framesRemaining;
    if (nframes > 1024) {
      nframes = 1024;
    }

    encoder->Encode(zeroes.getSamples(), nframes);
    framesRemaining -= nframes;

    if (encoder->outqueue.Available() >= 64 * 1024 * 1024 ||
        framesRemaining == 0) {
      fwrite(encoder->outqueue.Get(), 1, encoder->outqueue.Available(), outfile);
      encoder->outqueue.Advance(encoder->outqueue.Available());
    }
  }

  encoder->outqueue.Compact();
}

static void fillSilence(FILE *outfile, VorbisEncoder *encoder, double msecs)
{
  fillSilenceSamples(outfile, encoder, msecs * SAMPLE_RATE / 1000.0);
}

static void transcode(FILE *infile, FILE *outfile, void **resampleState, VorbisEncoder *encoder, double msecs)
{
  uint64_t framesRemaining = msecs * SAMPLE_RATE / 1000.0;
  VorbisDecoder decoder;
  bool drainResampler = false;

  while (framesRemaining) {
    while (decoder.m_samples_used < 1024 * NCHANNELS) {
      size_t nread = fread(decoder.DecodeGetSrcBuffer(4096), 1, 4096, infile);
      if (nread == 0 && decoder.m_samples_used == 0) {
        /* Silence left-over frames and then finish */
        fillSilenceSamples(outfile, encoder, framesRemaining);
        return;
      }
      if (nread > 0) {
        decoder.DecodeWrote(nread);
      }
      if (nread < 4096) {
        drainResampler = true;
        break;
      }
    }

    int nframes = decoder.m_samples_used / decoder.GetNumChannels();
    AudioBuffer abuf((float*)decoder.m_samples.Get(), nframes, decoder.GetNumChannels(), true);
    decoder.m_samples_used = 0;
    if (NCHANNELS == 1) {
      abuf.setMono();
    } else {
      abuf.setStereo();
    }
    if (decoder.GetSampleRate() != SAMPLE_RATE) {
      double factor = (double)SAMPLE_RATE / decoder.GetSampleRate();
      if (!*resampleState) {
        *resampleState = resample_open(1, factor, factor);
      }

      abuf.resample(*resampleState, factor, drainResampler);
    }

    if ((uint64_t)abuf.getFrames() > framesRemaining) {
      encoder->Encode(abuf.getSamples(), framesRemaining);
      framesRemaining = 0;
    } else {
      encoder->Encode(abuf.getSamples(), abuf.getFrames());
      framesRemaining -= abuf.getFrames();
    }

    if (encoder->outqueue.Available() >= 64 * 1024 * 1024 ||
        framesRemaining == 0) {
      fwrite(encoder->outqueue.Get(), 1, encoder->outqueue.Available(), outfile);
      encoder->outqueue.Advance(encoder->outqueue.Available());
    }
  }

  encoder->outqueue.Compact();
}

void WriteOutTrack(const char *chname, UserChannelList *list, int *track_id, const char *path)
{
  std::string outfilename = g_concatdir;
  outfilename += DIRCHAR_S;
  outfilename += chname;
  outfilename += ".ogg";

  if (list->items.size() == 0) {
    return;
  }

  FILE *outfile = fopen(outfilename.c_str(), "wb");
  if (!outfile)
  {
    printf("Unable to open \"%s\" for writing: %m", outfilename.c_str());
    return;
  }

  void *resampleState = NULL;
  VorbisEncoder encoder(SAMPLE_RATE, NCHANNELS, BITRATE, 0);

  double position = 0.0; /* in milliseconds */
  for (size_t i = 0; i < list->items.size(); i++) {
    UserChannelValueRec *rec = list->items[i];

    if (rec->position > position + 1e-5) {
      fillSilence(outfile, &encoder, rec->position - position);
      position = rec->position;
    }

    std::string infilename;
    if (!resolveFile(rec->guidstr.c_str(), infilename, path)) {
      position += rec->length;
      continue; /* skip this interval */
    }

    FILE *infile = fopen(infilename.c_str(), "rb");
    if (!infile) {
      printf("Unable to open \"%s\" for reading: %m", infilename.c_str());
      goto out;
    }

    transcode(infile, outfile, &resampleState, &encoder, rec->length);
    fclose(infile);
    position += rec->length;
  }

  if (resampleState) {
    resample_close(resampleState);
  }

  /* Flush encoder */
  encoder.Encode(NULL, 0);
  fwrite(encoder.outqueue.Get(), 1, encoder.outqueue.Available(), outfile);
  encoder.outqueue.Advance(encoder.outqueue.Available());

  (*track_id)++;

out:
  fclose(outfile);
}

static bool parseCliplog(FILE *logfile, UserChannelList localrecs[32],
                          std::vector<UserChannelList*> &curintrecs)
{
  double cur_bpm = -1.0;
  int cur_bpi = -1;
  int cur_interval = 0;
  double cur_position = 0.0;
  double cur_lenblock = 0.0;

  for (;;)
  {
    char buf[4096];
    buf[0] = '\0';
    if (!fgets(buf, sizeof(buf), logfile) || !buf[0]) {
      break;
    }
    if (buf[strlen(buf) - 1] == '\n') {
      buf[strlen(buf) - 1] = '\0';
    }
    if (!buf[0]) continue;

    std::istringstream stream(buf);
    std::vector<std::string> fields((std::istream_iterator<std::string>(stream)),
                                    std::istream_iterator<std::string>());

    if (fields.size() == 0) {
      continue;
    }
    if (fields[0] == "interval") {
      if (fields.size() != 4)
      {
        printf("interval line has wrong number of tokens\n");
        return false;
      }

      cur_position += cur_lenblock;

      double bpm = 0.0;
      int bpi = 0;
      bpm = atof(fields[2].c_str());
      bpi = atoi(fields[3].c_str());

      cur_bpi = bpi;
      cur_bpm = bpm;

      cur_lenblock = ((double)cur_bpi * 60000.0 / cur_bpm);
      cur_interval++;
    } else if (fields[0] == "local") {
      if (fields.size() != 3)
      {
        printf("local line has wrong number of tokens\n");
        return false;
      }
      UserChannelValueRec *p = new UserChannelValueRec;
      p->position = cur_position;
      p->length = cur_lenblock;
      p->guidstr = fields[1];
      localrecs[atoi(fields[2].c_str()) & 31].items.push_back(p);
    } else if (fields[0] == "user") {
      if (fields.size() != 5)
      {
        printf("user line has wrong number of tokens\n");
        return false;
      }

      std::string username = fields[2];
      if (username.size() < 2 ||
          username[0] != '"' ||
          username[username.size() - 1] != '"') {
        printf("user line has invalid username\n");
        return false;
      }
      username = username.substr(1, username.size() - 2);

      int chidx = atoi(fields[3].c_str());

      UserChannelValueRec *ucvr = new UserChannelValueRec;
      ucvr->guidstr = fields[1];
      ucvr->position = cur_position;
      ucvr->length = cur_lenblock;

      size_t x;
      for (x = 0; x < curintrecs.size(); x ++)
      {
        if (!strcasecmp(curintrecs[x]->user.c_str(), username.c_str()) &&
            curintrecs[x]->chidx == chidx)
        {
          break;
        }
      }
      if (x == curintrecs.size())
      {
        // add the rec
        UserChannelList *t = new UserChannelList;
        t->user = username;
        t->chidx = chidx;

        curintrecs.push_back(t);
      }
      if (curintrecs[x]->items.size())
      {
        UserChannelValueRec *lastitem = curintrecs[x]->items[curintrecs[x]->items.size() - 1]; // this is for when the server sometimes groups them in the wrong interval
        double last_end=lastitem->position + lastitem->length;
        if (ucvr->position < last_end)
        {
          ucvr->position = last_end;
        }
      }
      curintrecs[x]->items.push_back(ucvr);
      // add this record to it
    } else if (fields[0] == "end") {
      /* Do nothing */
    } else {
      printf("unknown token %s\n", fields[0].c_str());
      return false;
    }
  }
  return true;
}

int main(int argc, char **argv)
{
  if (argc != 2) {
    printf("Usage: %s <session_directory>\n", argv[0]);
    return 1;
  }

  std::string logfn = argv[1];
  logfn += DIRCHAR_S "clipsort.log";
  FILE *logfile = fopen(logfn.c_str(), "rt");
  if (!logfile) {
    printf("Error opening logfile \"%s\": %m\n", logfn.c_str());
    return 1;
  }

  UserChannelList localrecs[32];
  std::vector<UserChannelList*> curintrecs;
  if (!parseCliplog(logfile, localrecs, curintrecs)) {
    return 1;
  }

  fclose(logfile);

  printf("Done analyzing log, building output...\n");

  g_concatdir = argv[1];
  g_concatdir += DIRCHAR_S "concat";
  mkdir(g_concatdir.c_str(), 0755);

  int track_id=0;
  size_t x;
  for (x= 0; x < sizeof(localrecs)/sizeof(localrecs[0]); x ++)
  {
    char chname[512];
    sprintf(chname,"local_%02zu",x);
    WriteOutTrack(chname,localrecs+x, &track_id, argv[1]);

  }
  for (x= 0; x < curintrecs.size(); x ++)
  {
    char chname[4096];
    snprintf(chname, sizeof(chname), "%s_%02zu", curintrecs[x]->user.c_str(), x);
    char *p=chname;
    while (*p)
    {
      if (*p == '/'||*p == '\\' || *p == '?' || *p == '*' || *p == ':' || *p == '\'' || *p == '\"' || *p == '|' || *p == '<' || *p == '>') *p='_';
      p++;
    }

    WriteOutTrack(chname, curintrecs[x], &track_id, argv[1]);
  }
  printf("wrote %d tracks\n", track_id);

  return 0;
}
