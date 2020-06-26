#ifndef __WASM_PERF_TOOLS_H__
#define __WASM_PERF_TOOLS_H__

#include <stdint.h>

#ifdef __EMSCRIPTEN__
#  include <emscripten.h>
#else // __EMSCRIPTEN__
#  define EMSCRIPTEN_KEEPALIVE
#endif // __EMSCRIPTEN__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
  extern void EMSCRIPTEN_KEEPALIVE wasm_perf_mark_event(const char* event);
  extern void EMSCRIPTEN_KEEPALIVE wasm_perf_mark_start(const char* event, uint64_t reference);
  extern void EMSCRIPTEN_KEEPALIVE wasm_perf_mark_stop(const char* event, uint64_t reference);
  extern void EMSCRIPTEN_KEEPALIVE wasm_perf_record_progress(const char* work_item, float progress);
  extern void EMSCRIPTEN_KEEPALIVE wasm_perf_record_relative_progress(const char* work_item, float relative_progress);
  extern void EMSCRIPTEN_KEEPALIVE wasm_perf_done();
#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __WASM_PERF_TOOLS_H__
