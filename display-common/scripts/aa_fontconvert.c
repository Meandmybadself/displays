/*
Anti-aliased TrueType -> GFX-style font converter for this project.

Derived from Adafruit's fontconvert.c, but renders glyphs with FreeType's
grayscale (anti-aliased) rasterizer and emits 4-bit alpha coverage per pixel
instead of 1 bit. Output uses the AAfont/AAglyph structs (src/fonts/aa_font.h)
and is consumed by the aa_* blitter in src/display.cpp.

Packing: two 4-bit alpha pixels per byte, high nibble first, packed continuously
across the whole glyph; the last byte of each glyph is zero-padded if the pixel
count is odd. 8-bit FreeType coverage is quantized to 4 bits (v >> 4).

NOT AN ARDUINO SKETCH. Command-line tool. Requires FreeType.
  ./aa_fontconvert font.ttf size [first] [last]  > Name<size>pt7b.h
*/
#ifndef ARDUINO

#include <ctype.h>
#include <ft2build.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include FT_GLYPH_H
#include FT_MODULE_H

#define DPI 141 // Match Adafruit fontconvert so AA sizes line up with the 1-bit ones.

// Accumulate 4-bit alpha values, two per byte, with periodic hex output.
void ennib(uint8_t value) {
  static uint8_t row = 0, sum = 0, hi = 1, firstCall = 1;
  if (hi) {
    sum = (value & 0x0F) << 4; // high nibble
    hi = 0;
  } else {
    sum |= (value & 0x0F);     // low nibble -> emit byte
    hi = 1;
    if (!firstCall) {
      if (++row >= 12) {
        printf(",\n  ");
        row = 0;
      } else {
        printf(", ");
      }
    }
    printf("0x%02X", sum);
    sum = 0;
    firstCall = 0;
  }
}

// Local copy of the glyph metrics table entry (mirrors AAglyph in aa_font.h).
typedef struct {
  uint16_t bitmapOffset;
  uint8_t width, height, xAdvance;
  int8_t xOffset, yOffset;
} AAglyph_t;

int main(int argc, char *argv[]) {
  int i, j, err, first = ' ', last = '~', bitmapOffset = 0, x, y;
  double size; // fractional point sizes allowed (e.g. 9.45 for a 5% bump)
  char *fontName, c, *ptr;
  FT_Library library;
  FT_Face face;
  FT_Glyph glyph;
  FT_Bitmap *bitmap;
  FT_BitmapGlyphRec *g;
  AAglyph_t *table;

  if (argc < 3) {
    fprintf(stderr, "Usage: %s fontfile size [first] [last]\n", argv[0]);
    return 1;
  }

  size = atof(argv[2]);
  if (argc == 4) {
    last = atoi(argv[3]);
  } else if (argc == 5) {
    first = atoi(argv[3]);
    last = atoi(argv[4]);
  }
  if (last < first) {
    i = first;
    first = last;
    last = i;
  }

  ptr = strrchr(argv[1], '/');
  if (ptr)
    ptr++;
  else
    ptr = argv[1];

  if ((!(fontName = malloc(strlen(ptr) + 20))) ||
      (!(table = malloc((last - first + 1) * sizeof(AAglyph_t))))) {
    fprintf(stderr, "Malloc error\n");
    return 1;
  }

  strcpy(fontName, ptr);
  ptr = strrchr(fontName, '.');
  if (!ptr)
    ptr = &fontName[strlen(fontName)];
  // Keep the same naming convention as the 1-bit tool ("<size>pt7b"/"8b"). %g
  // keeps integer sizes clean ("19pt"); a fractional size like 9.45 becomes
  // "9.45pt", whose '.' is turned into '_' just below -> "9_45pt" (valid C ident).
  sprintf(ptr, "%gpt%db", size, (last > 127) ? 8 : 7);
  for (i = 0; (c = fontName[i]); i++) {
    if (isspace(c) || ispunct(c))
      fontName[i] = '_';
  }

  if ((err = FT_Init_FreeType(&library))) {
    fprintf(stderr, "FreeType init error: %d", err);
    return err;
  }
  // NOTE: unlike the 1-bit tool we keep FreeType's default interpreter — we WANT
  // multi-level gray coverage here.

  if ((err = FT_New_Face(library, argv[1], 0, &face))) {
    fprintf(stderr, "Font load error: %d", err);
    FT_Done_FreeType(library);
    return err;
  }
  FT_Set_Char_Size(face, (FT_F26Dot6)(size * 64.0 + 0.5), 0, DPI, 0);

  printf("#include \"fonts/aa_font.h\"\n\n");
  printf("const uint8_t %sBitmaps[] PROGMEM = {\n  ", fontName);

  for (i = first, j = 0; i <= last; i++, j++) {
    if ((err = FT_Load_Char(face, i, FT_LOAD_TARGET_NORMAL))) {
      fprintf(stderr, "Error %d loading char '%c'\n", err, i);
      continue;
    }
    if ((err = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL))) {
      fprintf(stderr, "Error %d rendering char '%c'\n", err, i);
      continue;
    }
    if ((err = FT_Get_Glyph(face->glyph, &glyph))) {
      fprintf(stderr, "Error %d getting glyph '%c'\n", err, i);
      continue;
    }

    bitmap = &face->glyph->bitmap;
    g = (FT_BitmapGlyphRec *)glyph;

    table[j].bitmapOffset = bitmapOffset;
    table[j].width = bitmap->width;
    table[j].height = bitmap->rows;
    table[j].xAdvance = face->glyph->advance.x >> 6;
    table[j].xOffset = g->left;
    table[j].yOffset = 1 - g->top;

    for (y = 0; y < (int)bitmap->rows; y++) {
      for (x = 0; x < (int)bitmap->width; x++) {
        uint8_t v = bitmap->buffer[y * bitmap->pitch + x]; // 8-bit coverage
        ennib((uint8_t)(v >> 4));                          // -> 4-bit alpha
      }
    }
    // Pad to a whole byte (even pixel count) so the next glyph starts aligned.
    int px = bitmap->width * bitmap->rows;
    if (px & 1)
      ennib(0);
    bitmapOffset += (px + 1) / 2;

    FT_Done_Glyph(glyph);
  }

  printf(" };\n\n");

  printf("const AAglyph %sGlyphs[] PROGMEM = {\n", fontName);
  for (i = first, j = 0; i <= last; i++, j++) {
    printf("  { %5d, %3d, %3d, %3d, %4d, %4d }", table[j].bitmapOffset,
           table[j].width, table[j].height, table[j].xAdvance, table[j].xOffset,
           table[j].yOffset);
    if (i < last) {
      printf(",   // 0x%02X", i);
      if ((i >= ' ') && (i <= '~'))
        printf(" '%c'", i);
      putchar('\n');
    }
  }
  printf(" }; // 0x%02X", last);
  if ((last >= ' ') && (last <= '~'))
    printf(" '%c'", last);
  printf("\n\n");

  printf("const AAfont %s PROGMEM = {\n", fontName);
  printf("  (uint8_t  *)%sBitmaps,\n", fontName);
  printf("  (AAglyph *)%sGlyphs,\n", fontName);
  if (face->size->metrics.height == 0) {
    printf("  0x%02X, 0x%02X, %d };\n\n", first, last, table[0].height);
  } else {
    printf("  0x%02X, 0x%02X, %ld };\n\n", first, last,
           face->size->metrics.height >> 6);
  }
  printf("// Approx. %d bytes\n", bitmapOffset + (last - first + 1) * 7 + 7);

  FT_Done_FreeType(library);
  return 0;
}

#endif /* !ARDUINO */
