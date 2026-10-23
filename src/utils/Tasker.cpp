#include "Tasker.h"

#include <Arduino.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

namespace {
constexpr uint16_t kTimer3Compare = 62499;

static_assert(F_CPU == 16000000UL,
              "Tasker requires a 16 MHz clock");
}  // namespace

Tasker* volatile Tasker::active_ = nullptr;

Tasker::Tasker(uint8_t interval, uint8_t len_buf)
    : interval_(interval == 0 ? 1 : interval),
      capacity_(len_buf),
      count_(0),
      elapsed_(0),
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

void Tasker::start() {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    TCCR3B = 0;
    TIMSK3 = 0;

    active_ = this;
    elapsed_ = 0;

    TCCR3A = 0;
    TCCR3C = 0;
    TCNT3 = 0;
    OCR3A = kTimer3Compare;

    TIFR3 = _BV(OCF3A);
    TIMSK3 = _BV(OCIE3A);
    TCCR3B = _BV(WGM32) | _BV(CS32);
  }
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

  if (current == nullptr) {
    return;
  }

  if (++current->elapsed_ < current->interval_) {
    return;
  }

  current->elapsed_ = 0;

  const uint8_t count = current->count_;
  for (uint8_t i = 0; i < count; ++i) {
    current->buf_[i]->execute();
  }
}

ISR(TIMER3_COMPA_vect) {
  Tasker::HandleInterrupt();
}
