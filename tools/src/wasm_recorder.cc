#include "wasm_perf.h"
#include "benchmark.h"


namespace {


wasm::perf::Benchmark benchmark;
ssize_t time_shift_in_us = 0;

} // namespace


extern "C" {
  void wasm_perf_ready() {
    time_shift_in_us = -static_cast<ssize_t>(benchmark.getTimeStamp());
  }

  void wasm_perf_done() {
    benchmark.submitDone();
    std::cout << benchmark;
  }

  void wasm_perf_mark_event(const char* event_id) {
    benchmark.getEventRecorder().submit(benchmark.getTimeStamp() + time_shift_in_us, event_id);
  }

  void wasm_perf_mark_begin(const char* interval_id, uint64_t reference) {
    benchmark.getIntervalRecorder(interval_id).submitBegin(benchmark.getTimeStamp() + time_shift_in_us, reference);
  }

  void wasm_perf_mark_end(const char* interval_id, uint64_t reference) {
    benchmark.getIntervalRecorder(interval_id).submitEnd(benchmark.getTimeStamp() + time_shift_in_us, reference);
  }

  void wasm_perf_record_progress(const char* work_item, float progress) {
    benchmark.getProgressRecorder(work_item).submitAccumulatedWork(benchmark.getTimeStamp() + time_shift_in_us, progress);
  }

  void wasm_perf_record_relative_progress(const char* work_item, float rel_progress) {
    benchmark.getProgressRecorder(work_item).submitWorkPackage(benchmark.getTimeStamp() + time_shift_in_us, rel_progress);
  }
}
