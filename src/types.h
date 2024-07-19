/*
 * xiffview by r. eberle
 *
 * distributed under the terms of
 * AWeb Public License Version 1.0
 * https://www.yvonrozijn.nl/aweb/apl.txt
*/

// 1 byte
typedef signed char         BYTE     ;
typedef unsigned char       UBYTE    ;
// 2 bytes
typedef short int           WORD     ;
typedef unsigned short int  UWORD    ;
// 4 bytes
typedef signed int          LONG     ;
typedef unsigned int        ULONG    ;
// 8 bytes pointers 
// (be aware! Amiga pointers are 4 bytes, and may get casted to ULONG or so)
typedef void *              APTR     ;
typedef unsigned long *     BPTR     ;
typedef unsigned long *     PLANEPTR ;
typedef char *              STRPTR   ;
// 2 bytes
typedef short               BOOL     ;

struct BitMap {
    UWORD   BytesPerRow;
    UWORD   Rows;
    UBYTE   Flags;
    UBYTE   Depth;
    UWORD   pad;
    PLANEPTR Planes[8];
};

// from exec/nodes.h
struct MinNode {
    struct MinNode *mln_Succ;
    struct MinNode *mln_Pred;
};

void types_check(void);
