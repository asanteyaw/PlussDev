<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 58694bce6585 -->

# StreamLabelAccessor::align

[Back to StreamLabelAccessor API Reference](../../stream_lbl_accessor_api_reference)

## Signatures

```cpp
Stream StreamLabelAccessor::align(const Stream& target_labels, const pluss::table::AlignOptions& options = {}) const
Stream StreamLabelAccessor::align(const pluss::table::Thread& target_labels, const pluss::table::AlignOptions& options = {}) const
```

## What It Does

Reorders (and, depending on `options.align`, relabels) the bound stream to
match `target_labels`, following exactly the same `AlignBy`/`AlignOptions`
rules as [`Thread::align`](../thread/align.md):

- **`AlignBy::Pos`** (the default): `target_labels` is used only for its
  *length* — the values and labels pass through positionally unchanged. See
  Example 3.
- **`AlignBy::Caselbl`**: each position in the result is looked up by
  matching `target_labels`' entries against the bound stream's own current
  labels; an unmatched target label produces `NULL` unless `options.fill`
  is set.

```{important} AlignBy::Both is rejected for a single Stream
`AlignBy::Both` (matching by *both* case label and feature) is a
`DataFrame`-only concept — passing it here throws, the same as it would for
a bare [`Thread::align`](../thread/align.md) call. Only `Pos` and `Caselbl`
are meaningful for a single `Stream`.
```

## Parameters

**target_labels** (`const Stream&` or `const pluss::table::Thread&`)
    The label set to align onto. Under `Pos`, only its length matters; under
    `Caselbl`, its entries are matched against the bound stream's own
    labels.

**options** (`const pluss::table::AlignOptions&`)
    `align` (default `AlignBy::Pos`): the matching strategy described above.
    `fill` (`std::optional<double>`, default `std::nullopt`): the value used
    for an unmatched `Caselbl` position instead of `NULL`.

## Returns

`Stream`
    A new stream reordered/relabeled per `target_labels` and `options`.

## Examples

Example 1. `AlignBy::Caselbl` reorders and relabels by matching case
labels — an unmatched target label (`"d"`) produces `NULL`.

```cpp
using pluss::table::Thread;
using pluss::table::AlignOptions;
using pluss::table::AlignBy;

auto labeled = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}))
                   .lbl().set(Thread(pluss::make_note({"b", "a", "c"})));

auto aligned = labeled.lbl().align(
    Thread(pluss::make_note({"a", "b", "d"})),
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

The bound stream's own labels are `["b", "a", "c"]`, so `"a"` looks up
`20`, `"b"` looks up `10`; `"d"` has no match at all, so it's `NULL`.

Example 2. Setting `options.fill` replaces the unmatched-label `NULL` with
a scalar instead.

```cpp
using pluss::table::Thread;
using pluss::table::AlignOptions;
using pluss::table::AlignBy;

auto labeled = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}))
                   .lbl().set(Thread(pluss::make_note({"b", "a", "c"})));

auto aligned = labeled.lbl().align(
    Thread(pluss::make_note({"a", "b", "d"})),
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

Example 3. The default `AlignBy::Pos` ignores `target_labels`'s *content*
entirely — only its length is checked. The stream's own current values and
labels pass through unchanged.

```cpp
using pluss::table::Thread;

auto labeled = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}))
                   .lbl().set(Thread(pluss::make_note({"b", "a", "c"})));

auto aligned = labeled.lbl().align(Thread(pluss::make_note({"q", "r", "s"})));
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

`target_labels`' actual text (`"q"`, `"r"`, `"s"`) never appears in the
result — only its length (`3`) mattered, and the bound stream's own
`["b", "a", "c"]` labels and `[10, 20, 30]` values pass through unchanged.

## Notes

- See the important callout above — `AlignBy::Both` throws for a single
  `Stream`.
- `Pos` is the default; you must explicitly request `Caselbl` for
  label-matching behavior.
- Both overloads (`Stream`/`Thread`) behave identically — pick whichever
  type your target labels already exist as.

## See Also

- [`Thread::align`](../thread/align.md): the underlying single-column
  alignment semantics this accessor's `align()` follows exactly.
- [`StreamLabelAccessor::set`](set.md): attach a label outright instead of
  reordering by an existing one.
- [`Stream::lbl`](../stream/lbl.md): the entry point this accessor is
  reached from.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="has.html">
    <span class="label">Next</span>
    <span class="title">has</span>
  </a>
</div>
