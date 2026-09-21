#include "CodeN/core/NoteTex.h"

#include "CodeN/compat/Invoke.h"
#include "CodeN/core/Errors.h"
#include "CodeN/core/Note.h"
#include "CodeN/core/OpsInternal.h"
#include "CodeN/core/TokenBatch.h"

#include <string>

namespace pluss::coden {

const Note& NoteTexAccessor::owner() const {
  PLUSS_CHECK(owner_ != nullptr, "Note.tex: unbound accessor");
  return *owner_;
}

Note NoteTexAccessor::add_math_delimiters() const {
  return _compat::invoke<Note>("tex_add_math_delimiters", owner());
}

TokenBatch NoteTexAccessor::align_blocks() const {
  return _compat::invoke<TokenBatch>("tex_align_blocks", owner());
}

Note NoteTexAccessor::balance_braces() const {
  return _compat::invoke<Note>("tex_balance_braces", owner());
}

Note NoteTexAccessor::balance_math_delims() const {
  return _compat::invoke<Note>("tex_balance_math_delims", owner());
}

TokenBatch NoteTexAccessor::bibliography_keys() const {
  return _compat::invoke<TokenBatch>("tex_bibliography_keys", owner());
}

Note NoteTexAccessor::canonicalize_macros() const {
  return _compat::invoke<Note>("tex_canonicalize_macros", owner());
}

TokenBatch NoteTexAccessor::captions() const {
  return _compat::invoke<TokenBatch>("tex_captions", owner());
}

TokenBatch NoteTexAccessor::citations() const {
  return _compat::invoke<TokenBatch>("tex_citations", owner());
}

Note NoteTexAccessor::collapse_macros() const {
  return _compat::invoke<Note>("tex_collapse_macros", owner());
}

TokenBatch NoteTexAccessor::command_names() const {
  return _compat::invoke<TokenBatch>("tex_command_names", owner());
}

TokenBatch NoteTexAccessor::commands() const {
  return _compat::invoke<TokenBatch>("tex_commands", owner());
}

TokenBatch NoteTexAccessor::commands(bool with_args) const {
  return _compat::invoke<TokenBatch>("tex_commands", owner(), {with_args});
}

Note NoteTexAccessor::frac_slash() const {
  return _compat::invoke<Note>("tex_convert_frac_to_slash", owner());
}

Note NoteTexAccessor::pm_signs() const {
  return _compat::invoke<Note>("tex_convert_pm_to_plusminus", owner());
}

Note NoteTexAccessor::sqrt_pow() const {
  return _compat::invoke<Note>("tex_convert_sqrt_to_pow", owner());
}

Note NoteTexAccessor::dedup_blank_lines() const {
  return _compat::invoke<Note>("tex_dedup_blank_lines", owner());
}

TokenBatch NoteTexAccessor::display_math() const {
  return _compat::invoke<TokenBatch>("tex_display_math", owner());
}

TokenBatch NoteTexAccessor::env_names() const {
  return _compat::invoke<TokenBatch>("tex_environment_names", owner());
}

TokenBatch NoteTexAccessor::envs() const {
  return _compat::invoke<TokenBatch>("tex_environments", owner());
}

Note NoteTexAccessor::escape() const {
  return _compat::invoke<Note>("tex_escape", owner());
}

Note NoteTexAccessor::input_tags() const {
  return _compat::invoke<Note>("tex_expand_inputs", owner());
}

Note NoteTexAccessor::macro_subs(::std::int64_t max_passes) const {
  PLUSS_CHECK(max_passes > 0, "Note.tex().macro_subs: max_passes must be positive");
  return _compat::invoke<Note>("tex_expand_macros", owner(), {max_passes});
}

TokenBatch NoteTexAccessor::extract_command_args(::std::string_view cmd) const {
  return _compat::invoke<TokenBatch>("tex_extract_command_args", owner(), {cmd});
}

// Generic environment extractor. Unlike the deleted per-name convenience
// methods it replaces (`extract_env_equation()`, `extract_env_array()`,
// ...), it always returns the full closed `TokenBatch` (tokens plus row
// offsets) rather than a bare `Note` of flat tokens -- calling directly
// into the same token-feature extraction the old named methods used,
// bypassing the legacy `tex_extract_environment` dispatch op (which
// discarded the offsets and returned only `batch.tokens()`).
TokenBatch NoteTexAccessor::extract_environment(::std::string_view name) const {
  const ::std::string op = ::std::string("tex_extract_env_") + ::std::string(name);
  return ops_internal::token_feature(owner(), op);
}

TokenBatch NoteTexAccessor::extract_functions() const {
  return _compat::invoke<TokenBatch>("tex_extract_functions", owner());
}

TokenBatch NoteTexAccessor::extract_greek() const {
  return _compat::invoke<TokenBatch>("tex_extract_greek", owner());
}

TokenBatch NoteTexAccessor::extract_identifiers() const {
  return _compat::invoke<TokenBatch>("tex_extract_identifiers", owner());
}

TokenBatch NoteTexAccessor::extract_numbers() const {
  return _compat::invoke<TokenBatch>("tex_extract_numbers", owner());
}

TokenBatch NoteTexAccessor::extract_operators() const {
  return _compat::invoke<TokenBatch>("tex_extract_operators", owner());
}

TokenBatch NoteTexAccessor::extract_symbols() const {
  return _compat::invoke<TokenBatch>("tex_extract_symbols", owner());
}

TokenBatch NoteTexAccessor::figures() const {
  return _compat::invoke<TokenBatch>("tex_figures", owner());
}

Note NoteTexAccessor::fix_linebreaks() const {
  return _compat::invoke<Note>("tex_fix_linebreaks", owner());
}

TokenBatch NoteTexAccessor::footnotes() const {
  return _compat::invoke<TokenBatch>("tex_footnotes", owner());
}

TokenBatch NoteTexAccessor::headings(::std::int64_t max_depth) const {
  return _compat::invoke<TokenBatch>("tex_headings", owner(), {max_depth});
}

TokenBatch NoteTexAccessor::includes() const {
  return _compat::invoke<TokenBatch>("tex_includes", owner());
}

TokenBatch NoteTexAccessor::inline_math() const {
  return _compat::invoke<TokenBatch>("tex_inline_math", owner());
}

TokenBatch NoteTexAccessor::labels() const {
  return _compat::invoke<TokenBatch>("tex_labels", owner());
}

TokenBatch NoteTexAccessor::macros_defined() const {
  return _compat::invoke<TokenBatch>("tex_macros_defined", owner());
}

TokenBatch NoteTexAccessor::macros_used() const {
  return _compat::invoke<TokenBatch>("tex_macros_used", owner());
}

TokenBatch NoteTexAccessor::math_blocks(::std::int64_t max_count) const {
  return _compat::invoke<TokenBatch>("tex_math_blocks", owner(), {max_count});
}

Note NoteTexAccessor::math_only() const {
  return _compat::invoke<Note>("tex_math_only", owner());
}

Note NoteTexAccessor::normalize_newlines() const {
  return _compat::invoke<Note>("tex_normalize_newlines", owner());
}

TokenBatch NoteTexAccessor::packages() const {
  return _compat::invoke<TokenBatch>("tex_packages", owner());
}

TokenBatch NoteTexAccessor::refs() const {
  return _compat::invoke<TokenBatch>("tex_refs", owner());
}

Note NoteTexAccessor::remove_accents() const {
  return _compat::invoke<Note>("tex_remove_accents", owner());
}

Note NoteTexAccessor::remove_bibliography() const {
  return _compat::invoke<Note>("tex_remove_bibliography", owner());
}

Note NoteTexAccessor::remove_big_delims() const {
  return _compat::invoke<Note>("tex_remove_big_delims", owner());
}

Note NoteTexAccessor::remove_citations() const {
  return _compat::invoke<Note>("tex_remove_citations", owner());
}

Note NoteTexAccessor::remove_color() const {
  return _compat::invoke<Note>("tex_remove_color", owner());
}

Note NoteTexAccessor::remove_commands() const {
  return _compat::invoke<Note>("tex_remove_commands", owner());
}

Note NoteTexAccessor::remove_environment(::std::string_view name) const {
  return _compat::invoke<Note>("tex_remove_environment", owner(), {name});
}

Note NoteTexAccessor::remove_environments() const {
  return _compat::invoke<Note>("tex_remove_environments", owner());
}

Note NoteTexAccessor::remove_figures() const {
  return _compat::invoke<Note>("tex_remove_figures", owner());
}

Note NoteTexAccessor::remove_font_commands() const {
  return _compat::invoke<Note>("tex_remove_font_commands", owner());
}

Note NoteTexAccessor::remove_formatting() const {
  return _compat::invoke<Note>("tex_remove_formatting", owner());
}

Note NoteTexAccessor::remove_graphics() const {
  return _compat::invoke<Note>("tex_remove_graphics", owner());
}

Note NoteTexAccessor::remove_hyperref() const {
  return _compat::invoke<Note>("tex_remove_hyperref", owner());
}

Note NoteTexAccessor::remove_labels() const {
  return _compat::invoke<Note>("tex_remove_labels", owner());
}

Note NoteTexAccessor::remove_left_right() const {
  return _compat::invoke<Note>("tex_remove_left_right", owner());
}

Note NoteTexAccessor::remove_macro(::std::string_view name) const {
  return _compat::invoke<Note>("tex_remove_macro", owner(), {name});
}

Note NoteTexAccessor::remove_math_delimiters() const {
  return _compat::invoke<Note>("tex_remove_math_delimiters", owner());
}

Note NoteTexAccessor::remove_packages() const {
  return _compat::invoke<Note>("tex_remove_packages", owner());
}

Note NoteTexAccessor::remove_refs() const {
  return _compat::invoke<Note>("tex_remove_refs", owner());
}

Note NoteTexAccessor::remove_spacing_cmds() const {
  return _compat::invoke<Note>("tex_remove_spacing_cmds", owner());
}

Note NoteTexAccessor::remove_tables() const {
  return _compat::invoke<Note>("tex_remove_tables", owner());
}

Note NoteTexAccessor::remove_urls() const {
  return _compat::invoke<Note>("tex_remove_urls", owner());
}

// Direct ops_internal call, matching Note::render_latex's own pre-existing
// implementation -- this op never went through the _compat::invoke string
// registry, so there is no "tex_render" dispatch entry to reuse.
Note NoteTexAccessor::render(::std::int64_t mode) const {
  return ops_internal::tex_render(owner(), mode);
}

Note NoteTexAccessor::replace_command(::std::string_view cmd, ::std::string_view repl) const {
  return _compat::invoke<Note>("tex_replace_command", owner(), {cmd, repl});
}

Note NoteTexAccessor::resolve_graphics() const {
  return _compat::invoke<Note>("tex_resolve_graphics", owner());
}

Note NoteTexAccessor::sanitize() const {
  return _compat::invoke<Note>("tex_sanitize", owner());
}

Note NoteTexAccessor::sanitize(::std::int64_t level) const {
  return _compat::invoke<Note>("tex_sanitize", owner(), {level});
}

TokenBatch NoteTexAccessor::sections() const {
  return _compat::invoke<TokenBatch>("tex_sections", owner());
}

Note NoteTexAccessor::set_macro(::std::string_view name, ::std::string_view body) const {
  return _compat::invoke<Note>("tex_set_macro", owner(), {name, body});
}

Note NoteTexAccessor::simplify_fractions() const {
  return _compat::invoke<Note>("tex_simplify_fractions", owner());
}

Note NoteTexAccessor::simplify_roots() const {
  return _compat::invoke<Note>("tex_simplify_roots", owner());
}

Note NoteTexAccessor::simplify_spacing() const {
  return _compat::invoke<Note>("tex_simplify_spacing", owner());
}

Note NoteTexAccessor::standardize_dots() const {
  return _compat::invoke<Note>("tex_standardize_dots", owner());
}

Note NoteTexAccessor::standardize_integrals() const {
  return _compat::invoke<Note>("tex_standardize_integrals", owner());
}

Note NoteTexAccessor::standardize_limits() const {
  return _compat::invoke<Note>("tex_standardize_limits", owner());
}

Note NoteTexAccessor::standardize_matrices() const {
  return _compat::invoke<Note>("tex_standardize_matrices", owner());
}

Note NoteTexAccessor::standardize_primes() const {
  return _compat::invoke<Note>("tex_standardize_primes", owner());
}

Note NoteTexAccessor::standardize_subscripts() const {
  return _compat::invoke<Note>("tex_standardize_subscripts", owner());
}

Note NoteTexAccessor::standardize_summations() const {
  return _compat::invoke<Note>("tex_standardize_summations", owner());
}

Note NoteTexAccessor::standardize_superscripts() const {
  return _compat::invoke<Note>("tex_standardize_superscripts", owner());
}

Note NoteTexAccessor::standardize_vectors() const {
  return _compat::invoke<Note>("tex_standardize_vectors", owner());
}

Note NoteTexAccessor::strip_comments() const {
  return _compat::invoke<Note>("tex_strip_comments", owner());
}

Note NoteTexAccessor::strip_metadata() const {
  return _compat::invoke<Note>("tex_strip_metadata", owner());
}

Note NoteTexAccessor::strip_preamble() const {
  return _compat::invoke<Note>("tex_strip_preamble", owner());
}

TokenBatch NoteTexAccessor::subsections() const {
  return _compat::invoke<TokenBatch>("tex_subsections", owner());
}

TokenBatch NoteTexAccessor::subsubsections() const {
  return _compat::invoke<TokenBatch>("tex_subsubsections", owner());
}

TokenBatch NoteTexAccessor::tables() const {
  return _compat::invoke<TokenBatch>("tex_tables", owner());
}

TokenBatch NoteTexAccessor::tabular_blocks() const {
  return _compat::invoke<TokenBatch>("tex_tabular_blocks", owner());
}

Note NoteTexAccessor::text_only() const {
  return _compat::invoke<Note>("tex_text_only", owner());
}

Note NoteTexAccessor::to_html() const {
  return _compat::invoke<Note>("tex_to_html", owner());
}

Note NoteTexAccessor::to_html(::std::int64_t mode) const {
  return _compat::invoke<Note>("tex_to_html", owner(), {mode});
}

// Direct ops_internal call, matching Note::text_to_latex's own pre-existing
// implementation -- this op never went through the _compat::invoke string
// registry, so there is no "tex_to_latex" dispatch entry to reuse.
Note NoteTexAccessor::to_latex(::std::int64_t mode) const {
  return ops_internal::text_to_tex(owner(), mode);
}

Note NoteTexAccessor::to_markdown() const {
  return _compat::invoke<Note>("tex_to_markdown", owner());
}

Note NoteTexAccessor::to_markdown(::std::int64_t mode) const {
  return _compat::invoke<Note>("tex_to_markdown", owner(), {mode});
}

Note NoteTexAccessor::to_text() const {
  return _compat::invoke<Note>("tex_to_text", owner());
}

Note NoteTexAccessor::to_text(::std::int64_t mode) const {
  return _compat::invoke<Note>("tex_to_text", owner(), {mode});
}

TokenBatch NoteTexAccessor::todos() const {
  return _compat::invoke<TokenBatch>("tex_todos", owner());
}

Note NoteTexAccessor::unescape() const {
  return _compat::invoke<Note>("tex_unescape", owner());
}

TokenBatch NoteTexAccessor::warnings() const {
  return _compat::invoke<TokenBatch>("tex_warnings", owner());
}

Note NoteTexAccessor::wrap_display_math(::std::string_view delim) const {
  return _compat::invoke<Note>("tex_wrap_display_math", owner(), {delim});
}

Note NoteTexAccessor::wrap_inline_math(::std::string_view delim) const {
  return _compat::invoke<Note>("tex_wrap_inline_math", owner(), {delim});
}

} // namespace pluss::coden
