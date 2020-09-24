/*
 * Copyright 2016 The Emscripten Authors.  All rights reserved.
 * Emscripten is available under two separate licenses, the MIT license and the
 * University of Illinois/NCSA Open Source License.  Both these licenses can be
 * found in the LICENSE file.
 */

// From the very useful lzbench project, https://github.com/inikep/lzbench

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "Alloc.h"
#include "LzmaDec.h"
#include "LzmaEnc.h"

#include "wasm_perf.h"

static void *SzAlloc(void *p, size_t size) { p = p; return MyAlloc(size); }
static void SzFree(void *p, void *address) { p = p; MyFree(address); }
static ISzAlloc g_Alloc = { SzAlloc, SzFree };

int64_t __attribute__ ((noinline)) lzbench_lzma_compress(char *inbuf, size_t insize, char *outbuf, size_t outsize, size_t level, size_t x, char* y)
{
	CLzmaEncProps props;
	int res;
    size_t headerSize = LZMA_PROPS_SIZE;
	SizeT out_len = outsize - LZMA_PROPS_SIZE;
	
	LzmaEncProps_Init(&props);
	props.level = level;
	LzmaEncProps_Normalize(&props);
  /*
  p->level = 5;
  p->dictSize = p->mc = 0;
  p->reduceSize = (UInt64)(Int64)-1;
  p->lc = p->lp = p->pb = p->algo = p->fb = p->btMode = p->numHashBytes = p->numThreads = -1;
  p->writeEndMark = 0;
  */
  
  	res = LzmaEncode((uint8_t*)outbuf+LZMA_PROPS_SIZE, &out_len, (uint8_t*)inbuf, insize, &props, (uint8_t*)outbuf, &headerSize, 0/*int writeEndMark*/, NULL, &g_Alloc, &g_Alloc);
	if (res != SZ_OK) return 0;
	
//	printf("out_len=%u LZMA_PROPS_SIZE=%d headerSize=%d\n", (int)(out_len + LZMA_PROPS_SIZE), LZMA_PROPS_SIZE, (int)headerSize);
	return LZMA_PROPS_SIZE + out_len;
}

int64_t __attribute__ ((noinline)) lzbench_lzma_decompress(char *inbuf, size_t insize, char *outbuf, size_t outsize, size_t x, size_t y, char* z)
{
	int res;
	SizeT out_len = outsize;
	SizeT src_len = insize - LZMA_PROPS_SIZE;
	ELzmaStatus status;
	
//	SRes LzmaDecode(Byte *dest, SizeT *destLen, const Byte *src, SizeT *srcLen, const Byte *propData, unsigned propSize, ELzmaFinishMode finishMode, ELzmaStatus *status, ISzAlloc *alloc)
	res = LzmaDecode((uint8_t*)outbuf, &out_len, (uint8_t*)inbuf+LZMA_PROPS_SIZE, &src_len, (uint8_t*)inbuf, LZMA_PROPS_SIZE, LZMA_FINISH_END, &status, &g_Alloc);
	if (res != SZ_OK) return 0;
	
//	printf("out_len=%u\n", (int)(out_len + LZMA_PROPS_SIZE));	
    return out_len;
}

// main part

int main(int argc, char **argv) {
  unsigned long uncompressed_size = 100000;
  int iters;
  int enable_compress = argc <= 1 || strncmp(argv[1], "compress", 9) == 0;
  int enable_decompress = argc <= 1 || strncmp(argv[1], "decompress", 11) == 0;
  int arg = argc > 2 ? argv[2][0] - '0' : 3;
  switch(arg) {
    case 0: return 0; break;
    case 1: iters = 4*1; break;
    case 2: iters = 4*10; break;
    case 3: iters = 4*22; break;
    case 4: iters = 4*125; break;
    case 5: iters = 4*225; break;
    default: printf("error: %d\\n", arg); return -1;
  }

  unsigned long maxCompressedSize = uncompressed_size * 2 + 10000; // whatever
  unsigned long compressed_size = maxCompressedSize;
  char* uncompressed_buffer = (char*)malloc(uncompressed_size);
  char* compressed_buffer = (char*)malloc(maxCompressedSize);

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
      compressed_size = lzbench_lzma_compress(uncompressed_buffer, uncompressed_size, compressed_buffer, maxCompressedSize, 4 /*level*/, 0, NULL);
    }
    wasm_perf_record_progress("compress", iters);
  } else if (enable_decompress) {
    compressed_size = lzbench_lzma_compress(uncompressed_buffer, uncompressed_size, compressed_buffer, maxCompressedSize, 4 /*level*/, 0, NULL);
  }
  printf("sizes: %d,%d\n", (int32_t)uncompressed_size, (int32_t)compressed_size);

  if (enable_decompress) {
    for (i = 0; i < iters; i++) {
      wasm_perf_record_progress("decompress", i);
      uncompressed_size = lzbench_lzma_decompress(compressed_buffer, compressed_size, uncompressed_buffer, uncompressed_size, 0, 0, NULL);
    }
    wasm_perf_record_progress("decompress", iters);
  }

  printf("ok.\n");

  return 0;
}
