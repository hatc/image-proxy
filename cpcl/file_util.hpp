// file_util.hpp
#pragma once

#ifndef __CPCL_FILE_UTIL_HPP
#define __CPCL_FILE_UTIL_HPP

#include <cpcl/basic.h>
#include <cpcl/string_piece.hpp>

namespace cpcl {

template<class Char>
struct os_path_traits {
  static Char Delimiter() {
#if defined(_MSC_VER)
    return (Char)'\\';
#else
    return (Char)'/';
#endif
  }
};

/* strip directory and suffix from filenames  
 * examples:                                  
 * basename(‘/usr/bin/sort’) -> ‘sort’        
 * basename(‘/include/stdio.h .h’) -> ‘stdio’ */
template<class Char>
inline BasicStringPiece<Char> BaseName(BasicStringPiece<Char> const &s) {
  typedef os_path_traits<Char> OSPathTraits;
  if (s.empty())
    return s;

  Char const *path_head = s.data() - 1;
  Char const *i = path_head + s.size();
  Char const *path_tail = i + 1;
  for (; i != path_head; --i) {
    if (*i == (Char)'.')
      path_tail = i;
    else if (*i == OSPathTraits::Delimiter())
      break;
  }

  ++i;
  return BasicStringPiece<Char>(i, path_tail - i);
}

template<class Char>
inline BasicStringPiece<Char> BaseName(Char const *s) {
  return BaseName(BasicStringPiece<Char>(s));
}

/* strip directory and tail suffix from filenames  
 * examples:                                  
 * basefilename(‘/usr/bin/sort’) -> ‘sort’        
 * basefilename(‘/include/stdio.h .h’) -> ‘stdio.h ’
 * basefilename(‘/include/source.adl.hpp’) -> ‘source.adl’ */
template<class Char>
inline BasicStringPiece<Char> BaseFileName(BasicStringPiece<Char> const &s) {
  typedef os_path_traits<Char> OSPathTraits;
  if (s.empty())
    return s;

  Char const *path_head = s.data() - 1;
  Char const *i = path_head + s.size();
  Char const *path_tail = i + 1;
  bool strip_suffix(false);
  for (; i != path_head; --i) {
    if (!strip_suffix && (*i == (Char)'.')) {
      path_tail = i;
      strip_suffix = true;
    }
    else if (*i == OSPathTraits::Delimiter())
      break;
  }

  ++i;
  return BasicStringPiece<Char>(i, path_tail - i);
}

template<class Char>
inline BasicStringPiece<Char> BaseFileName(Char const *s) {
  return BaseFileName(BasicStringPiece<Char>(s));
}

/* strip directory prefix from filenames  
 * examples:                                  
 * basefilename(‘/usr/bin/sort’) -> ‘sort’        
 * basefilename(‘/include/stdio.h .h’) -> ‘stdio.h .h’
 * basefilename(‘/include/source.adl.hpp’) -> ‘source.adl.hpp’ */
template<class Char>
inline BasicStringPiece<Char> FileName(BasicStringPiece<Char> const &s) {
  typedef os_path_traits<Char> OSPathTraits;
  if (s.empty())
    return s;

  Char const *path_head = s.data() - 1;
  Char const *i = path_head + s.size();
  Char const *path_tail = i + 1;
  for (; i != path_head; --i) {
    if (*i == OSPathTraits::Delimiter())
      break;
  }

  ++i;
  return BasicStringPiece<Char>(i, path_tail - i);
}
template<class Char>
inline BasicStringPiece<Char> FileName(Char const *s) {
  return FileName(BasicStringPiece<Char>(s));
}

/* strip non-directory suffix from file name  
 * examples:                                  
 * dirname(‘/usr/bin/sort’) -> ‘/usr/bin’
 * dirname(‘stdio.h .h’) -> ‘’
 * ??? dirname(‘stdio.h .h’) -> ‘.’ | ‘’ ‘.’ meaning the current directory */
template<class Char>
inline BasicStringPiece<Char> DirName(BasicStringPiece<Char> const &s) {
  typedef os_path_traits<Char> OSPathTraits;
  if (s.empty())
    return s;

  Char const *path_head = s.data() - 1;
  Char const *i = path_head + s.size();
  bool t = false;
  for (; i != path_head; --i) {
    if (*i == OSPathTraits::Delimiter())
      t = true;
    else if (t)
      break;
  }
  // return (t) ? BasicStringPiece<Char>(s.data(), i - path_head) : s;
  return (t) ? BasicStringPiece<Char>(s.data(), i - path_head) : BasicStringPiece<Char>();
}
template<class Char>
inline BasicStringPiece<Char> DirName(Char const *s) {
  return DirName(BasicStringPiece<Char>(s));
}

/* return suffix from filenames
 * examples:                                  
 * fileextension(‘/usr/bin/sort’) -> ‘’
 * fileextension(‘stdio.h .h’) -> ‘h’
 */
template<class Char>
inline BasicStringPiece<Char> FileExtension(BasicStringPiece<Char> const &s) {
  if (s.empty())
    return s;

  Char const *path_head = s.data() - 1;
  Char const *i = path_head + s.size();
  Char const *path_tail = i + 1;
  bool t = false;
  for (; i != path_head; --i) {
    if (*i == (Char)'.') {
      t = true;
      break;
    }
  }
  ++i;
  return (t) ? BasicStringPiece<Char>(i, path_tail - i) : BasicStringPiece<Char>();
}
template<class Char>
inline BasicStringPiece<Char> FileExtension(Char const *s) {
  return FileExtension(BasicStringPiece<Char>(s));
}

/* where is no use cases when this function overloads can be useful - transform string literal is meaningless, just define new
template<class Char, size_t N>
inline BasicStringPiece<Char> BaseNameFromLiteral(Char const (&s)[N]) {
  return BaseName(BasicStringPiece<Char>(s, N - 1));
}
template<class Char, size_t N>
inline BasicStringPiece<Char> BaseFileNameFromLiteral(Char const (&s)[N]) {
  return BaseFileName(BasicStringPiece<Char>(s, N - 1));
}
template<class Char, size_t N>
inline BasicStringPiece<Char> DirNameFromLiteral(Char const (&s)[N]) {
  return DirName(BasicStringPiece<Char>(s, N - 1));
}
template<class Char, size_t N>
inline BasicStringPiece<Char> FileExtensionFromLiteral(Char const (&s)[N]) {
  return FileExtension(BasicStringPiece<Char>(s, N - 1));
} */

} // namespace cpcl

#endif // __CPCL_FILE_UTIL_HPP
