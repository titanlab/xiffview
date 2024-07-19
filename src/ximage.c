/*
 * xiffview by r. eberle
 *
 * distributed under the terms of
 * AWeb Public License Version 1.0
 * https://www.yvonrozijn.nl/aweb/apl.txt
*/

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

#include "debug.h"
#include "types.h"
#include "usermsg.h"
#include "rgb.h"
#include "ximage.h"

XImage *ximage_create(struct BitMap *bm, ULONG *ctable, struct ximage_data *xid,
    Display *display, Visual *visual, int screen)
{
	int i, j, k, l;
    int scale_x, scale_y;
	char *pix, *r, *g, *b;
    int cindex;
	unsigned long idat_size;
	XImage *ximg = NULL;

	int scale             = xid->scale;
	char *bitplane_enable = xid->bitplane_enable;

	int h  = bm->Rows;
    int sx = 1;
    int sy = 1;
    int ys = 1;
    int yo = 0;

    // Not in productivity mode
    if(bm->Flags & NATIVE) {
        if(bm->Flags & LACE) {
            if(!xid->fix_flicker) {
                ys = 2;
                yo = xid->field;

                if(!(bm->Flags & HIRES)) {
                    h /= 2;
                } else {
                    sy = 2;
                }
            } else {
                if(!(bm->Flags & HIRES)) {
                    sx = 2;
                }
            }
        } else {
            if(bm->Flags & HIRES) {
                sy = 2;
                h *= 2;
            }
        }
    }

	int err            = 0;
	int bytes_per_line = bm->BytesPerRow;
	int w              = bm->BytesPerRow * 8 * sx;
	int depth          = bm->Depth;

    printf("w=%d h=%d sx=%d sy=%d ys=%d yo=%d bpl=%d\n", w, h, sx, sy, ys, yo, bytes_per_line);

	char *idat = malloc(w * h * 4 * scale * sx * scale * sy);
	if (idat) {
		pix = idat;

		for (i = yo; i < bm->Rows; i+=ys) {
			scale_y = scale * sy;

			while (scale_y) {
                ULONG rgb = ctable[0] & 0xfcfcfc;

				for (j = 0; j < bytes_per_line; j++) {
					for (k = 0; k < 8; k++) {
                        if(bm->Flags & HAM) {
                            int col = 0;
                            char cmd = 0;

                            for (l = 0; l < depth - 2; l++) {
                                char *plane = (char *) bm->Planes[l];
                                char bit_set = plane[i*bytes_per_line+j] & (1 << (7-k));
                                if (!bitplane_enable[l]) {
                                    bit_set = 0;
                                }
                                col = col | (bit_set ? (1 << l) : 0);
                            }

                            for (l = depth - 2; l < depth; l++) {
                                char *plane = (char *) bm->Planes[l];
                                char bit_set = plane[i*bytes_per_line+j] & (1 << (7-k));
                                cmd = cmd | (bit_set ? (1 << (l - (depth - 2))) : 0);
                            }

                            switch(cmd) {
                            case 0:
                                // set another col
                                if(depth == 6) {
                                    rgb = ctable[col];
                                } else {
                                    rgb = ctable[col] & 0xfcfcfc;
                                }
                                break;
                            case 1:
                                // change b
                                if(depth == 6) {
                                    rgb = (rgb & 0xffff00) | (col << 4) | (col << 0);
                                } else {
                                    rgb = (rgb & 0xfcfc00) | (col << 2);
                                }
                                break;
                            case 3:
                                // change g
                                if(depth == 6) {
                                    rgb = (rgb & 0xff00ff) | (col << 12) | (col << 8);
                                } else {
                                    rgb = (rgb & 0xfc00fc) | (col << 10);
                                }
                                break;
                            case 2:
                                // change r
                                if(depth == 6) {
                                    rgb = (rgb & 0x00ffff) | (col << 20) | (col << 16);
                                } else {
                                    rgb = (rgb & 0x00fcfc) | (col << 18);
                                }
                                break;
                            }
                        } else if(bm->Flags & EHB) {
                            int half = 0;

                            cindex = 0;

                            for (l = 0; l < depth; l++) {
                                char *plane = (char *) bm->Planes[l];
                                char bit_set = plane[i*bytes_per_line+j] & (1 << (7-k));
                                if (!bitplane_enable[l]) {
                                    bit_set = 0;
                                }

                                if(l == depth-1) {
                                    if(bit_set) {
                                        half = 1;
                                    }
                                } else {
                                    cindex = cindex | (bit_set ? (1 << l) : 0);
                                }
                            }

                            rgb = ctable[cindex];

                            if(half) {
                                rgb = ((rgb >> 1) & 0xff0000) |
                                    ((rgb >> 1) & 0x00ff00) |
                                    ((rgb >> 1) & 0x0000ff);
                            }
                        } else {
                            cindex = 0;

                            for (l = 0; l < depth; l++) {
                                char *plane = (char *) bm->Planes[l];
                                char bit_set = plane[i*bytes_per_line+j] & (1 << (7-k));
                                if (!bitplane_enable[l]) {
                                    bit_set = 0;
                                }
                                cindex = cindex | (bit_set ? (1 << l) : 0);
                            }

                            rgb = ctable[cindex];
                        }

						scale_x = scale * sx;
						while (scale_x) {
							b = pix; pix++;
							g = pix; pix++;
							r = pix; pix++; pix++;

                            *r = (rgb >> 16) & 0xff;
                            *g = (rgb >> 8) & 0xff;
                            *b = rgb & 0xff;

                            if(xid->ocs) {
                                *r = (*r & 0xf0) | (*r & 0xf0) >> 4;
                                *g = (*g & 0xf0) | (*g & 0xf0) >> 4;
                                *b = (*b & 0xf0) | (*b & 0xf0) >> 4;
                            }

							scale_x--;
						}
					}
				}
				scale_y--;
			}
		}

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

void ximage_free(XImage *ximg)
{
	debugmsg("ximage_free()");
	// also frees imagedata (according to the docs)
	if (ximg) XDestroyImage(ximg);
}
