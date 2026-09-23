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
  TokenBatch args(::std::string_view cmd) const;
  TokenBatch env(::std::string_view name) const;
  TokenBatch math_funcs() const;
  TokenBatch greek() const;
  TokenBatch ids() const;
  TokenBatch nums() const;
  TokenBatch ops() const;
  TokenBatch symbols() const;
  TokenBatch figures() const;
  Note fix_linebreaks() const;
  TokenBatch footnotes() const;
  TokenBatch headings(::std::int64_t max_depth) const;
  TokenBatch includes() const;
  TokenBatch inline_math() const;
  TokenBatch labels() const;
  TokenBatch macro_defs() const;
  TokenBatch macro_use() const;
  TokenBatch math_blocks(::std::int64_t max_count) const;
  Note math_only() const;
  Note line_norm() const;
  TokenBatch packages() const;
  TokenBatch refs() const;
  Note deaccent() const;
  Note strip_bib() const;
  Note strip_size() const;
  Note strip_cites() const;
  Note strip_color() const;
  Note strip_cmds() const;
  Note strip_env(::std::string_view name) const;
  Note strip_envs() const;
  Note strip_figs() const;
  Note strip_fonts() const;
  Note strip_fmt() const;
  Note strip_graphics() const;
  Note strip_links() const;
  Note strip_labels() const;
  Note strip_lr() const;
  Note strip_macro(::std::string_view name) const;
  Note strip_delims() const;
  Note strip_pkgs() const;
  Note strip_refs() const;
  Note cmd_sub(::std::string_view cmd, ::std::string_view repl) const;
  Note graphics_tags() const;
  Note render(::std::int64_t max_passes = 1) const;
  TokenBatch sections() const;
  Note strip_space() const;
  Note strip_tables() const;
  Note set_macro(::std::string_view name, ::std::string_view body) const;
  Note simplify_fractions() const;
  Note simplify_roots() const;
  Note dots_norm() const;
  Note int_norm() const;
  Note limit_norm() const;
  Note matrix_norm() const;
  Note prime_norm() const;
  Note sub_norm() const;
  Note sum_norm() const;
  Note sup_norm() const;
  Note vector_norm() const;
  Note strip_comments() const;
  Note strip_metadata() const;
  Note strip_preamble() const;
  TokenBatch subsections() const;
  TokenBatch subsubsections() const;
  TokenBatch tables() const;
  TokenBatch tabular_blocks() const;
  Note text_only() const;
  Note to_html() const;
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
