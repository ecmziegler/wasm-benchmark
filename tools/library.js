mergeInto(LibraryManager.library, {
  wasm_perf_ready: () => {}
  wasm_perf_done: () => {}
  wasm_perf_mark_event: () => {},
  wasm_perf_mark_begin: () => {},
  wasm_perf_mark_end: () => {},
  wasm_perf_record_progress: () => {},
  wasm_perf_record_relative_progress: () => {},
});
