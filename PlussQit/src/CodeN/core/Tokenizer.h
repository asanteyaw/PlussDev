#pragma once

#include <memory>
#include <string_view>

#include "CodeN/core/TokenBatch.h"
#include "CodeN/core/Note.h"

namespace pluss::coden {

// Tokenizer is an object-model hook for text segmentation.
//
// Goals:
// - Provide a stable OO surface for integrations (BPE, SentencePiece, etc.)
// - Keep the core Note/Chord ops as pure functions (ops_internal)
// - Avoid committing to a specific tokenizer algorithm early

class Tokenizer {
public:
  virtual ~Tokenizer() = default;

  // Tokenize each element of the Note; returns ragged tokens packed into a 1D Note.
  virtual TokenBatch tokenize(const Note& text) const = 0;

  // Convenience: tokenize a single string.
  TokenBatch tokenize(::std::string_view s) const;
};

// Simple built-in tokenizers.

class WhitespaceTokenizer final : public Tokenizer {
public:
  TokenBatch tokenize(const Note& text) const override;
};

class DelimTokenizer final : public Tokenizer {
public:
  explicit DelimTokenizer(std::string_view delim) : delim_(delim) {}
  TokenBatch tokenize(const Note& text) const override;

private:
  std::string delim_;
};

} // namespace pluss::coden
