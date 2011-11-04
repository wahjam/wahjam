/*
    Jesusonic DLL interface - jesusoninc_dll.h
    Copyright (C) 2004-2005 Cockos Incorporated

*/


#ifndef _JESUSONIC_DLL_H_
#define _JESUSONIC_DLL_H_

#define JESUSONIC_API_VERSION_CURRENT 0xf0f01003

// to get the API, GetProcAddress("JesusonicAPI"), call with no parms

typedef struct
{
  int ver;
  void *(*createInstance)();
  void (*destroyInstance)(void *);
  void (*set_rootdir)(void *, char *);
  void (*set_status)(void *,char *b, char *bufstr);
  void (*set_sample_fmt)(void *,int srate, int nch, int bps);
  void (*ui_init)(void *);
  void (*ui_run)(void *,int forcerun);
  void (*ui_quit)(void *);
  void (*osc_run)(void *,char *buf, int nbytes); // run this from your main thread, with the splbuf
  void (*jesus_process_samples)(void *,char *splbuf, int nbytes);

  void (*on_sounderr)(void *, int under, int over);

  void (*set_opts)(void *, int noquit, int nowriteout, int done); // -1 to not change
  void (*get_opts)(void *, int *noquit, int *nowriteout, int *done); // NULL to not query

  HWND (*ui_wnd_create)(void *);
  HWND (*ui_wnd_gethwnd)(void *);
  void (*ui_wnd_destroy)(void *);

  void (*preset_load)(void *, char *fn);
  void (*preset_save)(void *, char *fn);

  void (*knob_tweak)(void *, int wk, double amt, int isset);
  void (*trigger)(void *, int trigmask);

  int pad[492];//for future expansion

} jesusonicAPI;

#endif
