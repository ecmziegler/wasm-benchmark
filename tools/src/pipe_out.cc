#include "wasm_perf.h"
#include "time-keeper.h"

#include <stdio.h>
#include <inttypes.h>


namespace {

wasm::perf::TimeKeeper time_keeper;

} // namespace


extern "C" {

void wasm_perf_ready() {
  printf("[WASM_PERF/READY] %zu\n", time_keeper.getTimeStamp());
  fflush(stdout);
}

void wasm_perf_done() {
  printf("[WASM_PERF/DONE] %zu\n", time_keeper.getTimeStamp());
  fflush(stdout);
}

void wasm_perf_mark_event(const char* event) {
  printf("[WASM_PERF/EVENT]\t%zu\t%s\n", time_keeper.getTimeStamp(), event);
  fflush(stdout);
}

void wasm_perf_mark_begin(const char* event, uint64_t reference) {
  printf("[WASM_PERF/BEGIN]\t%zu\t%" PRId64 "\t%s\n", time_keeper.getTimeStamp(), reference, event);
  fflush(stdout);
}

void wasm_perf_mark_end(const char* event, uint64_t reference) {
  printf("[WASM_PERF/END]\t%zu\t%" PRId64 "\t%s\n", time_keeper.getTimeStamp(), reference, event);
  fflush(stdout);
}

void wasm_perf_record_progress(const char* work_item, float progress) {
  printf("[WASM_PERF/PROGRESS]\t%zu\t%f\t%s\n", time_keeper.getTimeStamp(), progress, work_item);
  fflush(stdout);
}

void wasm_perf_record_relative_progress(const char* work_item, float relative_progress) {
  printf("[WASM_PERF/REL_PROGRESS]\t%zu\t%f\t%s\n", time_keeper.getTimeStamp(), relative_progress, work_item);
  fflush(stdout);
}

} 
