#include "wasm_perf.h"
#include "benchmark.h"


namespace {


wasm::perf::Benchmark benchmark;


} // namespace


extern "C" {
  void wasm_perf_mark_event(const char* event_id) {
    benchmark.getEventRecorder().submit(benchmark.getTimeStamp(), event_id);
  }

  void wasm_perf_mark_start(const char* interval_id, uint64_t reference) {
    benchmark.getIntervalRecorder(interval_id).submitStart(benchmark.getTimeStamp(), reference);
  }

  void wasm_perf_mark_stop(const char* interval_id, uint64_t reference) {
    benchmark.getIntervalRecorder(interval_id).submitStop(benchmark.getTimeStamp(), reference);
  }

  void wasm_perf_record_progress(const char* work_item, float progress) {
    benchmark.getProgressRecorder(work_item).submitAccumulatedWork(benchmark.getTimeStamp(), progress);
  }

  void wasm_perf_record_relative_progress(const char* work_item, float rel_progress) {
    benchmark.getProgressRecorder(work_item).submitWorkPackage(benchmark.getTimeStamp(), rel_progress);
  }

  void wasm_perf_done() {
    benchmark.submitDone();
    std::cout << benchmark;
  }
}
