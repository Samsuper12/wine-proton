/*
 * DirectDraw XVidMode interface
 *
 * Copyright 2001 TransGaming Technologies, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* FIXME: ChangeDisplaySettings ought to be able to use this */

#include "config.h"
#include <string.h>

#include "ts_xlib.h"

#ifdef HAVE_LIBXXF86VM
#define XMD_H
#include "basetsd.h"
#include <X11/extensions/xf86vmode.h>
#endif  /* HAVE_LIBXXF86VM */
#include "x11drv.h"

#include "x11ddraw.h"
#include "xvidmode.h"

#include "windef.h"
#include "wingdi.h"
#include "ddrawi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(x11drv);

#ifdef HAVE_LIBXXF86VM

extern int usexvidmode;

static int xf86vm_event, xf86vm_error, xf86vm_major, xf86vm_minor;

#ifdef X_XF86VidModeSetGammaRamp
static int xf86vm_gammaramp_size;
static BOOL xf86vm_use_gammaramp;
#endif

LPDDHALMODEINFO xf86vm_modes;
unsigned xf86vm_mode_count;
XF86VidModeModeInfo** modes;

static void convert_modeinfo( const XF86VidModeModeInfo *mode, LPDDHALMODEINFO info )
{
  info->dwWidth      = mode->hdisplay;
  info->dwHeight     = mode->vdisplay;
  if (mode->htotal!=0 && mode->vtotal!=0)
      info->wRefreshRate = mode->dotclock * 1000 / (mode->htotal * mode->vtotal);
  else
      info->wRefreshRate = 0;
  TRACE(" width=%ld, height=%ld, refresh=%d\n",
        info->dwWidth, info->dwHeight, info->wRefreshRate);
  /* XVidMode cannot change display depths... */
  /* let's not bother with filling out these then... */
  info->lPitch         = 0;
  info->dwBPP          = 0;
  info->wFlags         = 0;
  info->dwRBitMask     = 0;
  info->dwGBitMask     = 0;
  info->dwBBitMask     = 0;
  info->dwAlphaBitMask = 0;
}

static void convert_modeline(int dotclock, const XF86VidModeModeLine *mode, LPDDHALMODEINFO info)
{
  info->dwWidth      = mode->hdisplay;
  info->dwHeight     = mode->vdisplay;
  if (mode->htotal!=0 && mode->vtotal!=0)
      info->wRefreshRate = dotclock * 1000 / (mode->htotal * mode->vtotal);
  else
      info->wRefreshRate = 0;
  TRACE(" width=%ld, height=%ld, refresh=%d\n",
        info->dwWidth, info->dwHeight, info->wRefreshRate);
  /* XVidMode cannot change display depths... */
  /* let's not bother with filling out these then... */
  info->lPitch         = 0;
  info->dwBPP          = 0;
  info->wFlags         = 0;
  info->dwRBitMask     = 0;
  info->dwGBitMask     = 0;
  info->dwBBitMask     = 0;
  info->dwAlphaBitMask = 0;
}

static int XVidModeErrorHandler(Display *dpy, XErrorEvent *event, void *arg)
{
    return 1;
}

void X11DRV_XF86VM_Init(void)
{
  int nmodes, i;
  Bool ok;

  if (xf86vm_major) return; /* already initialized? */

  /* if in desktop mode, don't use XVidMode */
  if (root_window != DefaultRootWindow(gdi_display)) return;

  if (!usexvidmode) return;

  /* see if XVidMode is available */
  wine_tsx11_lock();
  ok = XF86VidModeQueryExtension(gdi_display, &xf86vm_event, &xf86vm_error);
  if (ok)
  {
      X11DRV_expect_error(gdi_display, XVidModeErrorHandler, NULL);
      ok = XF86VidModeQueryVersion(gdi_display, &xf86vm_major, &xf86vm_minor);
      if (X11DRV_check_error()) ok = FALSE;
  }
  if (ok)
  {
#ifdef X_XF86VidModeSetGammaRamp
      if (xf86vm_major > 2 || (xf86vm_major == 2 && xf86vm_minor >= 1))
      {
          XF86VidModeGetGammaRampSize(gdi_display, DefaultScreen(gdi_display),
                                      &xf86vm_gammaramp_size);
          if (xf86vm_gammaramp_size == 256)
              xf86vm_use_gammaramp = TRUE;
      }
#endif

      /* retrieve modes */
      ok = XF86VidModeGetAllModeLines(gdi_display, DefaultScreen(gdi_display), &nmodes, &modes);
  }
  wine_tsx11_unlock();
  if (!ok) return;

  TRACE("XVidMode modes: count=%d\n", nmodes);

  xf86vm_mode_count = nmodes;
  xf86vm_modes = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DDHALMODEINFO) * nmodes);

  /* convert modes to DDHALMODEINFO format */
  for (i=0; i<nmodes; i++)
      convert_modeinfo(modes[i], &xf86vm_modes[i]);
  TRACE("Enabling XVidMode\n");
}

void X11DRV_XF86VM_Cleanup(void)
{
  if (modes) TSXFree(modes);
}

int X11DRV_XF86VM_GetCurrentMode(void)
{
  XF86VidModeModeLine line;
  int dotclock, i;
  DDHALMODEINFO cmode;

  if (!xf86vm_modes) return 0; /* no XVidMode */

  TRACE("Querying XVidMode current mode\n");
  wine_tsx11_lock();
  XF86VidModeGetModeLine(gdi_display, DefaultScreen(gdi_display), &dotclock, &line);
  wine_tsx11_unlock();
  convert_modeline(dotclock, &line, &cmode);
  for (i=0; i<xf86vm_mode_count; i++)
    if (memcmp(&xf86vm_modes[i], &cmode, sizeof(cmode)) == 0) {
      TRACE("mode=%d\n", i);
      return i;
    }
  ERR("unknown mode, shouldn't happen\n");
  return 0; /* return first mode */
}

void X11DRV_XF86VM_SetCurrentMode(int mode)
{
  if (!xf86vm_modes) return; /* no XVidMode */

  wine_tsx11_lock();
  XF86VidModeSwitchToMode(gdi_display, DefaultScreen(gdi_display), modes[mode]);
#if 0 /* it is said that SetViewPort causes problems with some X servers */
  XF86VidModeSetViewPort(gdi_display, DefaultScreen(gdi_display), 0, 0);
#else
  XWarpPointer(gdi_display, None, DefaultRootWindow(gdi_display), 0, 0, 0, 0, 0, 0);
#endif
  XSync(gdi_display, False);
  wine_tsx11_unlock();
}

void X11DRV_XF86VM_SetExclusiveMode(int lock)
{
  if (!xf86vm_modes) return; /* no XVidMode */

  wine_tsx11_lock();
  XF86VidModeLockModeSwitch(gdi_display, DefaultScreen(gdi_display), lock);
  wine_tsx11_unlock();
}

/* actual DirectDraw HAL stuff */

static DWORD PASCAL X11DRV_XF86VM_SetMode(LPDDHAL_SETMODEDATA data)
{
  X11DRV_XF86VM_SetCurrentMode(data->dwModeIndex);
  X11DRV_DDHAL_SwitchMode(data->dwModeIndex, NULL, NULL);
  data->ddRVal = DD_OK;
  return DDHAL_DRIVER_HANDLED;
}

int X11DRV_XF86VM_CreateDriver(LPDDHALINFO info)
{
  if (!xf86vm_mode_count) return 0; /* no XVidMode */

  info->dwNumModes = xf86vm_mode_count;
  info->lpModeInfo = xf86vm_modes;
  X11DRV_DDHAL_SwitchMode(X11DRV_XF86VM_GetCurrentMode(), NULL, NULL);
  info->lpDDCallbacks->SetMode = X11DRV_XF86VM_SetMode;
  return TRUE;
}


/***** GAMMA CONTROL *****/
/* (only available in XF86VidMode 2.x) */

#ifdef X_XF86VidModeSetGamma

static void GenerateRampFromGamma(WORD ramp[256], float gamma)
{
  float r_gamma = 1/gamma;
  unsigned i;
  TRACE("gamma is %f\n", r_gamma);
  for (i=0; i<256; i++)
    ramp[i] = pow(i/255.0, r_gamma) * 65535.0;
}

static BOOL ComputeGammaFromRamp(WORD ramp[256], float *gamma)
{
  float r_x, r_y, r_lx, r_ly, r_d, r_v, r_e, g_avg, g_min, g_max;
  unsigned i, f, l, g_n, c;
  f = ramp[0];
  l = ramp[255];
  if (f >= l) {
    ERR("inverted or flat gamma ramp (%d->%d), rejected\n", f, l);
    return FALSE;
  }
  r_d = l - f;
  g_min = g_max = g_avg = 0.0;
  /* check gamma ramp entries to estimate the gamma */
  TRACE("analyzing gamma ramp (%d->%d)\n", f, l);
  for (i=1, g_n=0; i<255; i++) {
    if (ramp[i] < f || ramp[i] > l) {
      ERR("strange gamma ramp ([%d]=%d for %d->%d), rejected\n", i, ramp[i], f, l);
      return FALSE;
    }
    c = ramp[i] - f;
    if (!c) continue; /* avoid log(0) */

    /* normalize entry values into 0..1 range */
    r_x = i/255.0; r_y = c / r_d;
    /* compute logarithms of values */
    r_lx = log(r_x); r_ly = log(r_y);
    /* compute gamma for this entry */
    r_v = r_ly / r_lx;
    /* compute differential (error estimate) for this entry */
    /* some games use table-based logarithms that magnifies the error by 128 */
    r_e = -r_lx * 128 / (c * r_lx * r_lx);

    /* compute min & max while compensating for estimated error */
    if (!g_n || g_min > (r_v + r_e)) g_min = r_v + r_e;
    if (!g_n || g_max < (r_v - r_e)) g_max = r_v - r_e;

    /* add to average */
    g_avg += r_v;
    g_n++;
    /* TRACE("[%d]=%d, gamma=%f, error=%f\n", i, ramp[i], r_v, r_e); */
  }
  if (!g_n) {
    ERR("no gamma data, shouldn't happen\n");
    return FALSE;
  }
  g_avg /= g_n;
  TRACE("low bias is %d, high is %d, gamma is %5.3f\n", f, 65535-l, g_avg);
  /* the bias could be because the app wanted something like a "red shift"
   * like when you're hit in Quake, but XVidMode doesn't support it,
   * so we have to reject a significant bias */
  if (f && f > (pow(1/255.0, g_avg) * 65536.0)) {
    ERR("low-biased gamma ramp (%d), rejected\n", f);
    return FALSE;
  }
  /* check that the gamma is reasonably uniform across the ramp */
  if (g_max - g_min > 0.1) {
    ERR("ramp not uniform (max=%f, min=%f, avg=%f), rejected\n", g_max, g_min, g_avg);
    return FALSE;
  }
  /* ok, now we're pretty sure we can set the desired gamma ramp,
   * so go for it */
  *gamma = 1/g_avg;
  return TRUE;
}

#endif /* X_XF86VidModeSetGamma */

/* Hmm... should gamma control be available in desktop mode or not?
 * I'll assume that it should */

BOOL X11DRV_XF86VM_GetGammaRamp(LPDDGAMMARAMP ramp)
{
#ifdef X_XF86VidModeSetGamma
  XF86VidModeGamma gamma;
  Bool ret;

  if (xf86vm_major < 2) return FALSE; /* no gamma control */
#ifdef X_XF86VidModeSetGammaRamp
  else if (xf86vm_use_gammaramp)
  {
      Bool ret;
      wine_tsx11_lock();
      ret = XF86VidModeGetGammaRamp(gdi_display, DefaultScreen(gdi_display), 256,
				    ramp->red, ramp->green, ramp->blue);
      wine_tsx11_unlock();
      return ret;
  }
#endif
  else
  {
      wine_tsx11_lock();
      ret = XF86VidModeGetGamma(gdi_display, DefaultScreen(gdi_display), &gamma);
      wine_tsx11_unlock();
      if (ret) {
	  GenerateRampFromGamma(ramp->red,   gamma.red);
	  GenerateRampFromGamma(ramp->green, gamma.green);
	  GenerateRampFromGamma(ramp->blue,  gamma.blue);
	  return TRUE;
      }
  }
#endif /* X_XF86VidModeSetGamma */
  return FALSE;
}

BOOL X11DRV_XF86VM_SetGammaRamp(LPDDGAMMARAMP ramp)
{
#ifdef X_XF86VidModeSetGamma
  XF86VidModeGamma gamma;

  if (xf86vm_major < 2) return FALSE; /* no gamma control */
#ifdef X_XF86VidModeSetGammaRamp
  else if (xf86vm_use_gammaramp)
  {
      Bool ret;
      wine_tsx11_lock();
      ret = XF86VidModeSetGammaRamp(gdi_display, DefaultScreen(gdi_display), 256,
				    ramp->red, ramp->green, ramp->blue);
      wine_tsx11_unlock();
      return ret;
  }
#endif
  else
  {
      if (ComputeGammaFromRamp(ramp->red,   &gamma.red) &&
	  ComputeGammaFromRamp(ramp->green, &gamma.green) &&
	  ComputeGammaFromRamp(ramp->blue,  &gamma.blue)) {
	  Bool ret;
	  wine_tsx11_lock();
	  ret = XF86VidModeSetGamma(gdi_display, DefaultScreen(gdi_display), &gamma);
	  wine_tsx11_unlock();
	  return ret;
      }
  }
#endif /* X_XF86VidModeSetGamma */
  return FALSE;
}

#endif /* HAVE_LIBXXF86VM */

/***********************************************************************
 *		GetDeviceGammaRamp (X11DRV.@)
 *
 * FIXME: should move to somewhere appropriate, but probably not before
 * the stuff in graphics/x11drv/ has been moved to dlls/x11drv, so that
 * they can include xvidmode.h directly
 */
BOOL X11DRV_GetDeviceGammaRamp(X11DRV_PDEVICE *physDev, LPVOID ramp)
{
#ifdef HAVE_LIBXXF86VM
  return X11DRV_XF86VM_GetGammaRamp(ramp);
#else
  return FALSE;
#endif
}

/***********************************************************************
 *		SetDeviceGammaRamp (X11DRV.@)
 *
 * FIXME: should move to somewhere appropriate, but probably not before
 * the stuff in graphics/x11drv/ has been moved to dlls/x11drv, so that
 * they can include xvidmode.h directly
 */
BOOL X11DRV_SetDeviceGammaRamp(X11DRV_PDEVICE *physDev, LPVOID ramp)
{
#ifdef HAVE_LIBXXF86VM
  return X11DRV_XF86VM_SetGammaRamp(ramp);
#else
  return FALSE;
#endif
}
