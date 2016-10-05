/*
* randomArt() taken from OpenSSH and bent to fit this project.
* Original license and commentary reproduced below.
*                                                    ---J. Ian Lindsay
*
* http://cvsweb.openbsd.org/cgi-bin/cvsweb/src/usr.bin/ssh/key.c?rev=1.75&content-type=text/x-cvsweb-markup
*
* Copyright (c) 2000, 2001 Markus Friedl.  All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
* IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
* OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
* NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
* THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
* THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
* Draw an ASCII-Art representing the fingerprint so human brain can
* profit from its built-in pattern recognition ability.
* This technique is called "random art" and can be found in some
* scientific publications like this original paper:
*
* "Hash Visualization: a New Technique to improve Real-World Security",
* Perrig A. and Song D., 1999, International Workshop on Cryptographic
* Techniques and E-Commerce (CrypTEC '99)
* sparrow.ece.cmu.edu/~adrian/projects/validation/validation.pdf
*
* The subject came up in a talk by Dan Kaminsky, too.
*
* If you see the picture is different, the key is different.
* If the picture looks the same, you still know nothing.
*
* The algorithm used here is a worm crawling over a discrete plane,
* leaving a trace (augmenting the field) everywhere it goes.
* Movement is taken from dgst_raw 2bit-wise.  Bumping into walls
* makes the respective movement vector be ignored for this turn.
* Graphs are not unambiguous, because circles in graphs can be
* walked in either direction.
*/

/*
* Field sizes for the random art.  Have to be odd, so the starting point
* can be in the exact middle of the picture, and FLDBASE should be >=8 .
* Else pictures would be too dense, and drawing the frame would
* fail, too, because the key type would not fit in anymore.
*/
#include <inttypes.h>
#include <DataStructures/StringBuilder.h>

#define	FLDBASE		8
#define	FLDSIZE_Y	(FLDBASE + 1)
#define	FLDSIZE_X	(FLDBASE * 2 + 1)

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

int randomArt(uint8_t* dgst_raw, unsigned int dgst_raw_len, const char* key_type, StringBuilder* output) {
	/*
	* Chars to be used after each other every time the worm
	* intersects with itself.  Matter of taste.
	*/
	char	*augmentation_string = " .o+=*BOX@%&#/^SE";
	unsigned char	 field[FLDSIZE_X][FLDSIZE_Y];
	unsigned int	 b;
	int	 x, y;
	unsigned int	 len = strlen(augmentation_string) - 1;

	//char* retval = malloc((FLDSIZE_X + 3) * (FLDSIZE_Y + 2));
	/* initialize field */
	memset(field, 0, FLDSIZE_X * FLDSIZE_Y);
	x = FLDSIZE_X / 2;
	y = FLDSIZE_Y / 2;

	/* process raw key */
	for (unsigned int i = 0; i < dgst_raw_len; i++) {
		int input;
		/* each byte conveys four 2-bit move commands */
		input = dgst_raw[i];
		for (b = 0; b < 4; b++) {
			/* evaluate 2 bit, rest is shifted later */
			x += (input & 0x1) ? 1 : -1;
			y += (input & 0x2) ? 1 : -1;

			/* assure we are still in bounds */
			x = max(x, 0);
			y = max(y, 0);
			x = min(x, FLDSIZE_X - 1);
			y = min(y, FLDSIZE_Y - 1);

			/* augment the field */
			field[x][y]++;
			input = input >> 2;
		}
	}

	/* mark starting point and end point*/
	field[FLDSIZE_X / 2][FLDSIZE_Y / 2] = len - 1;
	field[x][y] = len;

	/* fill in retval */
	output->concatf("+--[%s]", key_type);

	/* output upper border */
	for (unsigned int i = 4+strlen(key_type); i < FLDSIZE_X; i++) {
		output->concat('-');
	}
	output->concat("+\n");

	/* output content */
	for (y = 0; y < FLDSIZE_Y; y++) {
		output->concat('|');
		for (x = 0; x < FLDSIZE_X; x++) {
			output->concat((char) augmentation_string[min(field[x][y], len)]);
		}
		output->concat("|\n");
	}

	/* output lower border */
	output->concat('+');
	for (unsigned int i = 0; i < FLDSIZE_X; i++) {
		output->concat('-');
	}
	output->concat("+\n");

	return 0;
}
