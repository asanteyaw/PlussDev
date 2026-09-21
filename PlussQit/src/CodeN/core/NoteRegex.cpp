#include "CodeN/core/NoteRegex.h"

#include "CodeN/compat/Invoke.h"
#include "CodeN/core/Errors.h"
#include "CodeN/core/Note.h"
#include "CodeN/core/TokenBatch.h"

namespace pluss::coden {

const Note& NoteRegexAccessor::owner() const {
  PLUSS_CHECK(owner_ != nullptr, "Note.regex: unbound accessor");
  return *owner_;
}

Note NoteRegexAccessor::count(::std::string_view pattern) const {
  PLUSS_CHECK(owner().defined(), "regex().count: expected a defined Note");
  PLUSS_CHECK(owner().is_text() || owner().is_bytes(),
              "regex().count: expected Text or Bytes storage");
  return _compat::invoke<Note>("count_regex", owner(), {pattern});
}

TokenBatch NoteRegexAccessor::findall(::std::string_view pattern,
                                      ::std::int64_t flags) const {
  return _compat::invoke<TokenBatch>("regex_findall", owner(), {pattern, flags});
}

Note NoteRegexAccessor::match(::std::string_view pattern) const {
  return _compat::invoke<Note>("regex_match", owner(), {pattern});
}

Note NoteRegexAccessor::replace(::std::string_view pattern,
                                ::std::string_view replacement,
                                ::std::int64_t flags) const {
  return _compat::invoke<Note>("regex_replace", owner(),
                               {pattern, replacement, flags});
}

Note NoteRegexAccessor::search(::std::string_view pattern) const {
  return _compat::invoke<Note>("regex_search", owner(), {pattern});
}

TokenBatch NoteRegexAccessor::split(::std::string_view pattern,
                                    ::std::int64_t limit) const {
  return _compat::invoke<TokenBatch>("regex_split", owner(), {pattern, limit});
}

} // namespace pluss::coden
