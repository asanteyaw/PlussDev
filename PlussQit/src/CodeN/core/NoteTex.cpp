#include "CodeN/core/NoteTex.h"

#include "CodeN/compat/Invoke.h"
#include "CodeN/core/Errors.h"
#include "CodeN/core/Note.h"
#include "CodeN/core/OpsInternal.h"
#include "CodeN/core/TokenBatch.h"

#include <string>
#include <cctype>

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

namespace {

bool valid_tex_name(::std::string_view name) {
  while (!name.empty() && name.front() == '\\') name.remove_prefix(1);
  if (name.empty()) return false;
  for (::std::size_t i = 0; i < name.size(); ++i) {
    const unsigned char c = static_cast<unsigned char>(name[i]);
    if (::std::isalpha(c) != 0 || name[i] == '@') continue;
    if (name[i] == '*' && i + 1 == name.size()) continue;
    return false;
  }
  return true;
}

bool valid_macro_name(::std::string_view name) {
  if (!name.empty() && name.front() == '\\') name.remove_prefix(1);
  if (name.empty()) return false;
  for (const unsigned char c : name) {
    if (::std::isalpha(c) == 0 && c != '@') return false;
  }
  return true;
}

bool valid_macro_body(::std::string_view body) {
  ::std::int64_t depth = 0;
  for (::std::size_t i = 0; i < body.size(); ++i) {
    if (body[i] == '\\' && i + 1 < body.size()) {
      ++i;
      continue;
    }
    if (body[i] == '#') return false;
    if (body[i] == '{') ++depth;
    if (body[i] == '}' && --depth < 0) return false;
  }
  return depth == 0;
}

} // namespace

TokenBatch NoteTexAccessor::args(::std::string_view cmd) const {
  PLUSS_CHECK(valid_tex_name(cmd),
              "Note.tex().args: cmd must be a TeX control-word name");
  return _compat::invoke<TokenBatch>("tex_extract_command_args", owner(), {cmd});
}

TokenBatch NoteTexAccessor::env(::std::string_view name) const {
  PLUSS_CHECK(valid_tex_name(name) && name.front() != '\\',
              "Note.tex().env: name must be a TeX environment name");
  const ::std::string op = ::std::string("tex_extract_env_") + ::std::string(name);
  return ops_internal::token_feature(owner(), op);
}

TokenBatch NoteTexAccessor::math_funcs() const {
  return _compat::invoke<TokenBatch>("tex_extract_functions", owner());
}

TokenBatch NoteTexAccessor::greek() const {
  return _compat::invoke<TokenBatch>("tex_extract_greek", owner());
}

TokenBatch NoteTexAccessor::ids() const {
  return _compat::invoke<TokenBatch>("tex_extract_identifiers", owner());
}

TokenBatch NoteTexAccessor::nums() const {
  return _compat::invoke<TokenBatch>("tex_extract_numbers", owner());
}

TokenBatch NoteTexAccessor::ops() const {
  return _compat::invoke<TokenBatch>("tex_extract_operators", owner());
}

TokenBatch NoteTexAccessor::symbols() const {
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
  PLUSS_CHECK(max_depth >= 1 && max_depth <= 4,
              "Note.tex().headings: max_depth must be in [1, 4]");
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

TokenBatch NoteTexAccessor::macro_defs() const {
  return _compat::invoke<TokenBatch>("tex_macros_defined", owner());
}

TokenBatch NoteTexAccessor::macro_use() const {
  return _compat::invoke<TokenBatch>("tex_macros_used", owner());
}

TokenBatch NoteTexAccessor::math_blocks(::std::int64_t max_count) const {
  PLUSS_CHECK(max_count >= -1,
              "Note.tex().math_blocks: max_count must be -1 or nonnegative");
  return _compat::invoke<TokenBatch>("tex_math_blocks", owner(), {max_count});
}

Note NoteTexAccessor::math_only() const {
  return _compat::invoke<Note>("tex_math_only", owner());
}

Note NoteTexAccessor::line_norm() const {
  return _compat::invoke<Note>("tex_normalize_newlines", owner());
}

TokenBatch NoteTexAccessor::packages() const {
  return _compat::invoke<TokenBatch>("tex_packages", owner());
}

TokenBatch NoteTexAccessor::refs() const {
  return _compat::invoke<TokenBatch>("tex_refs", owner());
}

Note NoteTexAccessor::deaccent() const {
  return _compat::invoke<Note>("tex_remove_accents", owner());
}

Note NoteTexAccessor::strip_bib() const {
  return _compat::invoke<Note>("tex_remove_bibliography", owner());
}

Note NoteTexAccessor::strip_size() const {
  return _compat::invoke<Note>("tex_remove_big_delims", owner());
}

Note NoteTexAccessor::strip_cites() const {
  return _compat::invoke<Note>("tex_remove_citations", owner());
}

Note NoteTexAccessor::strip_color() const {
  return _compat::invoke<Note>("tex_remove_color", owner());
}

Note NoteTexAccessor::strip_cmds() const {
  return _compat::invoke<Note>("tex_remove_commands", owner());
}

Note NoteTexAccessor::strip_env(::std::string_view name) const {
  PLUSS_CHECK(valid_tex_name(name) && name.front() != '\\',
              "Note.tex().strip_env: name must be a TeX environment name");
  return _compat::invoke<Note>("tex_remove_environment", owner(), {name});
}

Note NoteTexAccessor::strip_envs() const {
  return _compat::invoke<Note>("tex_remove_environments", owner());
}

Note NoteTexAccessor::strip_figs() const {
  return _compat::invoke<Note>("tex_remove_figures", owner());
}

Note NoteTexAccessor::strip_fonts() const {
  return _compat::invoke<Note>("tex_remove_font_commands", owner());
}

Note NoteTexAccessor::strip_fmt() const {
  return _compat::invoke<Note>("tex_remove_formatting", owner());
}

Note NoteTexAccessor::strip_graphics() const {
  return _compat::invoke<Note>("tex_remove_graphics", owner());
}

Note NoteTexAccessor::strip_links() const {
  return _compat::invoke<Note>("tex_remove_hyperref", owner());
}

Note NoteTexAccessor::strip_labels() const {
  return _compat::invoke<Note>("tex_remove_labels", owner());
}

Note NoteTexAccessor::strip_lr() const {
  return _compat::invoke<Note>("tex_remove_left_right", owner());
}

Note NoteTexAccessor::strip_macro(::std::string_view name) const {
  PLUSS_CHECK(valid_tex_name(name),
              "Note.tex().strip_macro: name must be a TeX control-word name");
  return _compat::invoke<Note>("tex_remove_macro", owner(), {name});
}

Note NoteTexAccessor::strip_delims() const {
  return _compat::invoke<Note>("tex_remove_math_delimiters", owner());
}

Note NoteTexAccessor::strip_pkgs() const {
  return _compat::invoke<Note>("tex_remove_packages", owner());
}

Note NoteTexAccessor::strip_refs() const {
  return _compat::invoke<Note>("tex_remove_refs", owner());
}

Note NoteTexAccessor::cmd_sub(::std::string_view cmd, ::std::string_view repl) const {
  PLUSS_CHECK(valid_tex_name(cmd),
              "Note.tex().cmd_sub: cmd must be a TeX control-word name");
  return _compat::invoke<Note>("tex_replace_command", owner(), {cmd, repl});
}

Note NoteTexAccessor::graphics_tags() const {
  return _compat::invoke<Note>("tex_resolve_graphics", owner());
}

// Rendering has no compatibility-dispatch entry.
Note NoteTexAccessor::render(::std::int64_t max_passes) const {
  PLUSS_CHECK(max_passes > 0,
              "Note.tex().render: max_passes must be positive");
  return ops_internal::tex_render(owner(), max_passes);
}

TokenBatch NoteTexAccessor::sections() const {
  return _compat::invoke<TokenBatch>("tex_sections", owner());
}

Note NoteTexAccessor::strip_space() const {
  return _compat::invoke<Note>("tex_remove_spacing_cmds", owner());
}

Note NoteTexAccessor::strip_tables() const {
  return _compat::invoke<Note>("tex_remove_tables", owner());
}

Note NoteTexAccessor::set_macro(::std::string_view name, ::std::string_view body) const {
  PLUSS_CHECK(valid_macro_name(name),
              "Note.tex().set_macro: name must be a TeX control-word name");
  PLUSS_CHECK(valid_macro_body(body),
              "Note.tex().set_macro: body must be zero-argument TeX with balanced braces");
  return _compat::invoke<Note>("tex_set_macro", owner(), {name, body});
}

Note NoteTexAccessor::simplify_fractions() const {
  return _compat::invoke<Note>("tex_simplify_fractions", owner());
}

Note NoteTexAccessor::simplify_roots() const {
  return _compat::invoke<Note>("tex_simplify_roots", owner());
}

Note NoteTexAccessor::dots_norm() const {
  return _compat::invoke<Note>("tex_standardize_dots", owner());
}

Note NoteTexAccessor::int_norm() const {
  return _compat::invoke<Note>("tex_standardize_integrals", owner());
}

Note NoteTexAccessor::limit_norm() const {
  return _compat::invoke<Note>("tex_standardize_limits", owner());
}

Note NoteTexAccessor::matrix_norm() const {
  return _compat::invoke<Note>("tex_standardize_matrices", owner());
}

Note NoteTexAccessor::prime_norm() const {
  return _compat::invoke<Note>("tex_standardize_primes", owner());
}

Note NoteTexAccessor::sub_norm() const {
  return _compat::invoke<Note>("tex_standardize_subscripts", owner());
}

Note NoteTexAccessor::sum_norm() const {
  return _compat::invoke<Note>("tex_standardize_summations", owner());
}

Note NoteTexAccessor::sup_norm() const {
  return _compat::invoke<Note>("tex_standardize_superscripts", owner());
}

Note NoteTexAccessor::vector_norm() const {
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

// Direct ops_internal call, matching Note::text_to_latex's own pre-existing
// implementation -- this op never went through the _compat::invoke string
// registry, so there is no "tex_to_latex" dispatch entry to reuse.
Note NoteTexAccessor::to_latex(::std::int64_t mode) const {
  if (mode < 0 || mode > 2) {
    throw Error("Note::tex().to_latex: mode must be 0, 1, or 2");
  }
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
