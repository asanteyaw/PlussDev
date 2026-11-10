<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6873d8d9a7cb -->

# Stream::lbl

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
Stream::lbl() const -> StreamLabelAccessor
```

## What It Does

The entry point into `Stream`'s fuller row-label surface. `lbl()` binds the current stream
and hands back a `StreamLabelAccessor`:

### `StreamLabelAccessor` members

| Member | Kind or type | Signature or default |
|---|---|---|
| `values` | Method | `Stream values() const` |
| `has` | Method | `bool has() const` |
| `set` | Method | `Stream set(const Stream& labels) const` |
| `set` | Method | `Stream set(const pluss::table::Slate& labels) const` |
| `range` | Method | `Stream range() const` |
| `reset` | Method | `Stream reset() const` |
| `align` | Method | `Stream align(const Stream& target_labels, const pluss::table::AlignOptions& options = {}) const` |
| `align` | Method | `Stream align(const pluss::table::Slate& target_labels, const pluss::table::AlignOptions& options = {}) const` |

This has no equivalent on `Catalog` — `lbl()` is Stream-specific, since row labels are a
per-column (per-`Stream`) concept, not something that makes sense collection-wide.

```{important} Two related but distinct surfaces: label()/has_label() and lbl()
[`has_label()`](has_label.md)/[`label()`](label.md) are a plain getter pair for the raw
optional label `Slate` — no default-value synthesis. `lbl()` is richer: notably,
`lbl().values()` **always** returns something displayable, synthesizing a positional
`0..n-1` range when no label has actually been set — where `label().value()` would throw
`std::bad_optional_access` in the same situation. `has_label()` still reports `false` for
the synthesized case; only an explicitly set label counts as "has a label."
```

```{important} set()/align() overload resolution: prefer an explicit Slate
`pluss::make_note(...)` (and other `make_*` helpers) implicitly convert to *both* `Stream`
and `pluss::table::Slate` — passing one directly to `set()` or `align()` is ambiguous and
fails to compile. Wrap it explicitly: `set(pluss::table::Slate(pluss::make_note({...})))`.
A `Stream` value (not a bare `make_note(...)` call) resolves the `Stream` overload cleanly
with no wrapping needed.
```

## Parameters

`None` (parameters belong to the individual `StreamLabelAccessor` methods, not to `lbl()`
itself).

## Returns

`StreamLabelAccessor`
    A binder for the current stream — call one of its methods to inspect, set, reset, or
    align its row-label axis.

## Examples

Example 1. `values()` always returns a displayable `Stream` — even with no label set, it
synthesizes a positional range rather than throwing or returning empty.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

std::cout << "has(): " << stream.lbl().has() << "\n";
std::cout << stream.lbl().values();
```

### Output

```text
has(): 0
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[int64]  records=3
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 0                                                                                                        
1            | 1                                                                                                        
2            | 2                                                                                                        
========================================================================================================================
```

Example 2. `set()` attaches a label; `reset()` clears it back to the unlabeled state (note
`has()` goes back to `false`, even though `values()` would still show a synthesized range).

```cpp
using pluss::table::Slate;

auto stream = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0}));
auto labeled = stream.lbl().set(Slate(pluss::make_note({"a", "b", "c"})));
std::cout << "labeled has(): " << labeled.lbl().has() << "\n";

auto back = labeled.lbl().reset();
std::cout << "after reset has(): " << back.lbl().has();
```

### Output

```text
labeled has(): 1
after reset has(): 0
```

Example 3. `range()` keeps the bound stream's *values* unchanged and instead replaces its
*label* with the default positional range (`0..n-1`) — the inverse framing of `values()`,
which discards the values and extracts the label's content instead. Note this is **not** a
display fallback: `has()` on the result reports `true`, a real label was set.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

std::cout << stream.lbl().range();
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=3
label=yes  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 10                                                                                                       
1            | 20                                                                                                       
2            | 30                                                                                                       
========================================================================================================================
```

The `0`/`1`/`2` here is the **label** column; the `value` column (`10`/`20`/`30`) is the
original stream's own data, untouched. See
[`StreamLabelAccessor::range`](../stream-lbl/range.md) for the full breakdown, including how
`range()` overwrites an *existing custom* label too, not just an absent one.

Example 4. `align(target_labels, options)` reorders/relabels by case label under
`AlignBy::Caselbl`. An unmatched target label produces `NULL` unless `options.fill` is set
— identical rules to [`Slate::align`](../slate/align.md), applied through the label
accessor.

```cpp
using pluss::table::Slate;
using pluss::table::AlignOptions;
using pluss::table::AlignBy;

auto labeled = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}))
                   .lbl().set(Slate(pluss::make_note({"b", "a", "c"})));

auto aligned = labeled.lbl().align(
    Slate(pluss::make_note({"a", "b", "d"})),
    AlignOptions{.align = AlignBy::Caselbl});
std::cout << aligned;
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=3
label=yes  mask=yes  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
a            | 20                                                                                                       
b            | 10                                                                                                       
d            | NA                                                                                                       
========================================================================================================================
```

Example 5. Setting `options.fill` replaces the unmatched-label `NULL` with a scalar instead.

```cpp
using pluss::table::Slate;
using pluss::table::AlignOptions;
using pluss::table::AlignBy;

auto labeled = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}))
                   .lbl().set(Slate(pluss::make_note({"b", "a", "c"})));

auto aligned = labeled.lbl().align(
    Slate(pluss::make_note({"a", "b", "d"})),
    AlignOptions{.align = AlignBy::Caselbl, .fill = -1.0});
std::cout << aligned;
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=3
label=yes  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
a            | 20                                                                                                       
b            | 10                                                                                                       
d            | -1                                                                                                       
========================================================================================================================
```

Example 6. The default `AlignBy::Pos` ignores `target_labels` for relabeling entirely —
only its length is checked. The stream's own current values and labels pass through
unchanged, exactly as documented for [`Slate::align`](../slate/align.md).

```cpp
using pluss::table::Slate;

auto labeled = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}))
                   .lbl().set(Slate(pluss::make_note({"b", "a", "c"})));

auto aligned = labeled.lbl().align(Slate(pluss::make_note({"q", "r", "s"})));
std::cout << aligned;
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=3
label=yes  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
b            | 10                                                                                                       
a            | 20                                                                                                       
c            | 30                                                                                                       
========================================================================================================================
```

## Notes

- `lbl()` alone does no work — it only binds the stream; call one of its methods to
  actually inspect, set, reset, or align.
- `values()` and `range()` are inverse extractions, not variations of the same thing:
  `values()` discards the stream's values and returns the label's own content instead
  (synthesizing a positional range if unlabeled); `range()` keeps the stream's values
  unchanged and replaces the label with the default positional range, unconditionally —
  even overwriting an existing custom label. `has()` is the accurate existence check for
  the *original* stream; don't infer it from either method's result.
- `set()`/`align()` both overload on `Stream` vs `Slate` — see the second important
  callout above for the `make_note(...)` ambiguity gotcha.
- `align()` follows exactly the same `AlignBy`/`AlignOptions` rules as
  [`Slate::align`](../slate/align.md); see that page for the full breakdown of
  `Caselbl`/`Feature`/`Both` (note `Both` is rejected for a single `Stream`, same as for
  `Slate`).

## See Also

- [`Stream::has_label`](has_label.md) / [`Stream::label`](label.md): the plain getter pair
  for the same underlying label axis, without `values()`'s default-value synthesis.
- [`Stream::with_label`](with_label.md) / [`Stream::with_label_`](with_label_.md): a
  narrower set-only surface, equivalent to `lbl().set()` but strict about length matching.
- [`Stream::clear_label`](clear_label.md) / [`Stream::clear_label_`](clear_label_.md):
  equivalent to `lbl().reset()`, reached through the plain getter/setter surface.
- [`Slate::align`](../slate/align.md): the underlying single-column alignment semantics
  this accessor's `align()` follows.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="label.html">
    <span class="label">Previous</span>
    <span class="title">label</span>
  </a>
  <a class="next-link" href="log.html">
    <span class="label">Next</span>
    <span class="title">log</span>
  </a>
</div>
