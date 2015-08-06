// trace.h
#pragma once

#ifndef __CPCL_TRACE_H
#define __CPCL_TRACE_H

#include <cpcl/basic.h>
#include <cpcl/string_piece.hpp>

enum CPCL_TRACE_LEVEL {
  CPCL_TRACE_LEVEL_NONE = 0,
  CPCL_TRACE_LEVEL_INFO = 2,
  CPCL_TRACE_LEVEL_DEBUG = 1,
  CPCL_TRACE_LEVEL_ERROR = 8,
  CPCL_TRACE_LEVEL_WARNING = 4
};

namespace cpcl {

extern int TRACE_LEVEL;
extern FilePathChar const *TRACE_FILE_PATH;
void SetTraceFilePath(BasicStringPiece<FilePathChar> const &v);

#if defined(_MSC_VER)
void __cdecl Trace(CPCL_TRACE_LEVEL trace_level, char const *format, ...);
#else
void Trace(CPCL_TRACE_LEVEL trace_level, char const *format, ...);
#endif
void TraceString(CPCL_TRACE_LEVEL trace_level, StringPiece const &s);
inline void Debug(StringPiece const &s) {
  if (TRACE_LEVEL & CPCL_TRACE_LEVEL_DEBUG)
    TraceString(CPCL_TRACE_LEVEL_DEBUG, s);
}
inline void Warning(StringPiece const &s) {
  if (TRACE_LEVEL & CPCL_TRACE_LEVEL_WARNING)
    TraceString(CPCL_TRACE_LEVEL_WARNING, s);
}
inline void Error(StringPiece const &s) {
  if (TRACE_LEVEL & CPCL_TRACE_LEVEL_ERROR)
    TraceString(CPCL_TRACE_LEVEL_ERROR, s);
}

#if defined(_MSC_VER)
void __cdecl ErrorSystem(unsigned long error_code, char const *format, ...);
std::string GetSystemMessage(unsigned long error_code);
#else
void ErrorSystem(int error_code, char const *format, ...);
std::string GetSystemMessage(int error_code);
#endif

} // namespace cpcl

#endif // __CPCL_TRACE_H
