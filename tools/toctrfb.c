#include <stdio.h>
#include <stdlib.h>
#include <wand/MagickWand.h>

#define TRANSFORM(X, Y) ((((239 - X) * 320) + Y) * 3)

char srcpixels[320 * 240 * 3];
short dstpixels[320 * 240];

int main(int argc,char **argv) {
#define ThrowWandException(wand) { \
  char *description; \
 \
  ExceptionType severity; \
 \
  description=MagickGetException(wand,&severity); \
  (void) fprintf(stderr,"%s %s %lu %s\n",GetMagickModule(),description); \
  description=(char *) MagickRelinquishMemory(description); \
  exit(-1); \
}

  MagickBooleanType status;

  MagickWand *magick_wand;
  
  int x, y;
  FILE *out;

  if (argc != 3) {
    (void) fprintf(stdout,"Usage: %s image thumbnail\n",argv[0]);
    exit(0);
  }
  /*
    Read an image.
  */
  MagickWandGenesis();
  magick_wand=NewMagickWand();
  status=MagickReadImage(magick_wand,argv[1]);
  if (status == MagickFalse)
    ThrowWandException(magick_wand);
  /*
    Turn the images into a thumbnail sequence.
  */
  MagickResetIterator(magick_wand);
  if (MagickNextImage(magick_wand) == MagickFalse) {
    ThrowWandException(magick_wand);
  }


  status=MagickExportImagePixels(magick_wand, 0, 0, 320, 240, "RGB", CharPixel, srcpixels);
  if (status == MagickFalse)
    ThrowWandException(magick_wand);
  magick_wand=DestroyMagickWand(magick_wand);
  // we're done with magickwand here
  MagickWandTerminus();
  
  // rotate clockwise 90 and flip horizontally
  for(y = 0; y < 320; y++) {
    for(x = 0; x < 240; x++) {
      dstpixels[y * 240 + x] = (short)(srcpixels[(TRANSFORM(x, y) + 0)] >> 3 & 0x1F) << 11 |
                               (short)(srcpixels[(TRANSFORM(x, y) + 1)] >> 2 & 0x3F) <<  5 |
                               (short)(srcpixels[(TRANSFORM(x, y) + 2)] >> 3 & 0x1F) <<  0;
    }
  }
  
  out = fopen(argv[2], "wb");
  if(out == NULL) {
    fprintf(stderr, "Couldn't open file.\n");
    exit(-1);
  }
  if(fwrite(dstpixels, 1, 320 * 240 * 2, out) < 320 * 240 * 2) {
    fprintf(stderr, "Couldn't write image data.\n");
    fclose(out);
    exit(-1);
  }
  fclose(out);
  
  return(0);
}
