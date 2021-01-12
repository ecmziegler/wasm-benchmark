#include "benchmark.h"


namespace wasm {
namespace perf {


ProgressRecorder& ProgressRecorder::operator += (const ProgressRecorder& other) {
  std::vector<DataPoint> new_data;
  new_data.reserve(data_.size() + other.data_.size());

  auto this_iterator = data_.begin();
  auto other_iterator = other.data_.begin();
  const auto this_end = data_.end();
  const auto other_end = other.data_.end();

  // Align start times.
  size_t this_time_shift = 0;
  size_t other_time_shift = 0;
  if (start_time_ <= other.start_time_) {
    other_time_shift = static_cast<size_t>(std::chrono::duration_cast<std::chrono::milliseconds>(other.start_time_ - start_time_).count());
  } else {
    this_time_shift = static_cast<size_t>(std::chrono::duration_cast<std::chrono::milliseconds>(start_time_ - other.start_time_).count());
    start_time_ = other.start_time_;
  }

  // Add up work and add time points.
  double this_work = 0.0;
  double other_work = 0.0;
  while (this_iterator != this_end && other_iterator != other_end) {
    if ((this_iterator->time + this_time_shift) < (other_iterator->time + other_time_shift)) {
      this_work = this_iterator->work;
      new_data.emplace_back(this_iterator->time + this_time_shift, this_work + other_work);
      ++this_iterator;
    }
    else if ((this_iterator->time + this_time_shift) > (other_iterator->time + other_time_shift)) {
      other_work = other_iterator->work;
      new_data.emplace_back(other_iterator->time + other_time_shift, this_work + other_work);
      ++other_iterator;
    } else {
      this_work = this_iterator->work;
      other_work = other_iterator->work;
      new_data.emplace_back(this_iterator->time + this_time_shift, this_work + other_work);
      ++this_iterator;
      ++other_iterator;
    }
  }
  for (; this_iterator != this_end; ++this_iterator)
    new_data.emplace_back(this_iterator->time + this_time_shift, this_iterator->work + other_work);
  for (; other_iterator != other_end; ++other_iterator)
    new_data.emplace_back(other_iterator->time + other_time_shift, other_iterator->work + this_work);
  
  data_ = std::move(new_data);
  return *this;
}

ProgressRecorder::Analysis ProgressRecorder::analyze() const {
  if (data_.size() < 2) {
    throw std::domain_error("No data recorded");
  } else if (data_.size() < 4) {
    return Analysis{data_[data_.size() - 2].time, 0, data_[data_.size() - 2].time, data_[data_.size() - 1].time, (data_.back().work - data_[data_.size() - 2].work) / (data_.back().time - data_[data_.size() - 2].time)};
  }
  
  // Determine start-up time.
  auto start_up_iterator = data_.begin();
  for (; start_up_iterator != data_.end(); ++start_up_iterator) {
    if (start_up_iterator->work > 0.0)
      break;
  }
  if (start_up_iterator != data_.begin())
    --start_up_iterator;

  Analysis analysis{start_up_iterator->time, 0, 0, data_.back().time, 0.0, 0.0};
  
  // Fit peak performance and determine warm-up time.
  const auto last_iterator = data_.end() - 1;
  const auto last_reliable_iterator = last_iterator - 1;  // Skip last value as it might contain additional clean-up time.
  double previous_error = std::numeric_limits<double>::max();
  for (auto warm_up_iterator = data_.begin(); warm_up_iterator != last_reliable_iterator; ++warm_up_iterator) {
    const double performance = (last_reliable_iterator->work - warm_up_iterator->work) / (last_reliable_iterator->time - warm_up_iterator->time);
    const double effective_start_up_time = warm_up_iterator->time - warm_up_iterator->work / performance;
    double error = 0.0;
    for (auto iterator = warm_up_iterator; iterator != last_iterator; ++iterator) {
      const double delta = iterator->work - performance * (iterator->time - effective_start_up_time);
      error += delta * delta;
    }
    error /= last_iterator - warm_up_iterator;
    if (error < previous_error) {
      analysis.warm_up_time = 2.0 * (last_reliable_iterator->time - analysis.start_up_time - last_reliable_iterator->work / performance);
      analysis.effective_start_up_time = effective_start_up_time;
      analysis.peak_performance = performance;
      previous_error = error;
    }
    else
      break;
  }
  
  return analysis;
}


std::ostream& operator<<(std::ostream& os, const Benchmark& benchmark) {
  os << "[EVENTS]\n";
  for (const std::pair<size_t, std::string>& data_point : benchmark.event_recorder_.data_)
    os << data_point.first << '\t' << data_point.second << '\n';

  os << "\n[INTERVALS]\n";
  for (const auto& interval_recorder : benchmark.interval_recorders_) {
    for (const IntervalRecorder::DataPoint& data_point : interval_recorder.second.data_)
      os << data_point.begin << '\t' << data_point.end << '\t' << interval_recorder.first << '\t' << data_point.numeric_id << '\n';
  }

  for (const auto& progress_recorder : benchmark.progress_recorders_) {
    os << "\n[PROGRESS " << progress_recorder.first << "]\n";

    size_t last_time_stamp = 0;
    for (const ProgressRecorder::DataPoint& data_point : progress_recorder.second.data_)
    {
      if (last_time_stamp < data_point.time)
      {
        os << data_point.time << '\t' << data_point.work << '\n';
        last_time_stamp = data_point.time;
      }
    }

    ProgressRecorder::Analysis analysis = progress_recorder.second.analyze();
    os
      << "\n{\n\t\"start_up_time\": " << analysis.start_up_time << ",\n"
      << "\t\"warm_up_time\": " << analysis.warm_up_time << ",\n"
      << "\t\"effective_start_up_time\": " << analysis.effective_start_up_time << ",\n"
      << "\t\"effective_start_up_time\": " << analysis.effective_start_up_time << ",\n"
      << "\t\"duration\": " << analysis.duration << ",\n"
      << "\t\"initial_performance\": " << analysis.initial_performance << ",\n"
      << "\t\"peak_performance\": " << analysis.peak_performance << "\n}\n";
  }

  return os;
}


} // namespace perf
} // namespace wasm
