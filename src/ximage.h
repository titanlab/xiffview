/*
 * xiffview by r. eberle
 *
 * distributed under the terms of
 * AWeb Public License Version 1.0
 * https://www.yvonrozijn.nl/aweb/apl.txt
*/

struct ximage_data {
	int scale;
	char *bitplane_enable;
    int ocs;
    int field;
    int fix_flicker;
};

// BitMap pointer, colortable, ximage_data pointer, Display, Visual, Screen
XImage *ximage_create(struct BitMap *, ULONG *, struct ximage_data *, Display *, Visual *, int);
// XImage * (NULL is allowed)
void   ximage_free(XImage *);
