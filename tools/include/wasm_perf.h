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
  // Optionally mark the begin of the benchmark.
  extern void EMSCRIPTEN_KEEPALIVE wasm_perf_ready();
  // Optionally mark the end of the benchmark.
  extern void EMSCRIPTEN_KEEPALIVE wasm_perf_done();
  // Record a point-in-time event for reference.
  extern void EMSCRIPTEN_KEEPALIVE wasm_perf_mark_event(const char* event);
  // Mark the begin of an interval for reference.
  extern void EMSCRIPTEN_KEEPALIVE wasm_perf_mark_begin(const char* event, uint64_t reference);
  // Mark the end of an interval for reference.
  extern void EMSCRIPTEN_KEEPALIVE wasm_perf_mark_end(const char* event, uint64_t reference);
  // Record the absolute progress on a given work item. Different work items might interleave, but it may skew the result.
  extern void EMSCRIPTEN_KEEPALIVE wasm_perf_record_progress(const char* work_item, float progress);
  // Record the progress since the last record on a given work item. Different work items might interleave, but it may skew the result.
  extern void EMSCRIPTEN_KEEPALIVE wasm_perf_record_relative_progress(const char* work_item, float relative_progress);
#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __WASM_PERF_TOOLS_H__
