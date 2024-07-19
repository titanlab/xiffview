/*
 * xiffview by r. eberle
 *
 * distributed under the terms of
 * AWeb Public License Version 1.0
 * https://www.yvonrozijn.nl/aweb/apl.txt
*/

#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

#ifdef DEBUG
#include "../../../lib-c/debug.h"
#else
#include "debug_none.h"
#endif
#include "types.h"
#include "usermsg.h"
#include "rgb.h"
#include "ximage.h"

XImage *ximage_create(struct BitMap *bm, UWORD *ctable_rgb4, struct ximage_data *xid, Display *display, Visual *visual, int screen) {
	debugmsg("ximage_create() - bm->Depth: %d", bm->Depth);
	int i, j, k, l, scale_x, scale_y;
	char *pix, cindex, *r, *g, *b;
	unsigned long idat_size;
	XImage *ximg = NULL;

	int scale            = xid->scale;
	char *bitplane_enable = xid->bitplane_enable; // array (0 or 1 / true or false)

	int err = 0;
	int psize = bm->BytesPerRow * bm->Rows;
	int bytes_per_line = bm->BytesPerRow;
	int w              = bm->BytesPerRow*8;
	int h              = bm->Rows;
	int depth          = bm->Depth;
	// 4 = BGRA (8 bits per channel), and apply pixel scaling
	idat_size = w * h * 4 * (scale * scale); 

	char *idat = malloc(idat_size); // will be freed by XDestroyImage
	if (idat) {
		//debugmsg("ximage_create() - got idat");

		pix = idat;
		for (i = 0; i < h; i++) {
			scale_y = scale;

			while (scale_y) {
				for (j = 0; j < bytes_per_line; j++) {
					for (k = 0; k < 8; k++) {
						cindex = 0;
						for (l = 0; l < depth; l++) {
							char *plane = (char *) bm->Planes[l];
							char bit_set = plane[i*bytes_per_line+j] & (1 << (7-k));
							if (!bitplane_enable[l]) {
								bit_set = 0;
							}
							cindex = cindex | (bit_set ? (1 << l) : 0);
						}
						//if (cindex > 15) {
							//usererr("cindex > 15", NULL);
						//}
						//if (printit) 
							//printf("%d:%d\t", j*8+k, cindex);

						scale_x = scale;
						while (scale_x) {
							b = pix;
							pix++;
							g = pix;
							pix++;
							r = pix;
							pix++;
							pix++;
							rgb4torgb8(ctable_rgb4[cindex], r, g, b);
							scale_x--;
						}
					}
				}
				scale_y--;
			}

		}
		//for (i = 0; i < 15; i++) {
			//printf("ctable_rgb4[%d]: 0x%04x\n", i, ctable_rgb4[i]);
		//}
    	ximg = XCreateImage(display, visual, DefaultDepth(display, screen), ZPixmap, 0, idat, w * scale, h * scale, 32, 0);
		if (ximg) {
			debugmsg("ximage_create() - image created");
		} else {
			usererr("ximage_create() - image NOT created, freeing image data memory!", NULL);
			free(idat);
		}
	} else {
		err = 1;
		usererr("could not allocate imagedata", NULL);
	}
	return ximg;
}

void ximage_free(XImage *ximg) {
	debugmsg("ximage_free()");
	// also frees imagedata (according to the docs)
	if (ximg) XDestroyImage(ximg);
}
