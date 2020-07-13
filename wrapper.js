"use strict";

var global_recorder;
var global_instance;

function generate_glue_code(function_name) {
  const exported_function = global_recorder['_wasm_perf_' + function_name];
  return function (...args) {
    let length = 0;
    while (global_instance.HEAP8[args[0] + length] != 0)
      ++length;
    const dst_ptr = global_recorder.stackAlloc(length + 1);
    for (let pos = 0; pos <= length; ++pos)
      global_recorder.HEAP8[dst_ptr + pos] = global_instance.HEAP8[args[0] + pos];
    args[0] = dst_ptr;
    exported_function.apply(null, args);
  }
}

var _wasm_perf_ready;
var _wasm_perf_done;
var _wasm_perf_mark_event;
var _wasm_perf_mark_begin;
var _wasm_perf_mark_end;
var _wasm_perf_record_progress;
var _wasm_perf_record_relative_progress;


Promise.all([
  import(recorder_js).then(({default: recorder}) =>
    recorder({
      print: print,
      locateFile: (path, prefix) => recorder_js.substring(0, recorder_js.length - 4) + '.wasm',
      onAbort: status => {
        throw `Abnormal program termination with status ${status}`;
      }
    })
  ).then(recorder => {
    global_recorder = recorder;
    _wasm_perf_ready = global_recorder.__wasm_perf_ready;
    _wasm_perf_done = global_recorder.__wasm_perf_done;
    _wasm_perf_mark_event = generate_glue_code('mark_event');
    _wasm_perf_mark_begin = generate_glue_code('mark_begin');
    _wasm_perf_mark_end = generate_glue_code('mark_end');
    _wasm_perf_record_progress = generate_glue_code('record_progress');
    _wasm_perf_record_relative_progress = generate_glue_code('record_relative_progress');

    // Install monkey patched WebAssembly methods with markers.
    let wasm_compile_count = 0;
    let wasm_instantiate_count = 0;
    const wasm_compile = WebAssembly.compile;
    const wasm_instantiate = WebAssembly.instantiate;
    WebAssembly.compile = function (...args) {
      recorder.ccall('wasm_perf_mark_begin', 'void', ['string', 'int'], ['WebAssembly.compile', wasm_compile_count]);
      const result = wasm_compile.apply(this, args);
      recorder.ccall('wasm_perf_mark_end', 'void', ['string', 'int'], ['WebAssembly.compile', wasm_compile_count]);
      ++wasm_compile_count;
      return result;
    };
    WebAssembly.instantiate = function (...args) {
      recorder.ccall('wasm_perf_mark_begin', 'void', ['string', 'int'], ['WebAssembly.instantiate', wasm_instantiate_count]);
      const result = wasm_instantiate.apply(this, args);
      recorder.ccall('wasm_perf_mark_end', 'void', ['string', 'int'], ['WebAssembly.instantiate', wasm_instantiate_count]);
      ++wasm_instantiate_count;
      return result;
    }
    recorder._wasm_perf_ready();
    return recorder;
  }), import(wasm_js).then(({default: module}) =>
    module
  )
]).then(async ([recorder, module]) => {
  return module({
    locateFile: (path, prefix) => wasm_js.substring(0, wasm_js.length - 4) + '.wasm',
    print: verbose ? printErr : text => {},
    printErr: print,
    onAbort: status => {
      throw `Abnormal program termination with status ${status}`;
    },
    noInitialRun: true
  }).then(instance => [recorder, instance]);
}).then(([recorder, instance]) => {
  global_instance = instance;
  recorder.ccall('wasm_perf_mark_event', 'void', ['string'], ['instantiated']);
  recorder.ccall('wasm_perf_record_progress', 'void', ['string', 'float'], ['runs', 0]);
  for (let run = 0; run < runs; ++run) {
    instance.callMain(argv);
    recorder.ccall('wasm_perf_record_progress', 'void', ['string', 'float'], ['runs', run + 1]);
  }
  recorder._wasm_perf_done();
  quit(0);
}).catch(error => {
  printErr(`Execution failed with status ${error}`);
  quit(1);
});
