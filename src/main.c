/*
 * xiffview by r. eberle
 *
 * distributed under the terms of
 * AWeb Public License Version 1.0
 * https://www.yvonrozijn.nl/aweb/apl.txt
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

#ifdef DEBUG
#include "../../../lib-c/debug.h"
#else
#include "debug_none.h"
#endif
#include "usermsg.h"
#include "app.h"
#include "types.h"
#include "rgb.h"
#include "iff_load_ilbm.h" 
#include "cmd.h"
#include "gui.h"

char aplstring[] = "distributed under the terms of AWeb Public License Version 1.0";

struct appdata appdata_mem;
struct appdata *appdata = &appdata_mem;

// BitMapHeader is from iff_load_ilbm.h
BitMapHeader        *iff_bmh = NULL; 
// BitMap is from types.h
struct BitMap       *img_bm = NULL;
// AmigaOS colortable (large enough to hold any number of colors)
UWORD               img_colortable_rgb4[IFF_ILBM_COLORS_MAX];
// bordersize around image
int border = 20;

// we allocate one big chunk containing all planes,
// and set pointers to inside chunk - makes creating
// XImage much simpler.
int allocplanes(struct BitMap *b) {
	int i;
	int err = 0;
	int psize = b->BytesPerRow * b->Rows;
	// just allocate first plane, one big chunk.
	if (b->Planes[0] = (PLANEPTR) malloc(psize * b->Depth)) {
		char *mem = (char *) b->Planes[0];
		for (i = 1; i < b->Depth; i++) {
			mem += psize;
			b->Planes[i] = (PLANEPTR) mem;
		}
		debugmsg("alloc'd %d planes (b->Depth: %d)", i, b->Depth);
	} else {
		err = 1;
		usererr("could not allocate bitmap bitplanes", NULL);
	}
	if (err) return 0;
	return 1;
}

void freeplanes(struct BitMap *b) {
	int i;
	if (b->Planes[0]) {
		// just free first plane. remember: one big chunk.
		free(b->Planes[0]);
		for (i = 0; i < b->Depth; i++) {
			b->Planes[i] = NULL;
		}
	}
}


// ============================================================================
// main()
// ============================================================================
#define MODE_DEFAULT    1
#define MODE_CMD        2

int main(int argc, char **argv) {

//UWORD foo = 0x8000;
//printf("0x%04x\n", foo);
//printf("0x%02x", 0x80);
//printf("0x%04x", foo);
//return EXIT_SUCCESS;

	int i, numcolors;
	int mode = MODE_DEFAULT;
	int opt_cmd = 0;
	int opt_outfile = 0;
	sprintf(appdata->filename, "");
	sprintf(appdata->filename_output, "");
	sprintf(appdata->cmdstring, "");
	for (i = 0; i < IFF_ILBM_BITPLANES_MAX; i++) {
		appdata->gui_image_bitplane_enable[i] = 1;
	}
	appdata->gui_image_scale = 1;

#ifdef DEBUG
	types_check();
#endif

	for (i = 1; i < argc; i++) {
		if (opt_cmd) {
			debugmsg("opt-val: command string found");
			if (mode == MODE_CMD) {
				usererr("only one command please", NULL);
				return EXIT_FAILURE;
			}
			if (strlen(argv[i]) < APP_CMD_STRING_MAXLEN) {
				mode = MODE_CMD;
				strcpy(appdata->cmdstring, argv[i]);
				opt_cmd = 0;
			} else {
				usererr("command string too long", NULL);
				return EXIT_FAILURE;
			}
			continue;
		}
		if (opt_outfile) {
			debugmsg("opt-val: outfile found");
			if (strlen(appdata->filename_output)) {
				usererr("only one output filename please", NULL);
				return EXIT_FAILURE;
			}
			if (strlen(argv[i]) < APP_FILENAME_MAXLEN) {
				strcpy(appdata->filename_output, argv[i]);
				opt_outfile = 0;
			} else {
				usererr("output filename too long", NULL);
				return EXIT_FAILURE;
			}
			continue;
		}

		if (!strcmp(argv[i], "-v")) {
			printf(APP_STRING);
			printf("\n");
			return EXIT_SUCCESS;
		} else if (!strcmp(argv[i], "-c")) {
			debugmsg("opt -c found");
			opt_cmd = 1;
		} else if (!strcmp(argv[i], "-o")) {
			debugmsg("opt -o found");
			opt_outfile = 1;
		} else {
			debugmsg("arg filename found");
			if (strlen(appdata->filename)) {
				usererr("only one (input) file please", NULL);
				return EXIT_FAILURE;
			}
			if (strlen(argv[i]) < APP_FILENAME_MAXLEN) {
				strcpy(appdata->filename, argv[i]);
			} else {
				usererr("filename too long", NULL);
				return EXIT_FAILURE;
			}
		}
	}
	if (mode == MODE_CMD && !strlen(appdata->cmdstring)) {
		usererr("need command string", NULL);
		return EXIT_FAILURE;
	}
	if (!strlen(appdata->filename)) {
		usererr("need input IFF image filename", NULL);
		return EXIT_FAILURE;
	}

	debugmsg("filename: %s", appdata->filename);
	debugmsg("filename_output (pre-command): %s", appdata->filename_output);

	iff_bmh = (BitMapHeader *) malloc(sizeof(BitMapHeader));
	if (iff_bmh) {
		//debugmsg("bitmapheader alloc'd (sizeof: %d)", sizeof(BitMapHeader));

		if (iff_load_ilbm_header(appdata->filename, iff_bmh)) {
			debugmsg("header loaded");

			img_bm = (struct BitMap *) malloc(sizeof(struct BitMap));
			if (img_bm) {
				img_bm->Depth = iff_bmh->nPlanes;
				img_bm->BytesPerRow = iff_bmh->w/8;
				img_bm->Rows = iff_bmh->h;
				numcolors = 1 << img_bm->Depth;

				if (allocplanes(img_bm)) {
					//debugmsg("planes alloc'd");
					if (iff_load_ilbm(appdata->filename, img_bm, img_colortable_rgb4, iff_bmh)) {
						debugmsg("iff loaded");

                        if (mode == MODE_CMD) {
							// Command mode / no image display / CLI operation
							cmd_exec(appdata, img_bm, img_colortable_rgb4, iff_bmh);

                        } else {
							// GUI mode / image display
                            if (gui_new()) {
								Visual *vis = gui_getVisual();
								Display *dis = gui_getDisplay();
								int scr = gui_getScreen();
								if (vis->class != TrueColor) {
									usererr("sorry, must be truecolor display", NULL);
								}

								gui_setimage(appdata->filename, img_bm, img_colortable_rgb4, numcolors);

								if (gui_open()) {
									gui_mainloop();
									gui_close();
								} else {
									usererr("could not open GUI", NULL);
								}
								gui_delete();
							} else {
								usererr("could not init gui", NULL);
							}
                        }

					} else {
						usererr("could not load IFF ILBM file", NULL);
					}
					freeplanes(img_bm);
				} else {
					usererr("could not alloc BitMap planes", NULL);
				}
				free(img_bm);
			} else {
				usererr("could not alloc BitMap", NULL);
			}
		} else {
			usererr("could not load IFF ILBM bitmap header data", NULL);
		}
		free(iff_bmh);
	} else {
		usererr("could not alloc BitMapHeader", NULL);
	}

	//char a, b, c;
	//rgb4torgb8(0x0f00, &a, &b, &c);
	//rgb4torgb8(0x00f0, &a, &b, &c);
	//rgb4torgb8(0x000f, &a, &b, &c);
	//printf("rgb: %d %d %d", (int) a, (int) b, (int) c);
	
	//exit(EXIT_SUCCESS);
	return EXIT_SUCCESS; // pretty much zero. but EXIT_SUCCESS kinda feels better.
}
