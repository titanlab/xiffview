/*
 * xiffview by r. eberle
 *
 * distributed under the terms of
 * AWeb Public License Version 1.0
 * https://www.yvonrozijn.nl/aweb/apl.txt
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"
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
#define ID_CAMG              MAKE_ID('C', 'A', 'M', 'G')
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
	ULONG done = 0L, done_old = 0L;
	int offset = 0;
	int ok = 1;
	int lines = bmh->h;
	int planes = bmh->nPlanes;
    int masking = bmh->Masking;
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

    if(masking == 1) {
        planes++;
    }

	// for each scanline...
	for (i = 0; i < lines; i++) {
		// ...loop though planes:
		for (j = 0; j < planes; j++) {
            int write = 1;

            if(masking == 1 && j == planes-1) {
                write = 0;
            }

            if(write) {
                mem = (UBYTE *) bm->Planes[j];
                mem += offset;
            }

			//printf("decompressing %d %d %d %d -> %d (%d)\n", i, j, iff_scanline_len, done_old, done, done - done_old);
            done_old = done;
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
								if(write) {
                                    *memlw = bytebuflw;
								    memlw++;
                                }
								todo_scanline -= 4;
								k -= 4;
							}
							mem = (UBYTE *) memlw;
						}
						while (k) {
							if(write) {
                                *mem = bytebuf;
							    mem++;
                            }
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
					if(write) {
                        memcpy(mem, body_ptr, (LONG) bytebuf);
					    mem += bytebuf;
                    }
					body_ptr += bytebuf;
					done += (LONG) bytebuf;
					todo_scanline -= (int) bytebuf;
				}
			}
		}
		offset += iff_scanline_len;
	}

	free(body);
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
					//printf("BMHD: PageWidth/Height\t= %ld/%ld\n", bmh->PageWidth, bmh->PageHeight);
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

int iff_load_ilbm(char *filepath, struct BitMap *bm, ULONG *colortable, BitMapHeader *bmh) {
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
			PropChunk(iff, ID_ILBM, ID_CAMG);
			//PropChunk(iff, ID_ILBM, ID_DPI);
			StopChunk(iff, ID_ILBM, ID_BODY);
			ParseIFF(iff, IFFPARSE_SCAN);

            // CAMG
			if (sprop = FindProp (iff, ID_ILBM, ID_CAMG)) {
                if(sprop->sp_Size > 0) {
                    ULONG * data = sprop->sp_Data;
                    bytereverse((char *)data, 4);

                    if(data[0] & 0x4) {
                        bm->Flags |= LACE;
                    }

                    if(data[0] & 0x80) {
                        bm->Flags |= EHB;
                    }

                    if(data[0] & 0x800) {
                        bm->Flags |= HAM;
                    }

                    if(data[0] & 0x8000) {
                        bm->Flags |= HIRES;
                    }

                    // PAL, NTSC or Default monitor? -> NATIVE
                    if(((data[0] & 0xffff1000) == 0x00011000) ||
                       ((data[0] & 0xffff1000) == 0x00021000) ||
                       ((data[0] & 0xffff1000) == 0x00000000)) {
                        bm->Flags |= NATIVE;
                    }
                }
            }

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
						ULONG rgb;
						for (j = 0; j < (1 << bm->Depth - (bm->Flags & EHB ? 1 : 0)); j++) {
							if (j < i) {
								// red (at blue position)
								byte = *mem; //printf("r: 0x%02x ", byte);
								rgb = byte << 16;
								mem++;
								// green (at blue position, red at green)
								byte = *mem; //printf("g: 0x%02x ", byte);
								rgb |= byte << 8;
								mem++;
								// blue (red at red, green at green, nice)
								byte = *mem; //printf("b: 0x%02x ", byte);
                                rgb |= byte;
								mem++;
							} else {
								rgb = 0x00000000;
							}
							colortable[j] = rgb; // ok, copy to our colortable
							//printf("-> rgb8: 0x%08x\n", colortable[j]);
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

			if (ok) {
				struct ContextNode *cn = CurrentChunk(iff);
				if (cn && cn->cn_Type == ID_ILBM && cn->cn_ID == ID_BODY) {
					//debugmsg("iff_load() - cn->cn_Scan: %ld", cn->cn_Scan);

					LONG size = cn->cn_Size;
					debugmsg("iff_load_ilbm() - body size: %d", size);

                    iff_scanline_len = bmh->w / 16;
                    if((bmh->w % 16) != 0) {
                        iff_scanline_len++;
                    }
                    iff_scanline_len*=2;

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
						printf("iff_load() - still some bytes to go! %d\n", size);
						//ok = 0;
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
