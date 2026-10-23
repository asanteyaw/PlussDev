<!-- pluss-hand-authored -->

# Note TeX Accessor API Reference

[Back to Note API Reference](note_api_reference)

`Note::tex()` is the TeX/LaTeX-source operation boundary for Note. It keeps
generic text mechanics on `Note` while collecting operations whose meaning
depends on TeX syntax: comments, preamble, math delimiters, commands,
environments, macros, and the semantic structures (theorems, citations,
labels, sections, ...) that TeX/LaTeX source encodes.

The accessor is zero-overhead plumbing: obtaining it performs no transform
and does not enlarge `Note`. Every terminal method is still batched over all
cells and returns either a shape-preserving `Note` or a closed `TokenBatch`
containing Text tokens and Int64 offsets.

[`TokenBatch`](tokenbatch_api_reference.md) has its own method reference. Use
it to learn the flattened token Note, cumulative offsets, defined/empty
state, and borrowed-reference lifetime shared by every ragged extraction
result.

## Canonical Surface

The accessor deliberately omits duplicate spellings. Earlier flat-`Note`
`tex_*` methods that were pure aliases — 15 `tex_replace_<greek-letter>()`
convenience wrappers around `cmd_sub(name, repl)` with a literal
command name baked in, 12 `tex_remove_env_<name>()` wrappers around
`strip_env(name)`, 12 `tex_extract_env_<name>()` wrappers plus
`tex_arrays()` and 6 semantic-block wrappers (`tex_theorems()`,
`tex_lemmas()`, ...) all around what is now `env(name)` —
were deleted rather than carried over. `env(name)` also
gained real capability in the move: it now always returns the full closed
`TokenBatch` (tokens plus row offsets), where the old flat
`Note::tex_extract_environment(name)` silently discarded the offsets and
returned only the flat token `Note`.

`collapse_whitespace()` is intentionally absent from this accessor. Its former
implementation was exactly the root [`Note::collapse_whitespace`](api/note/collapse_whitespace.md)
operation, with no TeX-specific rule, so the duplicate spelling was removed.

Writing the `standardize_*` family surfaced a genuine bug, since fixed at the
source rather than documented as a workaround: seven internal TeX-generation
helpers (backing `vector_norm`, `matrix_norm`,
`int_norm`, `prime_norm`, `simplify_fractions`,
`simplify_roots`, and the `mode >= 1` `\sqrt(...)` path of `to_latex`,
inserted a doubled backslash into their output — `\\vec{v}` instead of
`\vec{v}` — which real
LaTeX reads as a forced linebreak followed by literal text, not the intended
command. Every affected method now emits the single, TeX-correct backslash;
the fix carries no behavioral change beyond removing the extra character.

## Coverage

Every method on the accessor now has a full, compiled-and-verified
page — all ninety-seven:

```{toctree}
:hidden:
:maxdepth: 1

api/note-tex/add_math_delimiters
api/note-tex/align_blocks
api/note-tex/args
api/note-tex/balance_braces
api/note-tex/balance_math_delims
api/note-tex/bibliography_keys
api/note-tex/canonicalize_macros
api/note-tex/captions
api/note-tex/citations
api/note-tex/cmd_sub
api/note-tex/collapse_macros
api/note-tex/command_names
api/note-tex/commands
api/note-tex/deaccent
api/note-tex/dedup_blank_lines
api/note-tex/display_math
api/note-tex/dots_norm
api/note-tex/env
api/note-tex/env_names
api/note-tex/envs
api/note-tex/escape
api/note-tex/figures
api/note-tex/fix_linebreaks
api/note-tex/footnotes
api/note-tex/frac_slash
api/note-tex/graphics_tags
api/note-tex/greek
api/note-tex/headings
api/note-tex/ids
api/note-tex/includes
api/note-tex/input_tags
api/note-tex/inline_math
api/note-tex/int_norm
api/note-tex/labels
api/note-tex/limit_norm
api/note-tex/line_norm
api/note-tex/macro_defs
api/note-tex/macro_subs
api/note-tex/macro_use
api/note-tex/math_blocks
api/note-tex/math_funcs
api/note-tex/math_only
api/note-tex/matrix_norm
api/note-tex/nums
api/note-tex/ops
api/note-tex/packages
api/note-tex/pm_signs
api/note-tex/prime_norm
api/note-tex/refs
api/note-tex/render
api/note-tex/sections
api/note-tex/set_macro
api/note-tex/simplify_fractions
api/note-tex/simplify_roots
api/note-tex/sqrt_pow
api/note-tex/strip_bib
api/note-tex/strip_cites
api/note-tex/strip_cmds
api/note-tex/strip_color
api/note-tex/strip_comments
api/note-tex/strip_delims
api/note-tex/strip_env
api/note-tex/strip_envs
api/note-tex/strip_figs
api/note-tex/strip_fmt
api/note-tex/strip_fonts
api/note-tex/strip_graphics
api/note-tex/strip_labels
api/note-tex/strip_links
api/note-tex/strip_lr
api/note-tex/strip_macro
api/note-tex/strip_metadata
api/note-tex/strip_pkgs
api/note-tex/strip_preamble
api/note-tex/strip_refs
api/note-tex/strip_size
api/note-tex/strip_space
api/note-tex/strip_tables
api/note-tex/sub_norm
api/note-tex/subsections
api/note-tex/subsubsections
api/note-tex/sum_norm
api/note-tex/symbols
api/note-tex/sup_norm
api/note-tex/tables
api/note-tex/tabular_blocks
api/note-tex/text_only
api/note-tex/to_html
api/note-tex/to_latex
api/note-tex/to_markdown
api/note-tex/to_text
api/note-tex/todos
api/note-tex/unescape
api/note-tex/vector_norm
api/note-tex/warnings
api/note-tex/wrap_display_math
api/note-tex/wrap_inline_math
```

- [`add_math_delimiters`](api/note-tex/add_math_delimiters.md): make every
  non-empty cell one complete math fragment while preserving an existing
  outer delimiter style.
- [`align_blocks`](api/note-tex/align_blocks.md): extract complete `align` and
  `align*` environments in document order with per-cell offsets.
- [`args`](api/note-tex/args.md): select balanced required arguments from one
  exact control word while preserving lane ownership.
- [`balance_braces`](api/note-tex/balance_braces.md) /
  [`balance_math_delims`](api/note-tex/balance_math_delims.md): repair lexical
  group and math-delimiter structure without pretending to validate TeX.
- [`bibliography_keys`](api/note-tex/bibliography_keys.md): extract individual
  citation and `bibitem` identifiers as a closed ragged result.
- [`canonicalize_macros`](api/note-tex/canonicalize_macros.md) /
  [`collapse_macros`](api/note-tex/collapse_macros.md): normalize built-in
  control words or compact source by removing local declarations.
- [`captions`](api/note-tex/captions.md) /
  [`citations`](api/note-tex/citations.md): extract semantic command arguments
  while retaining per-cell ownership.
- [`cmd_sub`](api/note-tex/cmd_sub.md): replace one exact control word with
  literal caller-provided text without consuming attached groups.
- [`command_names`](api/note-tex/command_names.md): inventory lexical TeX
  control words without confusing control symbols for names.
- [`commands`](api/note-tex/commands.md): extract either bare control-word
  lexemes or complete calls with balanced attached argument groups.
- [`deaccent`](api/note-tex/deaccent.md): unwrap exact TeX accent commands and
  fold accented UTF-8 Latin characters without damaging ordinary commands.
- [`frac_slash`](api/note-tex/frac_slash.md) /
  [`sqrt_pow`](api/note-tex/sqrt_pow.md): translate
  balanced fraction and root constructs into explicit arithmetic text.
- [`graphics_tags`](api/note-tex/graphics_tags.md): retain graphic paths as
  visible tags without filesystem access or option interpretation.
- [`pm_signs`](api/note-tex/pm_signs.md): render
  paired-sign control words without corrupting longer command names.
- [`dedup_blank_lines`](api/note-tex/dedup_blank_lines.md): cap vertical
  whitespace while preserving the source line-ending convention.
- [`display_math`](api/note-tex/display_math.md): extract complete display
  fragments while retaining their original delimiters and cell ownership.
- [`env_names`](api/note-tex/env_names.md) /
  [`envs`](api/note-tex/envs.md): inventory environment openers or extract
  complete begin/end regions.
- [`env`](api/note-tex/env.md): extract one exact environment kind with nested
  same-name regions balanced as one token.
- [`figures`](api/note-tex/figures.md): extract complete ordinary and starred
  figure regions in source order, including balanced same-name nesting.
- [`fix_linebreaks`](api/note-tex/fix_linebreaks.md): normalize physical line
  endings, remove trailing horizontal whitespace, and cap blank-line runs.
- [`footnotes`](api/note-tex/footnotes.md): extract balanced footnote bodies
  while distinguishing optional marks from authored content.
- [`headings`](api/note-tex/headings.md): build a lane-aware outline through
  an explicit structural depth rather than an accidental result-count limit.
- [`includes`](api/note-tex/includes.md): inventory input, include, and graphics
  targets without filesystem access or path rewriting.
- [`input_tags`](api/note-tex/input_tags.md): retain external-input targets as
  visible tags without reading the filesystem.
- [`line_norm`](api/note-tex/line_norm.md): canonicalize CRLF and CR line
  endings to LF without otherwise reformatting source.
- [`inline_math`](api/note-tex/inline_math.md) /
  [`math_blocks`](api/note-tex/math_blocks.md) /
  [`math_only`](api/note-tex/math_only.md): extract inline regions, retain one
  all-form region stream, or project each cell onto joined formula bodies.
- [`labels`](api/note-tex/labels.md): extract balanced cross-reference anchor
  bodies while rejecting escaped, longer, and incomplete command forms.
- [`macro_defs`](api/note-tex/macro_defs.md) /
  [`macro_use`](api/note-tex/macro_use.md): inventory complete local
  declarations or each cell's first-seen control-word vocabulary.
- [`macro_subs`](api/note-tex/macro_subs.md): substitute cell-local simple
  macros with an explicit, deterministic recursion bound.
- [`packages`](api/note-tex/packages.md): extract individual package names from
  complete package declarations while retaining per-cell ownership.
- [`refs`](api/note-tex/refs.md): extract balanced targets from the supported
  cross-reference command families.
- [`render`](api/note-tex/render.md): project common TeX notation and symbols
  into compact readable Unicode with bounded local-macro expansion.
- [`sections`](api/note-tex/sections.md): extract exact top-level section titles
  into a closed ragged result with per-cell offsets.
- [`set_macro`](api/note-tex/set_macro.md): install one canonical cell-local
  definition while replacing complete prior declarations of the same name.
- [`simplify_fractions`](api/note-tex/simplify_fractions.md) /
  [`simplify_roots`](api/note-tex/simplify_roots.md): canonicalize balanced
  fraction or root calls while retaining TeX source.
- [`strip_bib`](api/note-tex/strip_bib.md) /
  [`strip_cites`](api/note-tex/strip_cites.md): remove complete bibliography
  machinery or citation calls through balanced lexical scans.
- [`strip_cmds`](api/note-tex/strip_cmds.md): remove control words and every
  immediately attached balanced argument group.
- [`strip_color`](api/note-tex/strip_color.md): discard color styling while
  retaining visible wrapper content.
- [`strip_comments`](api/note-tex/strip_comments.md): remove unescaped comments
  while preserving LF, CRLF, and CR physical line endings.
- [`strip_delims`](api/note-tex/strip_delims.md): remove the six lexical math
  delimiter spellings while preserving escaped dollar signs and cell shape.
- [`strip_env`](api/note-tex/strip_env.md): remove one validated environment
  kind with same-name nesting balanced.
- [`strip_envs`](api/note-tex/strip_envs.md): remove every complete,
  properly nested environment region while preserving malformed source.
- [`strip_figs`](api/note-tex/strip_figs.md) /
  [`strip_graphics`](api/note-tex/strip_graphics.md): remove figure-family
  regions alone, or remove those regions together with direct graphic calls.
- [`strip_fmt`](api/note-tex/strip_fmt.md) /
  [`strip_fonts`](api/note-tex/strip_fonts.md): retain visible bodies while
  removing wrapper formatting, optionally including font declarations.
- [`strip_labels`](api/note-tex/strip_labels.md): remove complete label anchors
  without consuming escaped, longer, or malformed command text.
- [`strip_links`](api/note-tex/strip_links.md): remove destinations while
  retaining the visible labels of `href` and `hyperref` calls.
- [`strip_lr`](api/note-tex/strip_lr.md): remove exact automatic delimiter-size
  modifiers without damaging longer commands such as `leftarrow`.
- [`strip_macro`](api/note-tex/strip_macro.md): remove complete declarations of
  one validated macro name while retaining its use sites and other definitions.
- [`strip_metadata`](api/note-tex/strip_metadata.md): remove balanced title-page
  declarations and the bare title-rendering command.
- [`strip_pkgs`](api/note-tex/strip_pkgs.md): remove balanced package and
  document-class declarations while preserving malformed source.
- [`strip_preamble`](api/note-tex/strip_preamble.md): isolate the body between
  complete exact document boundaries, preserving incomplete source.
- [`strip_refs`](api/note-tex/strip_refs.md): remove complete calls from the
  supported cross-reference families through an exact balanced scan.
- [`strip_size`](api/note-tex/strip_size.md): remove exact `big`-family sizing
  commands while retaining delimiters and their contents.
- [`strip_space`](api/note-tex/strip_space.md): remove exact explicit-spacing
  controls while preserving malformed or escaped syntax.
- [`strip_tables`](api/note-tex/strip_tables.md): remove complete balanced table
  structures without changing the receiver's shape.
- [`escape`](api/note-tex/escape.md) / [`to_text`](api/note-tex/to_text.md):
  protect literal text for TeX source or reduce authored TeX to readable text.
- [`vector_norm`](api/note-tex/vector_norm.md) /
  [`matrix_norm`](api/note-tex/matrix_norm.md): canonicalize
  vector notation and bracketed matrix environments to one spelling each.
- [`int_norm`](api/note-tex/int_norm.md) /
  [`sum_norm`](api/note-tex/sum_norm.md) /
  [`limit_norm`](api/note-tex/limit_norm.md): collapse
  multi-integral and summation command spellings, and strip the
  `\limits`/`\nolimits` position modifier.
- [`sub_norm`](api/note-tex/sub_norm.md) /
  [`sup_norm`](api/note-tex/sup_norm.md) /
  [`prime_norm`](api/note-tex/prime_norm.md): brace every
  bare `_`/`^` script and rewrite trailing `'` marks as an explicit `\prime`
  superscript.
- [`dots_norm`](api/note-tex/dots_norm.md): collapse ellipsis
  command spellings to `\dots`.
- [`math_funcs`](api/note-tex/math_funcs.md) /
  [`greek`](api/note-tex/greek.md): classify recognized function and lowercase
  Greek control words inside complete math regions.
- [`ids`](api/note-tex/ids.md) / [`nums`](api/note-tex/nums.md) /
  [`ops`](api/note-tex/ops.md) / [`symbols`](api/note-tex/symbols.md): extract
  complementary lexical views of authored mathematics.
- [`subsections`](api/note-tex/subsections.md) /
  [`subsubsections`](api/note-tex/subsubsections.md): extract heading titles
  one and two levels below [`sections`](api/note-tex/sections.md).
- [`tables`](api/note-tex/tables.md) / [`tabular_blocks`](api/note-tex/tabular_blocks.md):
  extract complete floating-table and row/column environments, starred forms
  included.
- [`text_only`](api/note-tex/text_only.md): the exact prose-preserving mirror
  of [`math_only`](api/note-tex/math_only.md).
- [`to_html`](api/note-tex/to_html.md) / [`to_markdown`](api/note-tex/to_markdown.md):
  convert a recognized TeX subset to HTML or Markdown markup; `to_markdown`
  additionally takes a case-folding `mode` that `to_html` has no
  counterpart for.
- [`to_latex`](api/note-tex/to_latex.md): build TeX source up from plain
  text, the inverse direction from `to_text`.
- [`todos`](api/note-tex/todos.md) / [`warnings`](api/note-tex/warnings.md):
  extract authoring notes or embedded compiler diagnostics.
- [`unescape`](api/note-tex/unescape.md): the exact inverse of `escape`.
- [`wrap_inline_math`](api/note-tex/wrap_inline_math.md) /
  [`wrap_display_math`](api/note-tex/wrap_display_math.md): add math
  delimiters explicitly, with different existing-delimiter handling between
  the two.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="note_api_reference.html">
    <span class="label">Previous</span>
    <span class="title">Note API Reference</span>
  </a>
</div>
