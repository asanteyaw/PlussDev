#pragma once

#include "pluss/export.h"

#include <cstdint>
#include <string_view>

namespace pluss::coden {

class Note;
class TokenBatch;

// Regular-expression operations bound to one Note. The accessor borrows its
// receiver and is intended for an immediate chained call.
class CODEN_API NoteRegexAccessor final {
public:
  NoteRegexAccessor() = default;
  explicit NoteRegexAccessor(const Note& owner) noexcept : owner_(&owner) {}

  Note count(::std::string_view pattern) const;
  TokenBatch findall(::std::string_view pattern, ::std::int64_t flags = 0) const;
  Note match(::std::string_view pattern) const;
  Note replace(::std::string_view pattern, ::std::string_view replacement,
               ::std::int64_t flags = 0) const;
  Note search(::std::string_view pattern) const;
  TokenBatch split(::std::string_view pattern, ::std::int64_t limit = -1) const;

private:
  const Note& owner() const;
  const Note* owner_{nullptr};
};

} // namespace pluss::coden
