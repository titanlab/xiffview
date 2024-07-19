/*
 * xiffview by r. eberle
 *
 * distributed under the terms of
 * AWeb Public License Version 1.0
 * https://www.yvonrozijn.nl/aweb/apl.txt
*/

#include <stdlib.h>
#include <string.h>

#ifdef DEBUG
#include "../../../lib-c/debug.h"
#else
#include "debug_none.h"
#endif

#include "usermsg.h"
#include "bytereverse.h"
#include "types.h"
#include "iffparse.h"
#include "iff_load_ilbm.h"

// datatypes/pictureclass.h:219:#define
#define cmpNone                 0
#define cmpByteRun1             1

#define ID_ILBM              MAKE_ID('I', 'L', 'B', 'M')
#define ID_BMHD              MAKE_ID('B', 'M', 'H', 'D')
#define ID_CMAP              MAKE_ID('C', 'M', 'A', 'P')
//#define ID_CAMG              MAKE_ID('C', 'A', 'M', 'G')
//#define ID_DPI               MAKE_ID('D', 'P', 'I', ' ')
#define ID_BODY              MAKE_ID('B', 'O', 'D', 'Y')

// large enough for one line of superhires = 1280 = 160 * 8
UBYTE iff_scanline[160];
LONG iff_scanline_len; // actual current len (screen size)

/* Commodore Amiga (CAMG) Viewmodes structure */
//typedef struct {
   //ULONG ViewModes;
//} CamgChunk;

ULONG iff_load_ilbm_uncompressed(struct IFFHandle *iff, struct BitMap *bm, BitMapHeader *bmh) {
	LONG num;
	int i, j, k;
	UBYTE *mem;
	LONG done = 0L;
	int offset = 0;
	int lines = bmh->h;
	int planes = bmh->nPlanes;

	// for each scanline...
	for (i = 0; i < lines; i++) {
		//debugerr("iff_load() - scanline: %d - togo: %ld - cn->cn_Scan: %ld", i, togo, cn->cn_Scan);
		// ...loop though planes:
		for (j = 0; j < planes; j++) {
			mem = (UBYTE *) bm->Planes[j];
			mem += offset;
			num = ReadChunkBytes(iff, mem, iff_scanline_len);
			if (num == iff_scanline_len) {
				done += num;
				mem += num;
			} else {
				debugerr("iff_load() - failed to read %ld bytes from BODY: %ld (plane: %u)", iff_scanline_len, num, j);
				// break out:
				i = lines;
				j = planes;
			}
		}
		offset += iff_scanline_len;
	}
	return done;
}

ULONG iff_load_ilbm_compressed(struct IFFHandle *iff, struct BitMap *bm, LONG size, BitMapHeader *bmh) {
	LONG num;
	int i, j, k;
	BYTE bytebuf;
	ULONG bytebuflw;
	UBYTE *mem;
	ULONG *memlw;
	ULONG done = 0L;
	int offset = 0;
	int ok = 1;
	int lines = bmh->h;
	int planes = bmh->nPlanes;
	debugmsg("iff_load_ilbm_compressed() - lines: %d - planes: %d", lines, planes);

	UBYTE *body = malloc(size); // keep body for free()
	UBYTE *body_ptr = body;     // use body_ptr for parsing
	if (!body) {
		usererr("iff_load_ilbm_compressed() - could not allocate mem for body", NULL);
		return done;
	}

	num = ReadChunkBytes(iff, body, size);
	if (num != size) {
		usererr("iff_load_ilbm_compressed() - could not read body", NULL);
		free(body);
		return done;
	}
	//debugmsg("iff_load_ilbm_compressed() - read %ld body bytes", num);
	
	// for each scanline...
	for (i = 0; i < lines; i++) {
		// ...loop though planes:
		for (j = 0; j < planes; j++) {
			mem = (UBYTE *) bm->Planes[j];
			mem += offset;

			//debugmsg("decompressing %d %d", i, j);
			int todo_scanline = iff_scanline_len;
			//int safety = 0;
			// while scanline needs bytes...
			while (todo_scanline > 0) {
				/* ...get a byte, call it N (= our bytebuf) */
				bytebuf = *body_ptr; body_ptr++; 
				done++;
				if (bytebuf & 0x80) {
					//debugmsg("bytebuf negative: %d", bytebuf);
					if (bytebuf & 0x7f) {
						//debugmsg("bytebuf >= -127 and <= -1: %d", bytebuf);
						/* byte >= -127 and <= -1: repeat the next byte -N+1 times */
						k = (int) -bytebuf+1;
						bytebuf = *body_ptr; body_ptr++; 
						done++;

						int uselw = 1;
						/*
						Mon 14 Jun 2021 11:42:32 AM CEST
						linux / intel seems to be able to access any memory address as 
						longword, so we don't need to check for word boundaries or so.
						(until we create some Amiga version, of course. maybe.)

						int uselw = 0;
						if ((unsigned long) mem % 2) { 
							//debugmsg("byte is: 0x%02x - mem 0x%08x -> uneven, must use bytes", bytebuf, (unsigned long) mem); 
						} else { 
							//debugmsg("byte is: 0x%02x - mem 0x%08x -> even, trying longwords...", bytebuf, (unsigned long) mem);
							//uselw = 1; 
						}
						*/

						if (uselw && k >= 4) {
							//debugmsg("using longword %d %d - k: %d", i, j, k);
							//bytebuflw  = 0L | bytebuf;
							((char *) &bytebuflw)[0] = bytebuf;
							((char *) &bytebuflw)[1] = bytebuf;
							((char *) &bytebuflw)[2] = bytebuf;
							((char *) &bytebuflw)[3] = bytebuf;
							memlw = (ULONG *) mem;
							while (k >= 4) {
								*memlw = bytebuflw;
								memlw++;
								todo_scanline -= 4;
								k -= 4;
							}
							mem = (UBYTE *) memlw;
						}
						while (k) {
							*mem = bytebuf;
							mem++;
							todo_scanline--;
							k--;
						}
					} else {
						/* byte == -128: skip it, presumably it's padding */
						//debugmsg("bytebuf -128, skipping: %d", bytebuf);
					}
				} else {
					/* byte >= 0 and <= 127: copy the next N+1 bytes literally */
					bytebuf++;
					//debugmsg("copy literally: line: %d plane: %d - bytes (n+1): %d", i, j, bytebuf);
					memcpy(mem, body_ptr, (LONG) bytebuf);
					body_ptr += bytebuf;
					done += (LONG) bytebuf;
					todo_scanline -= (int) bytebuf;
					mem += bytebuf;
				}

				/*
				safety++;            // safety
				if (safety > 1000) { // safety
					debugerr("iff_load() - emergency exit, todo_scanline still > 0!(comp)");
					ok = 0; break;   // safety
				}                    // safety
				*/
			}
		}
		offset += iff_scanline_len;
	}
	free(body); // free body, not body_ptr
	return done;
}

// filepath:     file path
// bmh:          pointer to memory to store data to
int iff_load_ilbm_header(char *filepath, BitMapHeader *bmh) {
	int error;
	struct StoredProperty *sprop;
	int ok = 0;
	struct IFFHandle *iff = AllocIFF();
	if (iff) {
		// for Linux: no casting here, must use real pointer (size)
		//iff->iff_Stream = (ULONG) Open(filepath, MODE_OLDFILE);
		iff->iff_Stream = Open(filepath, MODE_OLDFILE);
		if (iff->iff_Stream) {
			InitIFFasDOS(iff);

			error = OpenIFF(iff, IFFF_READ);
			if (error) {
				usererr("could not OpenIFF()", NULL);
			} else {
				PropChunk(iff, ID_ILBM, ID_BMHD);
				StopChunk(iff, ID_ILBM, ID_BMHD);
				ParseIFF(iff, IFFPARSE_SCAN);
				sprop = FindProp(iff, ID_ILBM, ID_BMHD);
				if (sprop) {
					memcpy(bmh, sprop->sp_Data, sizeof(BitMapHeader));
					// for Linux: we're probably LSB-first / little endian
					//            so we reverse all multi byte fields
					bytereverse((char *) &bmh->w, 2);
					bytereverse((char *) &bmh->h, 2);
					bytereverse((char *) &bmh->x, 2);
					bytereverse((char *) &bmh->y, 2);
					bytereverse((char *) &bmh->TransparentColor, 2);
					bytereverse((char *) &bmh->PageWidth, 2);
					bytereverse((char *) &bmh->PageHeight, 2);
					//debugmsg("BMHD: PageWidth/Height\t= %ld/%ld", bmh->PageWidth, bmh->PageHeight);
					//debugmsg("BMHD: Compression\t= %d",           bmh->Compression);
					ok = 1;
				} else {
					usererr("no BMHD, no picture", NULL);
				}
				CloseIFF(iff);
			}
			Close((BPTR) iff->iff_Stream);
		} else {
			usererr("no iff stream", NULL);
		}
		FreeIFF(iff);
	} else {
		usererr("could not allocate iff handle", NULL);
	}
	if (ok) return 1;
	return 0;
}

/*
load iff into given bitmap memory, and colormap memory
from given filepath, using information stored in given BitMapHeader

bitmap must be set up to hold image data, otherwise error
*/

int iff_load_ilbm(char *filepath, struct BitMap *bm, UWORD *colortable, BitMapHeader *bmh) {
	//debugmsg("iff_load()");
	int ret = 0;
	int ok = 1;

	int i, j, error;
	UBYTE *mem;
	struct IFFHandle	*iff = NULL;
	struct StoredProperty *sprop;      /* defined in iffparse.h */
	LONG spropsize;
	UBYTE *spropdata;

	iff = AllocIFF();
	if (!iff) {
		debugerr("iff_load() - no iff");
		return ret;
	}
	// for Linux: no casting here, must use real pointer (size)
	//iff->iff_Stream = (ULONG) Open(filepath, MODE_OLDFILE);
	iff->iff_Stream = Open(filepath, MODE_OLDFILE);
	if (iff->iff_Stream) {
		InitIFFasDOS(iff);
		error = OpenIFF(iff, IFFF_READ);
		if (!error) {

			PropChunk(iff, ID_ILBM, ID_CMAP);
			//PropChunk(iff, ID_ILBM, ID_CAMG);
			//PropChunk(iff, ID_ILBM, ID_DPI);
			StopChunk(iff, ID_ILBM, ID_BODY);
			ParseIFF(iff, IFFPARSE_SCAN);

			// CMAP
			// rgb, 8 bit, left aligned 4-bit value (double to 8 bit when writing)
			if (sprop = FindProp(iff, ID_ILBM, ID_CMAP)) {
				i = sprop->sp_Size / 3; // number of colors, 3 = R, G, B
				if (i <= IFF_ILBM_COLORS_MAX) {
					// some programs - e.g. ppmtoilbm - seem to generate shorter colormaps, if possible.
					// so we simply always fill up to bitmap's number of colors with zeroes/black.
					if (i <= (1 << bm->Depth)) {
						// docs say: iff colormap is 1 byte per component (r, g, b),
						// each component left aligned 4-bit value.
						mem = sprop->sp_Data;
						UBYTE byte;
						UWORD rgb4;
						for (j = 0; j < (1 << bm->Depth); j++) {
							if (j < i) {
								// red (at blue position)
								byte = *mem; //printf("r: 0x%02x ", byte);
								rgb4 = 0 | (byte >> 4);
								rgb4 = rgb4 << 4;
								mem++;
								// green (at blue position, red at green)
								byte = *mem; //printf("g: 0x%02x ", byte);
								rgb4 = rgb4 | (byte >> 4);
								rgb4 = rgb4 << 4;
								mem++;
								// blue (red at red, green at green, nice)
								byte = *mem; //printf("b: 0x%02x ", byte);
								rgb4 = rgb4 | (byte >> 4);
								mem++;
							} else {
								rgb4 = 0x0000;
							}
							colortable[j] = rgb4; // ok, copy to our colortable
							//printf("-> rgb4: 0x%04x\n", colortable[j]);
						}

					} else {
						debugerr("iff_load() - colormap size > bitmap colors (planes): %d vs. %d", i, 1 << bm->Depth);
						ok = 0;
					}
				} else {
					debugerr("iff_load() - colormap too big for colortable: %ld -> %d", i, IFF_ILBM_COLORS_MAX);
					ok = 0;
				}
			} else {
				debugerr("iff_load() - no colormap");
				ok = 0;
			} 

			//if (sprop = FindProp (iff, ID_ILBM, ID_CAMG)) { }
			//if (sprop = FindProp (iff, ID_ILBM, ID_DPI)) { }

			if (ok) {
				struct ContextNode *cn = CurrentChunk(iff);
				if (cn && cn->cn_Type == ID_ILBM && cn->cn_ID == ID_BODY) {
					//debugmsg("iff_load() - cn->cn_Scan: %ld", cn->cn_Scan);

					LONG size = cn->cn_Size;
					debugmsg("iff_load_ilbm() - body size: %d", size);
					iff_scanline_len = bmh->w / 8; // 8 = 1 byte
					if (bmh->Compression == cmpNone) {
						size -= (LONG) iff_load_ilbm_uncompressed(iff, bm, bmh);
					} else if (bmh->Compression == cmpByteRun1) {
						debugmsg("iff_load() - loading...");
						size -= (LONG) iff_load_ilbm_compressed(iff, bm, size, bmh);
						debugmsg("iff_load() - loading... done (new size: %d)", size);
					} else {
						debugerr("iff_load() - unknown compression: %d", bmh->Compression);
					}

					if (size != 0) {
						debugerr("iff_load() - still some bytes to go! %d", size);
						ok = 0;
					}
				} else {
					char *c = (char *) &cn->cn_Type;
					debugerr("iff_load() - wrong chunk: type: %c %c %c %c", c[0], c[1], c[2], c[3]);
					c = (char *) &cn->cn_ID;
					debugerr("iff_load() - wrong chunk: id  : %c %c %c %c", c[0], c[1], c[2], c[3]);
					ok = 0;
				}
			}
			CloseIFF(iff);
		}
		Close((BPTR) iff->iff_Stream);
	} else {
		debugerr("iff_load() - no iff stream");
		ok = 0;
	}
	FreeIFF(iff);

	if (ok) ret = 1;
	return ret;
}
