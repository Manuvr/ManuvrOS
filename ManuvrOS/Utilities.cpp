/*
* randomArt() taken from github user nirenjan and bent to fit this project.
* No original license was found. Assuming attribution-only.
* https://gist.github.com/nirenjan/4450419
*                                                    ---J. Ian Lindsay
*
* "Hash Visualization: a New Technique to improve Real-World Security",
* Perrig A. and Song D., 1999, International Workshop on Cryptographic
* Techniques and E-Commerce (CrypTEC '99)
* sparrow.ece.cmu.edu/~adrian/projects/validation/validation.pdf
*/

#include <inttypes.h>
#include <DataStructures/StringBuilder.h>

#define XLIM 17
#define YLIM 9
#define ARSZ (XLIM * YLIM)

static const char* ra_symbols = " .:-+=*BoOX@&#^SE";


uint8_t ra_new_position(uint8_t *pos, uint8_t direction) {
  uint8_t newpos;
  uint8_t upd = 1;
  int8_t x0 = *pos % XLIM;
  int8_t y0 = *pos / XLIM;
  int8_t x1 = (direction & 0x01) ? (x0 + 1) : (x0 - 1);
  int8_t y1 = (direction & 0x02) ? (y0 + 1) : (y0 - 1);

  // Limit the range of x1 & y1
  if (x1 < 0) {
    x1 = 0;
  }
	else if (x1 >= XLIM) {
    x1 = XLIM - 1;
  }

  if (y1 < 0) {
    y1 = 0;
  }
	else if (y1 >= YLIM) {
    y1 = YLIM - 1;
  }

  newpos = y1 * XLIM + x1;

  if (newpos == *pos) {
    upd = 0;
  }
	else {
    *pos = newpos;
  }
  return upd;
}

#include <stdio.h>

int randomArt(uint8_t* dgst_raw, unsigned int dgst_raw_len, const char* title, StringBuilder* output) {
  if (0 >= dgst_raw_len) {
    return -1;
  }
  uint8_t array[ARSZ];
	bzero(&array, ARSZ);
  uint8_t pos = 76;

  for (uint16_t idx = 0; idx < dgst_raw_len; idx++) {
    uint8_t temp = *(dgst_raw + idx);
    for (uint8_t i = 0; i < 4; i++) {
      if (ra_new_position(&pos, (temp & 0x03))) array[pos]++;
      temp >>= 2;
    }
  }

  array[pos] = 16; // End
  array[76]  = 15; // Start

  StringBuilder temp;
  temp.concatf("+--[%10s ]--+\n", title);
	char line_buf[21];
	line_buf[0]  = '|';
	line_buf[18] = '|';
	line_buf[19] = '\n';
	line_buf[20] = '\0';
  for (uint8_t i = 0; i < YLIM; i++) {
		for (uint8_t j = 0; j < XLIM; j++) {
			*(line_buf + j + 1) = *(ra_symbols + array[j + (XLIM * i)] % 18);
		}
		temp.concatf(line_buf);
  }
  temp.concat("+-----------------+\n");
  temp.string();
  output->concatHandoff(&temp);
  return 0;
}
