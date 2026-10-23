#include "Tasker.h"

#include <Arduino.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

namespace {
constexpr uint32_t kTimer3TicksPerSecond = 15625;
constexpr uint8_t kMaxIntervalSeconds = 4;
}  

Tasker* volatile Tasker::active_ = nullptr;

Tasker::Tasker(uint8_t interval, uint8_t len_buf)
    : interval_(interval == 0 ? 1 : interval),
      capacity_(len_buf),
      count_(0),
      next_(0),
      buf_(len_buf == 0 ? nullptr : new Task*[len_buf]) {
  if (buf_ == nullptr) {
    capacity_ = 0;
  }
}

Tasker::~Tasker() {
  stop();
  delete[] buf_;
}

bool Tasker::AddTask(Task& task) {
  bool added = false;

  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    if (count_ < capacity_) {
      buf_[count_++] = &task;
      added = true;
    }
  }

  return added;
}

bool Tasker::start() {
  if (interval_ > kMaxIntervalSeconds) {
    return false;
  }

  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    TCCR3B = 0;
    TIMSK3 = 0;

    active_ = this;
    next_ = 0;

    TCCR3A = 0;
    TCCR3C = 0;
    TCNT3 = 0;
    OCR3A = static_cast<uint16_t>(kTimer3TicksPerSecond * interval_ - 1);

    TIFR3 = _BV(OCF3A);
    TIMSK3 = _BV(OCIE3A);
    TCCR3B = _BV(WGM32) | _BV(CS32) | _BV(CS30);  // CTC, prescaler 1024.
  }

  return true;
}

void Tasker::stop() {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    if (active_ == this) {
      TIMSK3 = 0;
      TCCR3B = 0;
      active_ = nullptr;
    }
  }
}

void Tasker::HandleInterrupt() {
  Tasker* current = active_;

  if (current == nullptr || current->count_ == 0) {
    return;
  }

  Task* task = current->buf_[current->next_];
  if (++current->next_ >= current->count_) {
    current->next_ = 0;
  }

  task->execute();
}

ISR(TIMER3_COMPA_vect) {
  Tasker::HandleInterrupt();
}
