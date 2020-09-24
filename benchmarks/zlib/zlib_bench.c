/*
 * Copyright 2013 The Emscripten Authors.  All rights reserved.
 * Emscripten is available under two separate licenses, the MIT license and the
 * University of Illinois/NCSA Open Source License.  Both these licenses can be
 * found in the LICENSE file.
 */

#include "zlib.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "wasm_perf.h"


// don't inline, to be friendly to js engine osr
void __attribute__ ((noinline)) do_compress(unsigned char* uncompressed_buffer, unsigned long uncompressed_size, unsigned char* compressed_buffer, unsigned long* compressed_size) {
  compress(compressed_buffer, compressed_size, uncompressed_buffer, uncompressed_size);
}

void __attribute__ ((noinline)) do_decompress(unsigned char* compressed_buffer, unsigned long compressed_size, unsigned char* uncompressed_buffer, unsigned long* uncompressed_size) {
  uncompress(uncompressed_buffer, uncompressed_size, compressed_buffer, compressed_size);
}

int main(int argc, char **argv) {
  unsigned long uncompressed_size = 100000;
  int iters;
  int enable_compress = argc <= 1 || strncmp(argv[1], "compress", 9) == 0;
  int enable_decompress = argc <= 1 || strncmp(argv[1], "decompress", 11) == 0;
  int arg = argc > 2 ? argv[2][0] - '0' : 3;
  switch(arg) {
    case 0: return 0; break;
    case 1: iters = 60; break;
    case 2: iters = 250; break;
    case 3: iters = 500; break;
    case 4: iters = 5*500; break;
    case 5: iters = 10*500; break;
    default: printf("error: %d\\n", arg); return -1;
  }

  unsigned long maxCompressedSize = compressBound(uncompressed_size);
  unsigned long compressed_size = maxCompressedSize;
  unsigned char* uncompressed_buffer = (unsigned char*)malloc(uncompressed_size);
  unsigned char* compressed_buffer = (unsigned char*)malloc(maxCompressedSize);

  int i = 0;
  int run = 0;
  char runChar = 17;
  while (i < uncompressed_size) {
    if (run > 0) {
      run--;
    } else {
      if ((i & 7) == 0) {
        runChar = i & 7;
        run = i & 31;
      } else {
        runChar = (i*i) % 6714;
      }
    }
    uncompressed_buffer[i] = runChar;
    i++;
  }

  if (enable_compress) {
    for (i = 0; i < iters; i++) {
      wasm_perf_record_progress("compress", i);
      do_compress(uncompressed_buffer, uncompressed_size, compressed_buffer, &compressed_size);
    }
    wasm_perf_record_progress("compress", iters);
  } else if (enable_decompress) {
    do_compress(uncompressed_buffer, uncompressed_size, compressed_buffer, &compressed_size);
  }
  printf("sizes: %d,%d\n", (int)uncompressed_size, (int)compressed_size);

  if (enable_decompress) {
    for (i = 0; i < iters; i++) {
      wasm_perf_record_progress("decompress", i);
      do_decompress(compressed_buffer, compressed_size, uncompressed_buffer, &uncompressed_size);
    }
    wasm_perf_record_progress("decompress", iters);
  }

  printf("ok.\n");

  return 0;
}
