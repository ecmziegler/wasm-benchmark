#include "benchmark.h"

#include <cstdio> 
#include <unistd.h> 
#include <iostream>
#include <fstream>
#include <regex>
#include <exception>
#include <sys/wait.h>


namespace {


std::regex regex("^\\[WASM_PERF/([A-Z_]+)\\]\t([0-9]+)(?:\t([0-9.]+)(?:\t(.+))?)?$");

class Arguments {
  public:
    Arguments(const int arg_count, char* const args[])
      : help_(false), verbose_(false), record_runs_(false), runs_(1) {
      size_t arg_index = 1;
      for (; arg_index < arg_count; ++arg_index) {
        if (args[arg_index][0] != '-') {
          break;
        } else if (strncmp(args[arg_index], "--", 3) == 0) {
          ++arg_index;
          break;
        } else if (strncmp(args[arg_index], "--help", 7) == 0 || strncmp(args[arg_index], "-h", 3) == 0) {
          help_ = true;
        } else if (strncmp(args[arg_index], "--verbose", 10) == 0 || strncmp(args[arg_index], "-v", 3) == 0) {
          verbose_ = true;
        } else if (strncmp(args[arg_index], "--record-runs", 14) == 0 || strncmp(args[arg_index], "-R", 3) == 0) {
          record_runs_ = true;
        } else if (strncmp(args[arg_index], "-o", 3) == 0) {
          ++arg_index;
          if (arg_index < arg_count) {
            output_file_.open(args[arg_index]);
          } else {
            throw std::invalid_argument("Missing argument after -o");
          }
        } else if (strncmp(args[arg_index], "-r", 3) == 0) {
          ++arg_index;
          if (arg_index < arg_count) {
            char* end;
            long long value = strtoll(args[arg_index], &end, 0);
            if (*end != '\0' || value < 0)
              throw std::invalid_argument("Invalid argument to -r");
            else
              runs_ = value;
          } else {
            throw std::invalid_argument("Missing argument after -r");
          }
        } else {
          throw std::invalid_argument("Unexpected argument");
        }
      }
      args_.assign(args + arg_index, args + arg_count);
      args_.push_back(nullptr);
    }

    size_t size() const {
      return args_.size() - 1;
    }

    char* operator[](const size_t index) const {
      return args_[index];
    }

    operator char* const*() const {
      return args_.data();
    }

    bool help() const {
      return help_;
    }

    bool getVerbose()  const {
      return verbose_;
    }

    bool getRecordRuns() const {
      return record_runs_;
    }

    std::ostream& getOutput() {
      return output_file_.is_open() ? output_file_ : std::cout;
    }

    size_t getRuns() const {
      return runs_;
    }

  private:
    bool help_;
    bool verbose_;
    bool record_runs_;
    std::vector<char*> args_;
    std::ofstream output_file_;
    size_t runs_;
};


double parseTime(const char* time_string) {
  const char* cursor = time_string;
  char* end = nullptr;
  double time = std::strtod(cursor, &end);
  while (*end == ':') {
    cursor = end + 1;
    time *= 60.0;
    time += std::strtod(cursor, &end);
  }
  if (*end == '\0')
    return time;
  else
    throw std::invalid_argument("Could not parse time");
}


ssize_t parseOutput(wasm::perf::Benchmark& benchmark, std::FILE* const input, ssize_t time_shift_in_us, const bool verbose) {
  char* line = nullptr;
  size_t max_line_length = 0;
  ssize_t line_length = -1;
  std::cmatch match;
  size_t time_in_us;
  try {
    while ((line_length = getline(&line, &max_line_length, input)) >= 0) {
      line[line_length - 1] = '\0';  // Remove newline at end.
      if (std::regex_match(line, match, regex) && !benchmark.done()) {
        time_in_us = std::stoull(match[2].str());
        if (match[1].compare("READY") == 0) {
          time_shift_in_us -= time_in_us;
        } else {
          time_in_us += time_shift_in_us;
          if (match[1].compare("DONE") == 0) {
            benchmark.submitDone();
          } else if (match[1].compare("EVENT") == 0) {
            benchmark.getEventRecorder().submit(time_in_us, match[4].str());
          } else if (match[1].compare("BEGIN") == 0) {
            const uint64_t interval_id = std::stoull(match[3].str());
            benchmark.getIntervalRecorder(match[4].str()).submitBegin(time_in_us, interval_id);
          } else if (match[1].compare("END") == 0) {
            const uint64_t interval_id = std::stoull(match[3].str());
            benchmark.getIntervalRecorder(match[4].str()).submitEnd(time_in_us, interval_id);
          } else if (match[1].compare("PROGRESS") == 0) {
            const float progress = std::stof(match[3].str());
            benchmark.getProgressRecorder(match[4].str()).submitAccumulatedWork(time_in_us, progress);
          } else if (match[1].compare("REL_PROGRESS") == 0) {
            const float rel_progress = std::stof(match[3].str());
            benchmark.getProgressRecorder(match[4].str()).submitWorkPackage(time_in_us, rel_progress);
          } else {
            std::cerr << "Unknown perf record " << match[1] << std::endl;
          }
        }
      } else if (verbose) {
        std::cout << line << std::endl;
      }
    }
    std::free(line);
  } catch (...) {
    std::free(line);
    throw;
  }
  return time_in_us;
}


} // namespace


int main(const int argc, char* const argv[]) {
  try {
    Arguments args(argc, argv);

    // Check number of command line parameters.
    if (args.help()) {
      std::cerr << "SYNTAX - " << argv[0] << " [--verbose|-v] [--record-runs|-R] [-o <output_file>] [-r <runs>] [--] [<command> [<args> ...]]" << std::endl;
      return 0;
    }

    if (args.size() == 0) {
      // Just read from STDIN.
      wasm::perf::Benchmark benchmark;
      if (args.getRecordRuns())
        benchmark.getProgressRecorder("runs").submitAccumulatedWork(benchmark.getTimeStamp(), 0);
      parseOutput(benchmark, stdin, 0, args.getVerbose());
      if (args.getRecordRuns())
        benchmark.getProgressRecorder("runs").submitAccumulatedWork(benchmark.getTimeStamp(), 1);
      args.getOutput() << benchmark;
    } else {
      wasm::perf::Benchmark benchmark;
      if (args.getRecordRuns())
        benchmark.getProgressRecorder("runs").submitAccumulatedWork(benchmark.getTimeStamp(), 0);
      ssize_t time_shift_in_us = 0;
      for (size_t run_index = 0; run_index < args.getRuns(); ++run_index) {
        // Create a new pipe.
        int fd[2];
        if (pipe(fd) < 0) {
          std::cerr << "ERROR - Could not create pipe" << std::endl;
          return 3;
        }

        // Fork a new process.
        int pid = fork();
        if (pid == 0) {
          // This is the new process. Assign STDOUT to the pipe, close unnecessary file descriptors and then execute the given command.
          dup2(fd[1], STDOUT_FILENO);
          close(fd[0]);
          close(fd[1]);
          execvp(args[0], args);
        } else {
          // This is the parent process. Read from pipe, close unnecessary file descriptors and then keep parsing the output.
          close(fd[1]);
          std::FILE* input = fdopen(fd[0], "r");
          try {
            time_shift_in_us = parseOutput(benchmark, input, time_shift_in_us, args.getVerbose());
            int status = -1;
            waitpid(pid, &status, 0);
            if (args.getRecordRuns())
              benchmark.getProgressRecorder("runs").submitAccumulatedWork(benchmark.getTimeStamp(), run_index + 1);
            status = WEXITSTATUS(status);
            if (status != 0)
              std::cerr << "Program exited with status " << status << std::endl;
            std::fclose(input);
          } catch (...) {
            std::fclose(input);
            throw;
          }
        }
      }
      args.getOutput() << benchmark;
    }

    return 0;
  } catch (const std::invalid_argument& exception) {
    std::cerr << "ERROR - Error parsing command line: " << exception.what() << std::endl;
    return 1;
  } catch (const std::exception& exception) {
    std::cerr << "ERROR - " << exception.what() << std::endl;
    return 2;
  }
}
