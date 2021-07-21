/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef  __GL_IMAGE_H__
#define  __GL_IMAGE_H__
/*
*  Defines for image files . . . .
*
*        Paul Haeberli - 1984
*      Look in /usr/people/4Dgifts/iristools/imgtools for example code!
*
*/

#include <stdio.h>

#define IMAGIC   0732

/* colormap of images */
#define CM_NORMAL    0  /* file contains rows of values which
					* are either RGB values (zsize == 3)
					* or greyramp values (zsize == 1) */
#define CM_DITHERED    1
#define CM_SCREEN    2  /* file contains data which is a screen
					* image; getrow returns buffer which
					* can be displayed directly with
					* writepixels */
#define CM_COLORMAP    3  /* a colormap file */

#define TYPEMASK    0xff00
#define BPPMASK      0x00ff
#define ITYPE_VERBATIM    0x0000
#define ITYPE_RLE    0x0100
#define ISRLE(type)    (((type) & 0xff00) == ITYPE_RLE)
#define ISVERBATIM(type)  (((type) & 0xff00) == ITYPE_VERBATIM)
#define BPP(type)    ((type) & BPPMASK)
#define RLE(bpp)    (ITYPE_RLE | (bpp))
#define VERBATIM(bpp)    (ITYPE_VERBATIM | (bpp))
#define  IBUFSIZE(pixels)  ((pixels+(pixels>>6))<<2)
#define  RLE_NOP      0x00

#define  ierror(p)    (((p)->flags&_IOERR)!=0)
#define  ifileno(p)    ((p)->file)
#define  getpix(p)    (--(p)->cnt>=0 ? *(p)->ptr++ : ifilbuf(p))
#define putpix(p,x)    (--(p)->cnt>=0 \
						? ((int)(*(p)->ptr++=(unsigned)(x))) \
						: iflsbuf(p,(unsigned)(x)))

typedef struct {
		unsigned short  imagic;    /* stuff saved on disk . . */
		unsigned short   type;
		unsigned short   dim;
		unsigned short   xsize;
		unsigned short   ysize;
		unsigned short   zsize;
		unsigned long   min;
		unsigned long   max;
		unsigned long  wastebytes;
		char     name[80];
		unsigned long  colormap;

		long     file;    /* stuff used in core only */
		unsigned short   flags;
		short    dorev;
		short    x;
		short    y;
		short    z;
		short    cnt;
		unsigned short  *ptr;
		unsigned short  *base;
		unsigned short  *tmpbuf;
		unsigned long  offset;
		unsigned long  rleend;    /* for rle images */
		unsigned long  *rowstart;  /* for rle images */
		long    *rowsize;  /* for rle images */
} IMAGE;


/*???Edouard.  We could do with better prototypes here. We need ones for
	iopen(), iclose() and longimagedata() at least.  I'll at least give it a go
	for now... */
IMAGE *iopen(char *, char *);
void iclose(IMAGE *);
long unsigned *longimagedata(char *);

/*???Edouard.  Old prototypes */
/*IMAGE *iopen();
IMAGE *icreate();
unsigned short *ibufalloc();*/

#define IMAGEDEF    /* for backwards compatibility */
#endif  /* !__GL_IMAGE_H__ */
