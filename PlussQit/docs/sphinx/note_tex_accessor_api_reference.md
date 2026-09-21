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
convenience wrappers around `replace_command(name, repl)` with a literal
command name baked in, 12 `tex_remove_env_<name>()` wrappers around
`remove_environment(name)`, 12 `tex_extract_env_<name>()` wrappers plus
`tex_arrays()` and 6 semantic-block wrappers (`tex_theorems()`,
`tex_lemmas()`, ...) all around what is now `extract_environment(name)` —
were deleted rather than carried over (reference pages for these three
generic methods are not yet written). `extract_environment(name)` also
gained real capability in the move: it now always returns the full closed
`TokenBatch` (tokens plus row offsets), where the old flat
`Note::tex_extract_environment(name)` silently discarded the offsets and
returned only the flat token `Note`.

`collapse_whitespace()` is intentionally absent from this accessor. Its former
implementation was exactly the root [`Note::collapse_whitespace`](api/note/collapse_whitespace.md)
operation, with no TeX-specific rule, so the duplicate spelling was removed.

Writing the `standardize_*` family surfaced a genuine bug, since fixed at the
source rather than documented as a workaround: seven internal TeX-generation
helpers (backing `standardize_vectors`, `standardize_matrices`,
`standardize_integrals`, `standardize_primes`, `simplify_fractions`,
`simplify_roots`, and the `mode >= 1` `\sqrt(...)` path of `to_latex`,
reference page not yet written) inserted a doubled
backslash into their output — `\\vec{v}` instead of `\vec{v}` — which real
LaTeX reads as a forced linebreak followed by literal text, not the intended
command. Every affected method now emits the single, TeX-correct backslash;
the fix carries no behavioral change beyond removing the extra character.

## Coverage

This reference is under active construction. Thirty-one methods have full,
compiled-and-verified teaching pages so far:

```{toctree}
:hidden:
:maxdepth: 1

api/note-tex/add_math_delimiters
api/note-tex/align_blocks
api/note-tex/balance_braces
api/note-tex/balance_math_delims
api/note-tex/bibliography_keys
api/note-tex/canonicalize_macros
api/note-tex/captions
api/note-tex/citations
api/note-tex/collapse_macros
api/note-tex/command_names
api/note-tex/commands
api/note-tex/dedup_blank_lines
api/note-tex/display_math
api/note-tex/env_names
api/note-tex/envs
api/note-tex/escape
api/note-tex/frac_slash
api/note-tex/input_tags
api/note-tex/macro_subs
api/note-tex/pm_signs
api/note-tex/sqrt_pow
api/note-tex/standardize_dots
api/note-tex/standardize_integrals
api/note-tex/standardize_limits
api/note-tex/standardize_matrices
api/note-tex/standardize_primes
api/note-tex/standardize_subscripts
api/note-tex/standardize_summations
api/note-tex/standardize_superscripts
api/note-tex/standardize_vectors
api/note-tex/to_text
```

- [`add_math_delimiters`](api/note-tex/add_math_delimiters.md): make every
  non-empty cell one complete math fragment while preserving an existing
  outer delimiter style.
- [`align_blocks`](api/note-tex/align_blocks.md): extract complete `align` and
  `align*` environments in document order with per-cell offsets.
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
- [`command_names`](api/note-tex/command_names.md): inventory lexical TeX
  control words without confusing control symbols for names.
- [`commands`](api/note-tex/commands.md): extract either bare control-word
  lexemes or complete calls with balanced attached argument groups.
- [`frac_slash`](api/note-tex/frac_slash.md) /
  [`sqrt_pow`](api/note-tex/sqrt_pow.md): translate
  balanced fraction and root constructs into explicit arithmetic text.
- [`pm_signs`](api/note-tex/pm_signs.md): render
  paired-sign control words without corrupting longer command names.
- [`dedup_blank_lines`](api/note-tex/dedup_blank_lines.md): cap vertical
  whitespace while preserving the source line-ending convention.
- [`display_math`](api/note-tex/display_math.md): extract complete display
  fragments while retaining their original delimiters and cell ownership.
- [`env_names`](api/note-tex/env_names.md) /
  [`envs`](api/note-tex/envs.md): inventory environment openers or extract
  complete begin/end regions.
- [`input_tags`](api/note-tex/input_tags.md): retain external-input targets as
  visible tags without reading the filesystem.
- [`macro_subs`](api/note-tex/macro_subs.md): substitute cell-local simple
  macros with an explicit, deterministic recursion bound.
- [`escape`](api/note-tex/escape.md) / [`to_text`](api/note-tex/to_text.md):
  protect literal text for TeX source or reduce authored TeX to readable text.
- [`standardize_vectors`](api/note-tex/standardize_vectors.md) /
  [`standardize_matrices`](api/note-tex/standardize_matrices.md): canonicalize
  vector notation and bracketed matrix environments to one spelling each.
- [`standardize_integrals`](api/note-tex/standardize_integrals.md) /
  [`standardize_summations`](api/note-tex/standardize_summations.md) /
  [`standardize_limits`](api/note-tex/standardize_limits.md): collapse
  multi-integral and summation command spellings, and strip the
  `\limits`/`\nolimits` position modifier.
- [`standardize_subscripts`](api/note-tex/standardize_subscripts.md) /
  [`standardize_superscripts`](api/note-tex/standardize_superscripts.md) /
  [`standardize_primes`](api/note-tex/standardize_primes.md): brace every
  bare `_`/`^` script and rewrite trailing `'` marks as an explicit `\prime`
  superscript.
- [`standardize_dots`](api/note-tex/standardize_dots.md): collapse ellipsis
  command spellings to `\dots`.

The remaining 69 methods on the accessor — cleanup, math-delimiter
handling, construct removal, environment extraction, macro expansion,
format conversion, structural and semantic extraction, and command/symbol
replacement — are implemented, tested, and reachable today as
`note.tex().<method>()`; their individual reference pages are still being
written.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="note_api_reference.html">
    <span class="label">Previous</span>
    <span class="title">Note API Reference</span>
  </a>
</div>
