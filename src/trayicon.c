/*****************************************************************************
 *
 * Authors: Michel Eyckmans (MCE) & Stefan De Troch (SDT)
 *
 * Content: This file is part of version 2.x of xautolock. It implements
 *          system tray icon support for visualizing when a nolock corner
 *          is engaged.
 *
 *          Please send bug reports etc. to mce@scarlet.be.
 *
 * --------------------------------------------------------------------------
 *
 * Copyright 1990, 1992-1999, 2001-2002, 2004, 2007 by  Stefan De Troch and
 * Michel Eyckmans.
 *
 * Versions 2.0 and above of xautolock are available under version 2 of the
 * GNU GPL. Earlier versions are available under other conditions. For more
 * information, see the License file.
 *
 *****************************************************************************/

#include "trayicon.h"
#include "miscutil.h"

/*
 *  System tray protocol atoms and window.
 */
static Atom traySelectionAtom = 0;
static Atom trayOpcodeAtom = 0;
static Window trayWindow = 0;
static Window iconWindow = 0;
static Bool iconVisible = False;
static Display* iconDisplay = 0;

/*
 *  Icon size (standard for system tray).
 */
#define ICON_SIZE 24

/*
 *  XPM data for lock-with-slash icon (24x24).
 */
static const char* lockSlashXpm[] = {
  "24 24 4 1",
  "  c None",
  ". c #000000",
  "+ c #FF0000",
  "@ c #FFFFFF",
  "                        ",
  "                        ",
  "        ........        ",
  "       ..      ..       ",
  "      ..        ..      ",
  "     ..          ..     ",
  "     .            .     ",
  "    ..   ++++++   ..    ",
  "   ....++......++....   ",
  "  ...++..........++...  ",
  "  ..++..@@@@@@@@..++..  ",
  "  .++...@@....@@...++.  ",
  "  .+....@@....@@....+.  ",
  "  .+....@@....@@....+.  ",
  "  ..++..@@....@@..++..  ",
  "  ...++..@@@@@@..++...  ",
  "   ....++......++....   ",
  "    ....++++++....      ",
  "      ........          ",
  "                        ",
  "                        ",
  "                        ",
  "                        ",
  "                        "
};

/*
 *  Parse simple XPM data and create a pixmap.
 */
static Pixmap
createIconPixmap (Display* d, Window w)
{
  int width = ICON_SIZE;
  int height = ICON_SIZE;
  Pixmap pixmap;
  GC gc;
  XGCValues gcValues;
  int x, y;
  char pixel;
  unsigned long blackPixel = BlackPixel (d, DefaultScreen (d));
  unsigned long whitePixel = WhitePixel (d, DefaultScreen (d));
  unsigned long redPixel;
  XColor redColor;
  Colormap colormap = DefaultColormap (d, DefaultScreen (d));

 /*
  *  Try to allocate red color, fall back to white if it fails.
  */
  if (XAllocNamedColor (d, colormap, "red", &redColor, &redColor))
  {
    redPixel = redColor.pixel;
  }
  else
  {
    redPixel = whitePixel;
  }

 /*
  *  Create pixmap and GC.
  */
  pixmap = XCreatePixmap (d, w, width, height, 
                          DefaultDepth (d, DefaultScreen (d)));
  gc = XCreateGC (d, pixmap, 0, &gcValues);

 /*
  *  Parse XPM data (starting from line 5, skipping header).
  */
  for (y = 0; y < height && y + 5 < 29; y++)
  {
    const char* line = lockSlashXpm[y + 5];
    for (x = 0; x < width && x < 24; x++)
    {
      pixel = line[x];
      
      if (pixel == '.')
      {
        XSetForeground (d, gc, blackPixel);
      }
      else if (pixel == '+')
      {
        XSetForeground (d, gc, redPixel);
      }
      else if (pixel == '@')
      {
        XSetForeground (d, gc, whitePixel);
      }
      else
      {
       /*
        *  Transparent (space) - use a light gray background.
        */
        XSetForeground (d, gc, 0xC0C0C0);
      }
      
      XDrawPoint (d, pixmap, gc, x, y);
    }
  }

  XFreeGC (d, gc);
  return pixmap;
}

/*
 *  Initialize tray icon support.
 */
void 
initTrayIcon (Display* d)
{
  char traySelectionName[64];
  int screen = DefaultScreen (d);

  iconDisplay = d;

 /*
  *  Create atom names for system tray protocol.
  */
  (void) sprintf (traySelectionName, "_NET_SYSTEM_TRAY_S%d", screen);
  traySelectionAtom = XInternAtom (d, traySelectionName, False);
  trayOpcodeAtom = XInternAtom (d, "_NET_SYSTEM_TRAY_OPCODE", False);

 /*
  *  Find the system tray window.
  */
  trayWindow = XGetSelectionOwner (d, traySelectionAtom);

  if (!trayWindow)
  {
   /*
    *  No system tray available. That's okay, just disable the feature.
    */
    return;
  }

 /*
  *  Create the icon window (initially unmapped).
  */
  iconWindow = XCreateSimpleWindow (d, DefaultRootWindow (d),
                                     0, 0, ICON_SIZE, ICON_SIZE, 0, 0, 0);

 /*
  *  Set window properties for the tray.
  */
  XSelectInput (d, iconWindow, ExposureMask | StructureNotifyMask);
}

/*
 *  Show the tray icon.
 */
void 
showTrayIcon (Display* d)
{
  XEvent ev;
  Pixmap iconPixmap;

  if (!trayWindow || !iconWindow || iconVisible)
  {
    return;
  }

 /*
  *  Send SYSTEM_TRAY_REQUEST_DOCK message to tray.
  */
  (void) memset (&ev, 0, sizeof (ev));
  ev.xclient.type = ClientMessage;
  ev.xclient.window = trayWindow;
  ev.xclient.message_type = trayOpcodeAtom;
  ev.xclient.format = 32;
  ev.xclient.data.l[0] = CurrentTime;
  ev.xclient.data.l[1] = 0; /* SYSTEM_TRAY_REQUEST_DOCK */
  ev.xclient.data.l[2] = iconWindow;
  ev.xclient.data.l[3] = 0;
  ev.xclient.data.l[4] = 0;

  XSendEvent (d, trayWindow, False, NoEventMask, &ev);

 /*
  *  Create and set the icon pixmap.
  */
  iconPixmap = createIconPixmap (d, iconWindow);
  XSetWindowBackgroundPixmap (d, iconWindow, iconPixmap);
  XFreePixmap (d, iconPixmap);

  XMapWindow (d, iconWindow);
  XFlush (d);

  iconVisible = True;
}

/*
 *  Hide the tray icon.
 */
void 
hideTrayIcon (Display* d)
{
  if (!trayWindow || !iconWindow || !iconVisible)
  {
    return;
  }

  XUnmapWindow (d, iconWindow);
  XFlush (d);

  iconVisible = False;
}
