#include "basic.h"

#include <stdlib.h>
#include <stdexcept>

#include "dumbassert.h"
#include "trace.h"

namespace cpcl {

static void _DefaultAssertHandler(char const *s, char const *file, unsigned int line) {
  cpcl::Trace(CPCL_TRACE_LEVEL_ERROR, "%s at %s:%d epic fail, process terminates", s, file, line);
  
  ::exit(EXIT_FAILURE);
  /* ::_exit(1);
  // The function _exit() terminates the calling process "immediately".  Any
  // open file descriptors belonging to the process are closed; any children of
  // the process are inherited by process 1, init, and the process's parent is
  // sent a SIGCHLD signal. */
}

static AssertHandlerPtr ASSERT_HANDLER = _DefaultAssertHandler;

void SetAssertHandler(AssertHandlerPtr v) {
  ASSERT_HANDLER = v;
}

void dumbassert(char const *s, char const *file, unsigned int line) {
  if (ASSERT_HANDLER)
    ASSERT_HANDLER(s, file, line);
  else
    throw std::runtime_error(s);
}

} // namespace cpcl
