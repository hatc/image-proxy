// timer.h
#pragma once

#ifndef __CPCL_TIMER_H
#define __CPCL_TIMER_H

#include <cpcl/basic.h>

namespace cpcl {

struct timer {
  timer();
  
  void restart();
  /* return elapsed time in seconds */
  double elapsed() const;
private:
  int64 start_time;
  int64 counts_per_second;
}; // timer

/* return type selected for match srand() function */
unsigned int TimedSeed();

} // namespace cpcl

#endif // __CPCL_TIMER_H
