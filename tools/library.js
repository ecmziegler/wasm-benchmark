mergeInto(LibraryManager.library, {
  wasm_perf_mark_event: () => {},
  wasm_perf_mark_start: () => {},
  wasm_perf_mark_stop: () => {},
  wasm_perf_record_progress: () => {},
  wasm_perf_record_relative_progress: () => {},
  wasm_perf_done: () => {}
});
