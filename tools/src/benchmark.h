#include "time-keeper.h"

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>


namespace wasm {
namespace perf {


class Benchmark;


class EventRecorder {
  friend std::ostream& operator<<(std::ostream& os, const Benchmark& benchmark);

  public:
    inline void submit(const size_t time_in_us, std::string&& event_id) {
      data_.emplace_back(time_in_us, event_id);
    }

  private:
    struct DataPoint {
      size_t time;
      std::string event_id;

      inline DataPoint(const size_t time, std::string&& event_id)
        : time(time), event_id(event_id) {
      }
    };

    std::vector<std::pair<size_t, std::string>> data_;
};


class IntervalRecorder {
  friend std::ostream& operator<<(std::ostream& os, const Benchmark& benchmark);

  public:
    inline void submitBegin(const size_t time_in_us, const uint64_t numeric_id) {
      open_intervals_.emplace(numeric_id, time_in_us);
    }

    inline void submitEnd(const size_t time_in_us, const uint64_t numeric_id) {
      const auto open_interval = open_intervals_.find(numeric_id);
      if (open_interval != open_intervals_.end()) {
        data_.emplace_back(open_interval->first, open_interval->second, time_in_us);
        open_intervals_.erase(open_interval);
      }
    }

  private:
    struct DataPoint {
      uint64_t numeric_id;
      size_t begin;
      size_t end;

      inline DataPoint(const uint64_t numeric_id, const size_t begin, const size_t end)
        : numeric_id(numeric_id), begin(begin), end(end) {
      }
    };

    std::vector<DataPoint> data_;
    std::unordered_map<uint64_t, size_t> open_intervals_;
};


class ProgressRecorder {
  friend std::ostream& operator<<(std::ostream& os, const Benchmark& benchmark);

  public:
    struct Analysis {
      size_t start_up_time;
      size_t warm_up_time;
      size_t effective_start_up_time;
      size_t duration;
      double initial_performance;
      double peak_performance;
    };

    inline ProgressRecorder() {
      data_.emplace_back(0, 0.0);
    }

    inline void submitAccumulatedWork(const size_t time_in_us, const double work) {
      if (work == 0.0)
        data_.back().time = time_in_us;
      else
        data_.emplace_back(time_in_us, work);
    }

    inline void submitWorkPackage(const size_t time_in_us, const double work_package) {
      if (work_package == 0.0)
        data_.back().time = time_in_us;
      else
        data_.emplace_back(time_in_us, data_.back().work + work_package);
    }

    inline void submitPerformance(const size_t time_in_us, const double performance) {
      data_.emplace_back(time_in_us, data_.back().work + performance * (time_in_us - data_.back().time));
    }

    ProgressRecorder& operator += (const ProgressRecorder& other);
    
    Analysis analyze() const;

  private:
    struct DataPoint {
      size_t time;
      double work;
      
      inline DataPoint(const size_t time, const double work)
        : time(time), work(work) {
      }
    };

    std::chrono::steady_clock::time_point start_time_;
    std::vector<DataPoint> data_;
};


class Benchmark {
  friend std::ostream& operator<<(std::ostream& os, const Benchmark& benchmark);

  public:
    inline Benchmark()
      : done_(false) {
    }

    inline size_t getTimeStamp() const {
      return time_keeper_.getTimeStamp();
    }
    
    inline bool done() const {
      return done_;
    }

    inline void submitDone() {
      done_ = true;
    }
  
    inline EventRecorder& getEventRecorder() {
      return event_recorder_;
    }

    inline IntervalRecorder& getIntervalRecorder(const std::string& id) {
      return interval_recorders_[id];
    }
  
    inline ProgressRecorder& getProgressRecorder(const std::string& id) {
      return progress_recorders_[id];
    }

  private:
    TimeKeeper time_keeper_;
    EventRecorder event_recorder_;
    std::unordered_map<std::string, IntervalRecorder> interval_recorders_;
    std::unordered_map<std::string, ProgressRecorder> progress_recorders_;
    bool done_;
};

std::ostream& operator<<(std::ostream& os, const Benchmark& benchmark);


} // namespace perf
} // namespace wasm
