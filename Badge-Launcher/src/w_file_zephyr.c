#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/kernel.h>

// Bypass doomtype.h conflict by defining __DOOMTYPE__
// This prevents the file from being included, so we must provide
// its definitions manually (compatible with Zephyr/stdbool).
#define __DOOMTYPE__

typedef bool boolean;
typedef uint8_t byte;

#define arrlen(array) (sizeof(array) / sizeof(*array))

#if defined(_WIN32) || defined(__DJGPP__)
#define DIR_SEPARATOR '\\'
#define DIR_SEPARATOR_S "\\"
#define PATH_SEPARATOR ';'
#else
#define DIR_SEPARATOR '/'
#define DIR_SEPARATOR_S "/"
#define PATH_SEPARATOR ':'
#endif

// Now include headers that would have included doomtype.h
#include "doom_wad.h" // Holds the external C array
#include "w_file.h"
#include "z_zone.h"

// Forward declaration needed if w_file.h doesn't export it
extern wad_file_class_t stdc_wad_file;

// Structure for our custom file handle
typedef struct {
  wad_file_t wad;
} zephyr_wad_file_t;

static wad_file_t *W_Zephyr_OpenFile(char *path) {
  zephyr_wad_file_t *result;
  result = Z_Malloc(sizeof(zephyr_wad_file_t), PU_STATIC, 0);
  result->wad.file_class = &stdc_wad_file;
  result->wad.mapped = (byte *)src_doomgeneric_doom1_wad;
  result->wad.length = src_doomgeneric_doom1_wad_len;
  return &result->wad;
}

static void W_Zephyr_CloseFile(wad_file_t *wad) { Z_Free(wad); }

static size_t W_Zephyr_Read(wad_file_t *wad, unsigned int offset, void *buffer,
                            size_t buffer_len) {
  if (offset > wad->length) {
    return 0;
  }
  size_t active_len = buffer_len;
  if (offset + active_len > wad->length) {
    active_len = wad->length - offset;
  }
  if (wad->mapped) {
    memcpy(buffer, wad->mapped + offset, active_len);
  } else {
    memcpy(buffer, src_doomgeneric_doom1_wad + offset, active_len);
  }
  return active_len;
}

wad_file_class_t stdc_wad_file = {
    W_Zephyr_OpenFile,
    W_Zephyr_CloseFile,
    W_Zephyr_Read,
};
