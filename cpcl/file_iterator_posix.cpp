#include "basic.h"
#include <string.h> // strnlen, memchr
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>

#include "file_iterator.h"
#include "trace.h"

namespace cpcl {

template<class Char>
struct MaskMatch {
  typedef BasicStringPiece<Char> StringPiece;
  // std::string mask_storage; - copy ctor may throw
  Char mask_storage[NAME_MAX];
  StringPiece mask;

  MaskMatch()
  {}
  // MaskMatch(StringPiece const &mask_) : mask_storage(mask_.as_string()), mask(mask_storage)
  // MaskMatch(StringPiece const &mask_) : mask(std::char_traits<Char>::copy(mask_storage, mask_.data(), mask_.size()), mask_.size())
  MaskMatch(StringPiece const &mask_) {
    size_t n((mask_.size() < NAME_MAX) ? mask_.size() : NAME_MAX); // std::min(mask_.size(), NAME_MAX);
    if (n > 0)
      mask.set(std::char_traits<Char>::copy(mask_storage, mask_.data(), n), n);
  }

  bool operator()(StringPiece const &s) const {
    if (mask.empty())
      return true;

    const_iterator mask_it = mask.begin(), mask_tail = mask.end();
    for (const_iterator it = s.begin(), tail = s.end(); mask_it != mask_tail && it != tail;) {
      // '*' - zero or more characters, i.e. '.*', not '.+'
      if ((Char)'*' == *mask_it) {
        while (mask_it != mask_tail && (Char)'*' == *mask_it)
          ++mask_it;
        if (mask_it == mask_tail)
          break; // '*' at mask tail, any string match

        it = SkipTo(*mask_it, it, tail);
        while (it != tail && !TryMatch(mask_it, mask_tail, it, tail)) {
          if (it != tail && ++it != tail) // first != used by second !=, second != needed for SkipTo precondition
            it = SkipTo(*mask_it, it, tail);
        }
      }
      else if (*it++ != *mask_it++)
        break;
    }
    while (mask_it != mask_tail && (Char)'*' == *mask_it)
      ++mask_it;

    return mask_it == mask_tail;
  }
private:
  typedef typename StringPiece::const_iterator const_iterator;

  // [precondition:head != tail] needed for find
  const_iterator SkipTo(Char c, const_iterator head, const_iterator tail) const {
    // return next char after {c} or tail if not found
    const_iterator r = std::char_traits<Char>::find(head, tail - head, c);
    return (!r) ? tail : r;
  }

  // try match [head, tail) at [mask_it, mask_tail)
  // consume mask, text(it) if match
  // try match mask until mask_it != mask_tail or *mask_it == '*'
  bool TryMatch(const_iterator &mask_it_, const_iterator mask_tail, const_iterator &it_, const_iterator tail) const {
    const_iterator mask_it = mask_it_, it = it_;
    while (mask_it != mask_tail && *mask_it != (Char)'*' && it != tail) {
      if (*mask_it++ != *it++)
        return false;
    }
    mask_it_ = mask_it; it_ = it;
    return mask_it == mask_tail;
  }
};

// use ScopedPtr with placement delete for store Handle - can't use for store, need complete type for instanation - FileIterator::Handle incomplete
struct FileIterator::Handle {
  DIR *dir;
  MaskMatch<char> mask_match;

  /*explicit Handle(DIR *dir_) : dir(dir_)
  {}*/
  Handle(DIR *dir_, StringPiece const &mask) : dir(dir_), mask_match(mask)
  {}
  ~Handle() {
    if (!!dir) {
      ::closedir(dir);
      dir = (DIR*)0;
    }
  }
};

static inline std::string& SearchPath(std::string &storage, StringPiece dir_path) {
  // search_path = search_path.trim_tail(StringPieceFromLiteral("/"));
  dir_path = dir_path.trim_tail(StringPieceFromLiteral("/"));
  storage.reserve(dir_path.size() + 1);
  storage.assign(dir_path.data(), dir_path.size());
  storage.append(1, '/');
  return storage;
}

inline StringPiece StringPieceFromEntry(struct dirent const *entry) {
  size_t const file_name_size = ::strnlen(entry->d_name, arraysize(entry->d_name));
#if defined(_BSD_SOURCE)
  if (entry->d_type == DT_DIR || entry->d_type == DT_UNKNOWN) {
#else
 {
#endif
   /*if ((file_name_size == 1 && entry->d_name[0] == '.')
    || (file_name_size == 2 && entry->d_name[0] == '.' entry->d_name[1] == '.'))*/
   // file_name_size can't be zero
   if (entry->d_name[0] == '.' && (file_name_size == 1 || (entry->d_name[1] == '.' && file_name_size == 2)))
     return cpcl::StringPiece();
 }
  return StringPiece(entry->d_name, file_name_size);
}

/* full file path i.e. /some/dir/file interpreted as directory name
 * search file path i.e. /some/dir/*.so splitted to dir_path:"/some/dir/", mask:"*.so"
 * examples:
 * "/some/dir" -> dir_path:"/some/dir/", mask:""
 * "/some/dir/" -> dir_path:"/some/dir/", mask:""
 * "/some/dir/file" -> dir_path:"/some/dir/file/", mask:""
 * "/some/dir/*.so" -> dir_path:"/some/dir/", mask:"*.so" */
FileIterator::FileIterator(StringPiece const &search_path) : handle(0), dir_path_size(0) {
  COMPILE_ASSERT(sizeof(FileIterator::Handle) <= sizeof(handle_storage), invalid_handle_storage);

  StringPiece file_name = FileName(search_path), dir_path, mask;
  // if !file_name.find('*')
  //  dir_name = search_path
  // else
  //  dir_name = DirName();
  if (!file_name.empty()) {
    if (!::memchr(file_name.data(), '*', file_name.size()))
      dir_path = search_path;
    else
      mask = file_name;
  }
  if (dir_path.empty())
    dir_path = DirName(search_path);

  DIR *dir = ::opendir(SearchPath(file_path, dir_path).c_str());
  if (!dir) {
    int error_code = errno;
    if (error_code != ENOENT)
      ErrorSystem(error_code, "FileIterator('%s'): opendir fails:", file_path.c_str());
  } else {
    handle = new (handle_storage)Handle(dir, mask);
  }
}

FileIterator::~FileIterator() {
  Dispose();
}

void FileIterator::Dispose() {
  if (!!handle) {
    handle->~Handle();
    handle = 0;
  }
}

bool FileIterator::Next(FileInfo *r) {
  if (!handle)
    return false;

  StringPiece file_name;
  struct dirent entry_storage;
  struct dirent *entry;
  if (::readdir_r(handle->dir, &entry_storage, &entry) == 0 && !!entry) {
    file_name = StringPieceFromEntry(entry);
    if (!file_name.empty()) {
      if (!handle->mask_match(file_name))
        file_name = StringPiece();
    }
    if (!file_name.empty()) {
      SetFilePath(file_name);
      if (!!r) {
        r->file_path = file_path;
        r->attributes = FileInfo::ATTRIBUTE_INVALID;

        struct stat info;
        if (::stat(file_path.c_str(), &info) == -1) {
          ErrorSystem(errno, "FileIterator::Next(): stat('%s') fails:", file_path.c_str());
        } else {
          if (!!(info.st_mode & S_IFDIR))
            r->attributes |= FileInfo::ATTRIBUTE_DIRECTORY;
          if (!!(info.st_mode & S_IFREG))
            r->attributes |= FileInfo::ATTRIBUTE_NORMAL;
        }
      }
    }

    return (file_name.empty()) ? Next(r) : true;
  } else {
    Dispose();
    return false;
  }
}

} // namespace cpcl
