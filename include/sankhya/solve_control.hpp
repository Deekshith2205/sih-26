// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>

namespace sankhya {

/// Live metrics reported during a solve.
///
/// Reported approximately every 100ms to the progress callback. The `phase` field
/// identifies which part of the solve is running.
struct Progress {
  enum class Phase : std::uint8_t { kPresolve, kLp, kTree };

  Phase phase = Phase::kLp;
  int64_t iterations = 0;
  int64_t nodes = 0;
  double objective = 0.0;
  double best_bound = 0.0;
  double gap = 0.0;
  double elapsed_seconds = 0.0;
  int64_t open_nodes = 0;
};

/// A callback invoked periodically by the solver.
/// Returning a non-zero value requests an immediate interrupt of the solve.
using ProgressCallback = std::function<int(const Progress&)>;

/// Solve-scoped control object.
///
/// This object is passed down to all solver engines participating in a single solve.
/// It holds the interruption state and the progress callback. Since a solve may scale
/// or transform the model (producing a copy of `Model`), holding this state here ensures
/// all transformations share the same interruption context and `Model` itself remains
/// copyable and movable.
class SolveControl {
 public:
  /// The user-provided progress callback, invoked approximately every 100ms.
  ProgressCallback progress_callback;

  /// Request an immediate interruption of the solve.
  /// This is safe to call from another thread or a signal handler.
  void interrupt() noexcept { interrupt_requested_.store(true, std::memory_order_relaxed); }

  /// Has an interruption been requested?
  [[nodiscard]] bool interruption_requested() const noexcept {
    return interrupt_requested_.load(std::memory_order_relaxed);
  }

  /// Reset the interruption flag. Call before starting a new solve.
  void reset() noexcept {
    interrupt_requested_.store(false, std::memory_order_relaxed);
    first_callback_invoked = false;
  }

  /// Shared throttle state so nested engines don't all fire simultaneously
  bool first_callback_invoked = false;
  std::chrono::steady_clock::time_point last_callback_time;

 private:
  std::atomic<bool> interrupt_requested_{false};
};

}  // namespace sankhya
