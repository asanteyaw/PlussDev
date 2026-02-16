#include "CodeN/core/Tokenizer.h"

#include <array>

#include "CodeN/core/OpsInternal.h"

namespace pluss::coden {

TokenBatch Tokenizer::tokenize(::std::string_view s) const {
  const ::std::array<::std::string_view, 1> flat{s};
  Note n = ops_internal::note_from_utf8(flat, {});
  return tokenize(n);
}

TokenBatch WhitespaceTokenizer::tokenize(const Note& text) const {
  return ops_internal::split_whitespace(text);
}

TokenBatch DelimTokenizer::tokenize(const Note& text) const {
  return ops_internal::split(text, delim_);
}

} // namespace pluss::coden
