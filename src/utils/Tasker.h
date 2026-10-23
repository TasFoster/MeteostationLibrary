#pragma once

#include <stdint.h>

#include "Task.h"

// Timer3, Mega 2560 at 16 MHz. One task per interrupt, in round-robin order.
class Tasker {
 public:
  // Hardware interval: 1..4 seconds; zero is treated as one second.
  Tasker(uint8_t interval, uint8_t len_buf);
  ~Tasker();

  // Tasks are borrowed and must outlive their registration.
  bool AddTask(Task& task);
  // Returns false for an interval greater than four seconds.
  bool start();
  void stop();

  static void HandleInterrupt();

  Tasker(const Tasker&) = delete;
  Tasker& operator=(const Tasker&) = delete;

 private:
  uint8_t interval_;
  uint8_t capacity_;
  uint8_t count_;
  uint8_t next_;
  Task** buf_;

  static Tasker* volatile active_;
};
