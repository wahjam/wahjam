/*
    NINJAM ClipLogCvt - cliplogcvt.cpp
    Copyright (C) 2005 Cockos Incorporated

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

#include "../WDL/vorbisencdec.h"

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

int resolveFile(const char *name, std::string &outpath, const char *path)
{
  const char *p=name;
  while (*p && *p == '0') p++;
  if (!*p) return 0; // empty name

  const char *exts[] = {".ogg", ".OGG"};
  std::string fnfind;
  int x;
  for (x = 0; x < (int)(sizeof(exts)/sizeof(exts[0])); x ++)
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
        char buf[4096];
        if (realpath(fnfind.c_str(), buf))
        {
          outpath = buf;
        }       
        return 1;
      }
    }
  }
  printf("Error resolving guid %s\n",name);
  return 0;

}

void usage(const char *argv)
{
   printf("Usage: %s <session_directory>\n", argv[0]);
   exit(1);
}

std::string g_concatdir;

void WriteOutTrack(const char *chname, UserChannelList *list, int *track_id, const char *path)
{
  int y;
  FILE *concatout=NULL;
  double last_pos=-1000.0, last_len=0.0;
  std::string concat_fn;
  int concat_filen=0;

  const double DELTA=0.0000001;
  for (y = 0; y < list->items.size(); y ++)
  {
    std::string op;
    if (!resolveFile(list->items[y]->guidstr.c_str(), op, path)) 
    {
      if (concatout) 
      {
        if (concatout) fclose(concatout);
      }
      concatout=0;
      continue;
    }

    fprintf(stderr, "%s position %g\n", op.c_str(), list->items[y]->position);

/*    if (concatout && fabs(last_pos+last_len - list->items[y]->position) > DELTA)
    {
      if (concatout) fclose(concatout);
      concatout=0;
    }

    if (!concatout)
    {
      concat_fn = g_concatdir;
      char buf[4096];
      sprintf(buf, DIRCHAR_S "%s_%03d.%s", chname, concat_filen++, "ogg");
      concat_fn += buf;

      if (realpath(concat_fn.c_str(),buf))
      {
        concat_fn = buf;
      }

      concatout = fopen(concat_fn.c_str(), "wb");
      if (!concatout)
      {
        printf("Warning: error opening %s. RESULTING TXT WILL LACK REFERENCE TO THIS FILE! ACK!\n", concat_fn.c_str());
      }
      last_pos = list->items[y]->position;
      last_len = 0.0;
    }

    if (concatout)
    {
      FILE *fp = fopen(op.c_str(),"rb");
      if (fp)
      {
        // decode file
        VorbisDecoder decoder;
        int did_write=0;

        for (;;)
        {
          int l=fread(decoder.DecodeGetSrcBuffer(1024),1,1024,fp);
          decoder.DecodeWrote(l);

          if (decoder.m_samples_used>0)
          {

            if (concatout_wav->Status() && (decoder.GetNumChannels() != concatout_wav->get_nch() || decoder.GetSampleRate() != concatout_wav->get_srate()))
            {
              // output parameter change
//                printf("foo\n");

              delete concatout_wav;

              concat_fn = g_concatdir;
              char buf[4096];
              sprintf(buf,DIRCHAR_S "%s_%03d.wav",chname,concat_filen++);
              concat_fn += buf;

              if (realpath(concat_fn.c_str(), buf))
              {
                concat_fn = buf;
              }

              concatout_wav = new WaveWriter;
              last_pos = list->items[y]->position;
              last_len = 0.0;

            }

            if (!concatout_wav->Status() && decoder.GetNumChannels() && decoder.GetSampleRate())
            {
//                  printf("opening new wav\n");
              if (!concatout_wav->Open(concat_fn.c_str(),g_write_wav_bits, decoder.GetNumChannels(), decoder.GetSampleRate(),0))
              {
                printf("Warning: error opening %s to write WAV\n",concat_fn.c_str());
                break;
              }
            }

            if (concatout_wav->Status())
            {
              concatout_wav->WriteFloats((float *)decoder.m_samples.Get(),decoder.m_samples_used);
            }
            did_write += decoder.m_samples_used;

            decoder.m_samples_used=0;
          }

          if (!l) break;
        }

        last_len += did_write * 1000.0 / (double)concatout_wav->get_srate() / (double)concatout_wav->get_nch();
        if (!did_write)
        {
          printf("Warning: error decoding %s to convert to WAV\n", op.c_str());
        }

        fclose(fp);
      }
    } */
  }
  if (concatout)
  {
    if (concatout) fclose(concatout);
    concatout=0;
  }
  if (y) (*track_id)++;
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
    fgets(buf, sizeof(buf), logfile);
    if (!buf[0]) break;
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

      int idx = 0;
      double bpm = 0.0;
      int bpi = 0;
      idx = atoi(fields[1].c_str());
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
      int chidx = atoi(fields[3].c_str());

      UserChannelValueRec *ucvr = new UserChannelValueRec;
      ucvr->guidstr = fields[1];
      ucvr->position = cur_position;
      ucvr->length = cur_lenblock;

      int x;
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
  int x;
  for (x= 0; x < (int)(sizeof(localrecs)/sizeof(localrecs[0])); x ++)
  {
    char chname[512];
    sprintf(chname,"local_%02d",x);
    WriteOutTrack(chname,localrecs+x, &track_id, argv[1]);

  }
  for (x= 0; x < curintrecs.size(); x ++)
  {
    char chname[4096];
    sprintf(chname, "%s_%02d", curintrecs[x]->user.c_str(), x);
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
