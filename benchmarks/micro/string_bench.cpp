#include <string>
#include <stdlib.h>
#include "wasm_perf.h"

int xDoNotRemove = 0;

void string1M_test() {
  for (int x = 0; x < 1000000; x++) {
    std::string s = "foo";
    s += "baz";
    xDoNotRemove += s.size();
  }
}

int main(int argc, char** argv) {
  int n = argc > 1 ? atoi(argv[1]) : 0;
  wasm_perf_record_relative_progress("string", 0);
  for (int i = 0; i < n; i++) {
    string1M_test();
    wasm_perf_record_relative_progress("string", 1);
  }
}
