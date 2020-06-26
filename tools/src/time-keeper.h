#include <chrono>

namespace wasm {
namespace perf {

class TimeKeeper {
  public:
    inline TimeKeeper ()
      : start_time_(std::chrono::steady_clock::now()) {
    }

    inline size_t getTimeStamp () const {
      std::chrono::steady_clock::duration duration = std::chrono::steady_clock::now() - start_time_;
      return static_cast<size_t>(std::chrono::duration_cast<std::chrono::microseconds>(duration).count());
    }

  private:
    std::chrono::steady_clock::time_point start_time_;
};

} // namespace perf
} // namespace wasm
