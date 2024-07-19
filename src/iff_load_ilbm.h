/*
 * xiffview by r. eberle
 *
 * distributed under the terms of
 * AWeb Public License Version 1.0
 * https://www.yvonrozijn.nl/aweb/apl.txt
*/

#define IFF_ILBM_COLORS_MAX	    256
#define IFF_ILBM_BITPLANES_MAX	8

/*  Bitmap header (BMHD) structure  */
typedef struct {
	UWORD   w, h;           /*  Width, height in pixels */
	WORD    x, y;           /*  x, y position for this bitmap  */
	UBYTE   nPlanes;        /*  # of planes  */
	UBYTE   Masking;
	UBYTE   Compression;
	UBYTE   pad1;
	UWORD   TransparentColor;
	UBYTE   XAspect, YAspect;
	WORD    PageWidth, PageHeight;
} BitMapHeader;

int iff_load_ilbm_header(char *, BitMapHeader *);
int iff_load_ilbm(char *, struct BitMap *, UWORD *, BitMapHeader *);
