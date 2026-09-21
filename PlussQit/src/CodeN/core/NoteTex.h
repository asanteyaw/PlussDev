#pragma once

#include "pluss/export.h"

#include <cstdint>
#include <string_view>

namespace pluss::coden {

class Note;
class TokenBatch;

// TeX/LaTeX-source operations bound to one Note: cleanup, math-delimiter
// handling, construct removal, environment extraction, macro expansion,
// math simplification/standardization, format conversion, structural and
// semantic extraction, and command/symbol replacement. The accessor
// borrows its receiver and is intended for an immediate chained call.
class CODEN_API NoteTexAccessor final {
public:
  NoteTexAccessor() = default;
  explicit NoteTexAccessor(const Note& owner) noexcept : owner_(&owner) {}

  Note add_math_delimiters() const;
  TokenBatch align_blocks() const;
  Note balance_braces() const;
  Note balance_math_delims() const;
  TokenBatch bibliography_keys() const;
  Note canonicalize_macros() const;
  TokenBatch captions() const;
  TokenBatch citations() const;
  Note collapse_macros() const;
  TokenBatch command_names() const;
  TokenBatch commands() const;
  TokenBatch commands(bool with_args) const;
  Note frac_slash() const;
  Note pm_signs() const;
  Note sqrt_pow() const;
  Note dedup_blank_lines() const;
  TokenBatch display_math() const;
  Note escape() const;
  TokenBatch env_names() const;
  TokenBatch envs() const;
  Note input_tags() const;
  Note macro_subs(::std::int64_t max_passes = 1) const;
  TokenBatch extract_command_args(::std::string_view cmd) const;
  // Generic environment extractor: unlike its now-removed per-name
  // predecessors, this always returns the full closed `TokenBatch` (tokens
  // plus row offsets), never just the flat token Note.
  TokenBatch extract_environment(::std::string_view name) const;
  TokenBatch extract_functions() const;
  TokenBatch extract_greek() const;
  TokenBatch extract_identifiers() const;
  TokenBatch extract_numbers() const;
  TokenBatch extract_operators() const;
  TokenBatch extract_symbols() const;
  TokenBatch figures() const;
  Note fix_linebreaks() const;
  TokenBatch footnotes() const;
  TokenBatch headings(::std::int64_t max_depth) const;
  TokenBatch includes() const;
  TokenBatch inline_math() const;
  TokenBatch labels() const;
  TokenBatch macros_defined() const;
  TokenBatch macros_used() const;
  TokenBatch math_blocks(::std::int64_t max_count) const;
  Note math_only() const;
  Note normalize_newlines() const;
  TokenBatch packages() const;
  TokenBatch refs() const;
  Note remove_accents() const;
  Note remove_bibliography() const;
  Note remove_big_delims() const;
  Note remove_citations() const;
  Note remove_color() const;
  Note remove_commands() const;
  Note remove_environment(::std::string_view name) const;
  Note remove_environments() const;
  Note remove_figures() const;
  Note remove_font_commands() const;
  Note remove_formatting() const;
  Note remove_graphics() const;
  Note remove_hyperref() const;
  Note remove_labels() const;
  Note remove_left_right() const;
  Note remove_macro(::std::string_view name) const;
  Note remove_math_delimiters() const;
  Note remove_packages() const;
  Note remove_refs() const;
  Note remove_spacing_cmds() const;
  Note remove_tables() const;
  Note remove_urls() const;
  Note render(::std::int64_t mode = 0) const;
  Note replace_command(::std::string_view cmd, ::std::string_view repl) const;
  Note resolve_graphics() const;
  Note sanitize() const;
  Note sanitize(::std::int64_t level) const;
  TokenBatch sections() const;
  Note set_macro(::std::string_view name, ::std::string_view body) const;
  Note simplify_fractions() const;
  Note simplify_roots() const;
  Note simplify_spacing() const;
  Note standardize_dots() const;
  Note standardize_integrals() const;
  Note standardize_limits() const;
  Note standardize_matrices() const;
  Note standardize_primes() const;
  Note standardize_subscripts() const;
  Note standardize_summations() const;
  Note standardize_superscripts() const;
  Note standardize_vectors() const;
  Note strip_comments() const;
  Note strip_metadata() const;
  Note strip_preamble() const;
  TokenBatch subsections() const;
  TokenBatch subsubsections() const;
  TokenBatch tables() const;
  TokenBatch tabular_blocks() const;
  Note text_only() const;
  Note to_html() const;
  Note to_html(::std::int64_t mode) const;
  Note to_latex(::std::int64_t mode = 0) const;
  Note to_markdown() const;
  Note to_markdown(::std::int64_t mode) const;
  Note to_text() const;
  Note to_text(::std::int64_t mode) const;
  TokenBatch todos() const;
  Note unescape() const;
  TokenBatch warnings() const;
  Note wrap_display_math(::std::string_view delim) const;
  Note wrap_inline_math(::std::string_view delim) const;

private:
  const Note& owner() const;
  const Note* owner_{nullptr};
};

} // namespace pluss::coden
