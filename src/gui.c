/*
 * xiffview by r. eberle
 *
 * distributed under the terms of
 * AWeb Public License Version 1.0
 * https://www.yvonrozijn.nl/aweb/apl.txt
*/

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

#include <stdio.h>
#include <stdlib.h>

#ifdef DEBUG
#include "../../../lib-c/debug.h"
#else
#include "debug_none.h"
#endif
#include "usermsg.h"
#include "app.h"
#include "types.h"
#include "ximage.h"
#include "rgb.h"
#include "gui.h"

#define GUI_IMG_COORDS_STR_DEFAULT    "(move mouse over image)"

// gui metrics:
int		 gui_border = 20;              // space between gui elements
int      gui_colwidth = 150;           // width of additional layout columns (next to image display)
int      gui_font_height = 20;         // TODO: read font metrics from system
int      gui_font_height_baseline = 3; // TODO: read font metrics from system
int      gui_font_cwidth = 10;         // TODO: read font metrics from system
int      gui_palette_cell_size = 8;
int      gui_palette_rows =      4;
int      gui_palette_cols =      32;
int      gui_palette_height; // calculated
int      gui_ximage_empty_w = 320;
int      gui_ximage_empty_h = 256;
int		 gui_gadget_button_width = 10;
int		 gui_gadget_height = 10;

extern struct appdata *appdata;

Display  *gui_display;
Visual   *gui_visual;
int      gui_screen;
Window   gui_window;
Atom     gui_WMDeleteMessage;
Colormap gui_colormap;
unsigned long gui_pixel_black, gui_pixel_white;
GC       gui_gc;
XImage   *gui_ximage;

#define GUI_IMAGE_SCALE_MAX   3

char           gui_status_str[1024];
char           gui_img_descr_str[1024];
char           gui_img_coords_str[32];
struct BitMap *gui_img_bitmap;
UWORD         *gui_img_colortable_rgb4;
int            gui_img_colortable_length;

#define GUI_PALETTE_HANDLER_MODE_DRAW       0
#define GUI_PALETTE_HANDLER_MODE_MOUSECHECK 1

char tmpstr[1024];

struct gui_coords {
	int x, y, w, h;
};

struct {
	struct gui_coords image;
	struct gui_coords coords;
	struct gui_coords info;
	struct gui_coords palette;
	struct gui_coords gadgets;
	struct gui_coords status;
	int window_width, window_height; // total width and height of layout (for window inner size)
} gui_geometry;

Display *gui_getDisplay() {
	return gui_display;
}
Visual *gui_getVisual() {
	return gui_visual;
}
int gui_getScreen() {
	return gui_screen;
}

void gui_setimage(char *descr, struct BitMap *bm, UWORD *colortable_rgb4, int numcolors) {
	debugmsg("gui_setimage()");
	struct ximage_data xid;

	if (gui_ximage) {
		debugmsg("gui_setimage() - gui_ximage exists, dropping");
		ximage_free(gui_ximage);
	}

	xid.scale           = appdata->gui_image_scale;
	xid.bitplane_enable = appdata->gui_image_bitplane_enable;
	gui_ximage = ximage_create(bm, colortable_rgb4, &xid,  gui_display, gui_visual, gui_screen);
	debugmsg("gui_setimage() - ximage w, h: %d, %d", gui_ximage->width, gui_ximage->height);

	sprintf(gui_img_descr_str, "%s", descr);
	gui_img_bitmap = bm;
	gui_img_colortable_rgb4 = colortable_rgb4;
	gui_img_colortable_length = numcolors;
}

void gui_drawimage() {
	debugmsg("gui_drawimage()");
	char r, g, b;
	int x, y, w, h, len;
	char empty = 0;

	if (!gui_ximage) {
		empty = 1;
		usererr("no gui_ximage - using empty", NULL);
	}

	x = gui_geometry.image.x;
	y = gui_geometry.image.y;
	w = gui_geometry.image.w;
	h = gui_geometry.image.h;

	//XSetWindowBackgroundPixmap(gui_display, gui_window, gui_image_pixmap);

	if (empty) {
		XSetForeground(gui_display, gui_gc, gui_pixel_black);
		XDrawRectangle(gui_display, gui_window, gui_gc, x, y, w, h);
		sprintf(tmpstr, "error, no ximage");
		len = strlen(tmpstr);
		x += gui_border;
		y += gui_border + gui_font_height - gui_font_height_baseline;
		XDrawString(gui_display, gui_window, gui_gc, x, y, tmpstr, len);
	} else {
		//debugmsg("gui_drawimage() - putting image...");
		XPutImage(gui_display, gui_window, gui_gc, gui_ximage, 0, 0, x, y, w, h);
	}
}

void gui_drawinfo() {
	//debugmsg("gui_drawinfo()");
	int i, len, breakout;
	char str[64];
	int x = gui_geometry.info.x;
	int y = gui_geometry.info.y;
	debugmsg("gui_drawinfo() - x, y: %d, %d", x, y);

	XSetForeground(gui_display, gui_gc, gui_pixel_black);

	if (appdata->gui_image_scale == 1) {
		sprintf(str, "original size");
	} else {
		sprintf(str, "scaled x%d", appdata->gui_image_scale);
	}
	sprintf(tmpstr, 
		"%s (%d colors, size: %d x %d) (%s)",
		gui_img_descr_str, gui_img_colortable_length, gui_img_bitmap->BytesPerRow*8, gui_img_bitmap->Rows,
		str
	);
	len = strlen(tmpstr);
	XDrawString(gui_display, gui_window, gui_gc, x, y + gui_font_height - gui_font_height_baseline, tmpstr, len);
}

//==============================================================
// color palette
//==============================================================

void gui_palette_mkcell(int colornr, int x, int y) {
	debugmsg("gui_palette_mkcell(colornr = %d, x = %d, y = %d)", colornr, x, y);
	char r, g, b;
	XColor xcolor;

	rgb4torgb8(gui_img_colortable_rgb4[colornr], &r, &g, &b);
	xcolor.red = r << 8;
	xcolor.green = g << 8;
	xcolor.blue = b << 8;
	XAllocColor(gui_display, gui_colormap, &xcolor);
	XSetForeground(gui_display, gui_gc, xcolor.pixel);
	XFillRectangle(gui_display, gui_window, gui_gc, x, y, gui_palette_cell_size, gui_palette_cell_size);
}

void gui_palette_handler(int mode, int mousex, int mousey, int *result) {
	debugmsg("gui_palette_handler(mode = %d (%s))", mode, (mode == 0 ? "draw" : "check"));

	int row, col;
	int colornr = 0;
	int breakout = 0;
	int xoff = gui_geometry.palette.x;
	int yoff = gui_geometry.palette.y;
	if (result) *result = -1;

	int x;
	int y = yoff;
	for (row = 0; row < gui_palette_rows; row++) {
		x = xoff;
		for (col = 0; col < gui_palette_cols; col++) {

			if (mode == GUI_PALETTE_HANDLER_MODE_DRAW) {
				gui_palette_mkcell(colornr, x, y);
			} else {
				if (!(mousex < x || mousex > x + gui_palette_cell_size || 
				      mousey < y || mousey > y + gui_palette_cell_size)) {
					*result = colornr;
					return;
				}
			}

			x += gui_palette_cell_size;
			colornr++;
			if (colornr == gui_img_colortable_length) {
				breakout = 1;
				break;
			}
		}
		y += gui_palette_cell_size;
		if (breakout) {
			break;
		}
	}
}

void gui_drawpalette() {
	gui_palette_handler(GUI_PALETTE_HANDLER_MODE_DRAW, 0, 0, NULL);
}


void gui_drawcoords() {
	int len;
	int x = gui_geometry.coords.x;
	int y = gui_geometry.coords.y;
	int w = gui_geometry.coords.w;
	int h = gui_geometry.coords.h;
	int pad = 5;
	XSetForeground(gui_display, gui_gc, gui_pixel_white);
	XFillRectangle(gui_display, gui_window, gui_gc, x, y, w, h);
	XSetForeground(gui_display, gui_gc, gui_pixel_black);

	sprintf(tmpstr, "Coords");
	len = strlen(tmpstr);
	XDrawString(gui_display, gui_window, gui_gc, x, y + gui_font_height - gui_font_height_baseline, tmpstr, len);
	y += gui_font_height;
	len = strlen(gui_img_coords_str);
	XDrawString(gui_display, gui_window, gui_gc, x, y + gui_font_height - gui_font_height_baseline, gui_img_coords_str, len);
}

//==============================================================
// gadgets
//==============================================================

void gui_drawgadgets() {
	int i, len;
	int x = gui_geometry.gadgets.x;
	int y = gui_geometry.gadgets.y;

	int w = gui_gadget_button_width;
	int h = gui_gadget_height;
	int pad = 5;

	XSetForeground(gui_display, gui_gc, gui_pixel_black);

	// bitplane selection
	sprintf(tmpstr, "Bitplanes");
	len = strlen(tmpstr);
	XDrawString(gui_display, gui_window, gui_gc, x, y + gui_font_height - gui_font_height_baseline, tmpstr, len);
	y += gui_font_height + pad;
	for (i = 0; i < gui_img_bitmap->Depth; i++) {
		XDrawRectangle(gui_display, gui_window, gui_gc, x, y, w, h);
		if (appdata->gui_image_bitplane_enable[i]) {
			XFillRectangle(gui_display, gui_window, gui_gc, x + 2, y + 2, w - 3, h - 3);
		}
		sprintf(tmpstr, "%d (key: F%d)", i, i+1);
		len = strlen(tmpstr);
		XDrawString(gui_display, gui_window, gui_gc, x + w + 5, y + h, tmpstr, len);
		y += h + pad;
	}
	sprintf(tmpstr, "(hit F9 to enable all)");
	len = strlen(tmpstr);
	XDrawString(gui_display, gui_window, gui_gc, x, y + gui_font_height - gui_font_height_baseline, tmpstr, len);
}

void gui_drawstatus() {
	int len;
	int x = gui_geometry.status.x;
	int y = gui_geometry.status.y;
	int w = gui_geometry.status.w;
	int h = gui_geometry.status.h;
	XSetForeground(gui_display, gui_gc, gui_pixel_white);
	XFillRectangle(gui_display, gui_window, gui_gc, x, y, w, h);
	XSetForeground(gui_display, gui_gc, gui_pixel_black);
	XDrawRectangle(gui_display, gui_window, gui_gc, x, y, w, h);
	len = strlen(gui_status_str);
	XDrawString(gui_display, gui_window, gui_gc, x + 3, y + 2 + gui_font_height - gui_font_height_baseline, gui_status_str, len);
}

void gui_status(char *str) {
	sprintf(gui_status_str, "%s", str);
	gui_drawstatus();
}


void gui_mkgeometry() {
	debugmsg("gui_mkgeometry()");
	int y_max;

	gui_geometry.image.x = gui_border;
	gui_geometry.image.y = gui_border;
	if (gui_ximage) {
		gui_geometry.image.w = gui_ximage->width;
		gui_geometry.image.h = gui_ximage->height;
	} else {
		gui_geometry.image.w = gui_ximage_empty_w;
		gui_geometry.image.h = gui_ximage_empty_h;
	}

	gui_geometry.info.x = gui_border;
	gui_geometry.info.y = gui_geometry.image.y + gui_geometry.image.h + gui_border;
	gui_geometry.info.h = gui_font_height;

	gui_geometry.palette.x = gui_border;
	gui_geometry.palette.y = gui_geometry.info.y + gui_geometry.info.h + gui_border;
	gui_geometry.palette.w = gui_palette_cell_size * gui_palette_cols;
	gui_geometry.palette.h = gui_palette_cell_size * gui_palette_rows;

	y_max = gui_geometry.palette.y + gui_geometry.palette.h + gui_border;

	gui_geometry.coords.x = gui_geometry.image.x + gui_geometry.image.w + gui_border;
	gui_geometry.coords.y = gui_border;
	// we use default string to determine width (because default string is probably longer than coords)
	gui_geometry.coords.w = strlen(GUI_IMG_COORDS_STR_DEFAULT) * gui_font_cwidth;
	gui_geometry.coords.h = gui_font_height * 2;

	gui_geometry.gadgets.x = gui_geometry.coords.x;
	gui_geometry.gadgets.y = gui_geometry.coords.y + gui_geometry.coords.h + gui_border;

	gui_geometry.status.x = 0;
	gui_geometry.status.y = y_max;
	gui_geometry.status.w = gui_geometry.gadgets.x + gui_colwidth;
	gui_geometry.status.h = gui_font_height + 4; // 2 pixels above and below

	gui_geometry.window_width = gui_geometry.status.w + 1;
	gui_geometry.window_height = y_max + gui_geometry.status.h + 1;
}

void gui_redraw() {
	XClearWindow(gui_display, gui_window);
	gui_drawimage();
	gui_drawcoords();
	gui_drawinfo();
	gui_drawpalette();
	gui_drawgadgets();
	gui_drawstatus();
};

void gui_subwin_help() {
	gui_status("help: key s changes image scaling. hit q or ESC to quit.");
}

// buttonnr 0 = left mouse button
int gui_handlemouseclick(int buttonnr, int mousex, int mousey) {
	return 0;
}

// returns 0 if keyboard input requests exit,
// otherwise 1
int gui_handlekeyboard(XKeyEvent *xkeyevent) {
	int i;
	int ret = 1;
	int bitplanenr = -1;
	char str[2];
	char c = 0;
	char redraw = 0;

	KeySym key = XLookupKeysym(xkeyevent, 0);	
	char text[255]; // char buffer for KeyPress Events

	if (IsFunctionKey(key)) {
		//debugmsg("fkey!");
		switch (key) {
			case XK_F1:
				bitplanenr = 0;
			break;
			case XK_F2:
				bitplanenr = 1;
			break;
			case XK_F3:
				bitplanenr = 2;
			break;
			case XK_F4:
				bitplanenr = 3;
			break;
			case XK_F5:
				bitplanenr = 4;
			break;
			case XK_F6:
				bitplanenr = 5;
			break;
			case XK_F7:
				bitplanenr = 6;
			break;
			case XK_F8:
				bitplanenr = 7;
			break;
			case XK_F9:
				for (i = 0; i < gui_img_bitmap->Depth; i++) {
					appdata->gui_image_bitplane_enable[i] = 1;
				}
				redraw = 1;
			break;
		}
		if (bitplanenr != -1 && bitplanenr < gui_img_bitmap->Depth) {
			appdata->gui_image_bitplane_enable[bitplanenr] ^= 1;
			redraw = 1;
		}
		if (redraw) {
			// re-generate image...
			gui_setimage(gui_img_descr_str, gui_img_bitmap, gui_img_colortable_rgb4, gui_img_colortable_length);
			// ...and redraw window:
			gui_redraw();
		}
	} else {
		if (XLookupString(xkeyevent, text, 255, &key, 0) == 1) {
			//printf("keypress: %d\n", text[0]);
			c = text[0];
		}
	}

	switch (c) {
		case 27: // ESC
		case 'q':
			ret = 0;
		break;
		case 'h':
			gui_subwin_help();
		break;
		case 's':
			appdata->gui_image_scale++;
			if (appdata->gui_image_scale > GUI_IMAGE_SCALE_MAX) {
				appdata->gui_image_scale = 1;
			}
			gui_close();
			// re-create gui_ximage with new scaling:
			gui_setimage(gui_img_descr_str, gui_img_bitmap, gui_img_colortable_rgb4, gui_img_colortable_length);
			gui_open();
		break;
	}
	return ret;
}

void gui_mainloop() {
	XEvent event;
	UWORD col_rgb4;
	int x, y, colornr, coordx, coordy;
	unsigned char r, g, b;
	while(1) {		
		XNextEvent(gui_display, &event);

		if (event.type == ClientMessage && event.xclient.data.l[0] == gui_WMDeleteMessage) {
			return;
		}
	
		if (event.type == Expose && event.xexpose.count == 0) {
			gui_redraw();
		}
		if (event.type == KeyPress) {
			if (!gui_handlekeyboard(&event.xkey)) {
				return;
			}
		}
		// Thu Oct 14 09:40:42 PM CEST 2021
		// hm, this currently does nothing...? (see at XSelectInput(), too)
		//if (event.type == DestroyNotify) {
			//debugmsg("gui_mainloop() - DestroyNotify");
			//return;
		//}
		if (event.type == ButtonPress) {
			x = event.xbutton.x,
			y = event.xbutton.y;
			if (!gui_handlemouseclick(0, x, y)) { // 0 = left button
				return;
			}
		}
		if (event.type == MotionNotify) {
			x = event.xmotion.x,
			y = event.xmotion.y;

			// check image / pixels mouseover:
			if (!(x < gui_geometry.image.x || x > gui_geometry.image.x + gui_geometry.image.w ||
			      y < gui_geometry.image.y || y > gui_geometry.image.y + gui_geometry.image.h)) {
				  coordx = (x - gui_geometry.image.x) / appdata->gui_image_scale;
				  coordy = (y - gui_geometry.image.y) / appdata->gui_image_scale;
				  sprintf(gui_img_coords_str, "%d, %d", coordx, coordy);
				  gui_drawcoords();
			} else {
				if (strcmp(gui_img_coords_str, GUI_IMG_COORDS_STR_DEFAULT)) {
					sprintf(gui_img_coords_str, GUI_IMG_COORDS_STR_DEFAULT);
				  	gui_drawcoords();
				}
			}

			// check color palette cells mouseover:
			gui_palette_handler(GUI_PALETTE_HANDLER_MODE_MOUSECHECK, x, y, &colornr);
			if (colornr != -1) {
				col_rgb4 = gui_img_colortable_rgb4[colornr];
				rgb4torgb8(col_rgb4, &r, &g, &b);
				sprintf(tmpstr, "Color: %d  RGB8: %d,%d,%d  RGB4: %d,%d,%d (0x%04x)", 
				        colornr, r, g, b, r >> 4, g >> 4, b >> 4, col_rgb4);
				gui_status(tmpstr);
			}

		}
	}
}

int gui_new() {
	int ret = 1;
	gui_ximage = NULL;
	sprintf(gui_status_str, "welcome to xiffview. hit h for help, q or ESC to quit.");
	sprintf(gui_img_coords_str, GUI_IMG_COORDS_STR_DEFAULT);
	gui_display = XOpenDisplay((char *)0);
	if (gui_display) {
		XSynchronize(gui_display, 1);
		gui_visual = XDefaultVisual(gui_display, 0);
		gui_screen  = XDefaultScreen(gui_display);
		gui_colormap = DefaultColormap(gui_display, gui_screen);
		gui_pixel_black = BlackPixel(gui_display, gui_screen),
		gui_pixel_white = WhitePixel(gui_display, gui_screen);
		ret = 1;
	} else {
		usererr("could not open x display", NULL);
	}
	return ret;
}

int gui_open() {
	debugmsg("gui_open()");
	int w, h;

	gui_palette_height = gui_palette_cell_size * gui_palette_rows;

	gui_mkgeometry();
	w = gui_geometry.window_width;
	h = gui_geometry.window_height;

   	gui_window = XCreateSimpleWindow(gui_display, DefaultRootWindow(gui_display), 0, 0,	
		w, h, 5, gui_pixel_black, gui_pixel_white);
	XSetStandardProperties(gui_display, gui_window, APP_STRING, APP_STRING_SHORT, None, NULL, 0, NULL);

	gui_WMDeleteMessage = XInternAtom(gui_display, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(gui_display, gui_window, &gui_WMDeleteMessage, 1);

	// Thu Oct 14 09:40:42 PM CEST 2021
	// hm, this currently does nothing...? (see at event.type == DestroyNotify, too)
	//XSelectInput(gui_display, gui_window, ExposureMask|ButtonPressMask|KeyPressMask|PointerMotionMask|StructureNotifyMask);
	XSelectInput(gui_display, gui_window, ExposureMask|ButtonPressMask|KeyPressMask|PointerMotionMask);
    gui_gc = XCreateGC(gui_display, gui_window, 0,0);        
	XSetBackground(gui_display, gui_gc, gui_pixel_white);
	XSetForeground(gui_display, gui_gc, gui_pixel_black);
	XClearWindow(gui_display, gui_window);
	XMapRaised(gui_display, gui_window);

	gui_redraw();
	return 1;
}

void gui_close() {
	debugmsg("gui_close()");
	if (gui_ximage) ximage_free(gui_ximage);
	gui_ximage = NULL;
	XDestroyWindow(gui_display, gui_window);
};

void gui_delete() {
	XFreeGC(gui_display, gui_gc);
	XCloseDisplay(gui_display);	
}
