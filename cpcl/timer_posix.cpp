#include "basic.h"

#include <sys/time.h>
#include <errno.h>

#include "timer.h"
#include "trace.h"

namespace cpcl {

timer::timer() : counts_per_second(0) {
  restart();
}

void timer::restart() { // postcondition: elapsed()==0
  struct timeval t = { 0, 0 };
  if (::gettimeofday(&t, NULL) != 0)
    ErrorSystem(errno, "%s", "timer::restart(): gettimeofday(&t, NULL) fails: ");

  start_time = static_cast<int64>(t.tv_sec) * 1000 * 1000 + static_cast<int64>(t.tv_usec);
}

double timer::elapsed() const { // return elapsed time in seconds
  struct timeval t = { 0, 0 };
  if (::gettimeofday(&t, NULL) != 0)
    ErrorSystem(errno, "%s", "timer::elapsed(): gettimeofday(&t, NULL) fails: ");

  int64 v(static_cast<int64>(t.tv_sec) * 1000 * 1000 + static_cast<int64>(t.tv_usec));
  return double(v - start_time) / (1000 * 1000);
}

unsigned int TimedSeed() {
  struct timeval t = { 0, 0 };
  if (::gettimeofday(&t, NULL) != 0)
    ErrorSystem(errno, "%s", "TimedSeed(): gettimeofday(&t, NULL) fails: ");

  return (static_cast<unsigned int>(t.tv_sec) >> 20) + static_cast<unsigned int>(t.tv_usec);
}

} // namespace cpcl

#if 0

// Libraries += librt.a
#include <unistd.h>
#include <time.h>

#include <string>
#include <map>
#include <iostream>

typedef std::map<clockid_t, std::string> ClockNames;
static ClockNames clock_names;
static std::string invalid_clock_name("ClockName not found");
inline std::string const& ClockName(clockid_t clock_id) {
 if (clock_names.empty()) {
#ifdef INSERT_CLOCK_ID_NAME_PAIR
#error INSERT_CLOCK_ID_NAME_PAIR already defined
#endif
#define INSERT_CLOCK_ID_NAME_PAIR(v) clock_names.insert(std::make_pair(v, #v))
  INSERT_CLOCK_ID_NAME_PAIR(CLOCK_REALTIME);
  // INSERT_CLOCK_ID_NAME_PAIR(CLOCK_MONOTONIC);
  INSERT_CLOCK_ID_NAME_PAIR(CLOCK_PROCESS_CPUTIME_ID);
  INSERT_CLOCK_ID_NAME_PAIR(CLOCK_THREAD_CPUTIME_ID);
#undef INSERT_CLOCK_ID_NAME_PAIR
 }
 ClockNames::const_iterator i = clock_names.find(clock_id);
 if (i != clock_names.end())
  return i->second;
 else
  return invalid_clock_name;
}

inline void ClockResolution(clockid_t clk_id) {
 struct timespec r;
 if (::clock_getres(clk_id, &r) == -1) {
  std::cout << "clock_getres fails\n";
  return;
 }
 std::cout << ClockName(clk_id) << ": \n"
  "\tseconds: " << (long)r.tv_sec << std::endl <<
  "\tnanoseconds: " << (long)r.tv_nsec << std::endl;
}

int main(int, char**) {
#if defined(_POSIX_TIMERS)
 ClockResolution(CLOCK_REALTIME);
#endif
#if defined(_POSIX_MONOTONIC_CLOCK)
 ClockResolution(CLOCK_MONOTONIC);
#endif
#if defined(_POSIX_CPUTIME)
 ClockResolution(CLOCK_PROCESS_CPUTIME_ID);
#endif
#if defined(_POSIX_THREAD_CPUTIME)
 ClockResolution(CLOCK_THREAD_CPUTIME_ID);
#endif

 return 0;
}

#endif
