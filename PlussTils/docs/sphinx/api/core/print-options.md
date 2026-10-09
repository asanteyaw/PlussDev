# Print options: PrintOptions and its fields

[Back to Core Utilities](../core.md) · [Core Utilities API Reference](../../api_reference.md)

`PrintOptions` is the formatting spec that an `OutputEngine` call takes as its
last argument, or that is stored as the engine's baseline. Its fields are
grouped by what they affect, and each group has its own reference page.

- [`PrintOptions`](../core-print-options.md): the struct itself, its resolution
  order, and how a call's options merge with the engine's baseline.
- [Joining](../core-print-options/joining.md): `sep`, `end`, `prefix`, `suffix`,
  `line_prefix`, `flush`.
- [Layout](../core-print-options/layout.md): `align`, `width`, `fill`,
  `truncate`, `ellipsis`.
- [Numbers](../core-print-options/numbers.md): `notation`, `precision`, `sign`,
  `alt`, `uppercase`, `grouping`, `group_size`, `decimal_point`,
  `trim_trailing_zeros`, the nan/infinity text.
- [Humanized units](../core-print-options/humanize.md): `humanize`,
  `humanize_precision`.
- [Booleans and optionals](../core-print-options/booleans-and-optionals.md):
  `bool_style`, `bool_true`, `bool_false`, `null_text`, `optional_tag`.
- [Strings](../core-print-options/strings.md): `quote_strings`, `quote_char`,
  `escape_non_printable`, `case_transform`.
- [Containers](../core-print-options/containers.md): `item_sep`, `kv_sep`, the
  bracket fields, `pair_style`, `max_items`, `more_verbose`, `max_depth`,
  `sort_keys`, `show_indices`, `multiline`, `indent`, `trailing_comma`.
- [Color](../core-print-options/color.md): `color_mode`, `style`, `syntax`.
- [The format-string path](../core-print-options/format-path.md):
  `missing_arg`, `strict_args`, and `pack`.
- [Fluent builders](../core-print-options/builders.md): `with_sep`, `with_end`,
  `fixed`, `sci`, `hex`, `percent`, `pad`, `grouped`, `colored`, `quoted`,
  `upcase`, `pretty`, `capped`.
- [Presets](../core-print-options/presets.md): `plain`, `pretty_json`, `repr`,
  `table_cell`.

The enumerations and the two supporting structs each have their own page:
[`Align`](../core-align.md), [`Sign`](../core-sign.md),
[`Notation`](../core-notation.md), [`Humanize`](../core-humanize.md),
[`BoolStyle`](../core-bool-style.md), [`CaseXf`](../core-case-xf.md),
[`PairStyle`](../core-pair-style.md), [`ColorMode`](../core-color-mode.md),
[`Color`](../core-color.md), [`Style`](../core-style.md), and
[`SyntaxTheme`](../core-syntax-theme.md).

For a guide with worked programs, see
[Print options](../../manual/tils-print-options.md).

```{toctree}
:maxdepth: 1
:hidden:

../core-print-options
../core-print-options/joining
../core-print-options/layout
../core-print-options/numbers
../core-print-options/humanize
../core-print-options/booleans-and-optionals
../core-print-options/strings
../core-print-options/containers
../core-print-options/color
../core-print-options/format-path
../core-print-options/builders
../core-print-options/presets
../core-align
../core-sign
../core-notation
../core-humanize
../core-bool-style
../core-case-xf
../core-pair-style
../core-color-mode
../core-color
../core-style
../core-syntax-theme
```
