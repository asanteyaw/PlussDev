<!-- pluss-hand-authored -->

# Thread::label

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::label() const -> ThreadLabelAccessor
```

## What It Does

Enters the dedicated accessor for this thread's **case-label axis** — the per-row
index every thread carries, independently of its own payload kind. `label()` itself
does nothing but bind the accessor to the current thread; the accessor exposes
inspection (`.values()`, `.has()`), promoting an explicit label thread and clearing it
back to the default (`.set()`, `.range()`, `.reset()`), alignment (`.align()`),
Index-style set operations between two label threads (`.unite()`, `.intersect()`,
`.diff()`, `.symdiff()`), positional lookup (`.find()`, `.pos()`, `.slice()`), and
missing-label handling (`.fillnull()`, `.isnull()`, `.notnull()`).

`Thread::label()` is the 1D foundation this machinery is built on — a `DataFrame`'s
own case-label axis is itself a `Thread`, and [`DataFrame::label()`](../dataframe/label.md)
adds frame-specific concerns (multi-level names, ordering, renaming, dropping,
`codes()`/`levels()`) on top of this same 1D surface.

This page previews the accessor's most common entry points. Every one of its methods
has its own dedicated, example-rich reference page — start at the
[ThreadLabelAccessor API Reference](../../thread_label_accessor_api_reference) and use
its left sidebar to jump straight to any method (`values`, `set`, `reset`, `unite`,
`find`, `pos`, and more).

## Parameters

`None`

## Returns

`ThreadLabelAccessor`
    A lightweight accessor bound to this thread. Methods that inspect labels return a
    `Thread` or `bool`; methods that transform the label axis return a new `Thread`.
    The source thread is never mutated.

## Examples

Example 1. A thread with no explicit labels reports default `0..n-1` range labels,
and `.has()` correctly reports there's nothing explicit set.

```cpp
using namespace pluss::table;

auto scores = Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto labels = scores.label().values();
std::cout << labels;

auto has_explicit = scores.label().has();
std::cout << has_explicit;
```

### Output

```text
0  0
1  1
2  2

ptype: Series
```

```text
false
```

Example 2. Attach explicit labels, then confirm `.has()` now reports `true`.

```cpp
auto labeled = scores.label().set(Thread(pluss::make_note({"Ada", "Bo", "Cy"})));

auto values = labeled.label().values();
std::cout << values;

std::cout << labeled.label().has();
```

### Output

```text
0  Ada
1  Bo
2  Cy

ptype: Note
```

```text
true
```

`.set(...)` is non-mutating — it returns a copy with the new labels attached.
`.reset()` clears them back to the default `0..n-1` range (and `.has()` then reports
`false` again).

## Notes

- `.values()` returns the explicit label thread if one is set, otherwise the default
  `0..n-1` range labels — matching the Pluss hybrid alignment model.
- `.has()` reports whether **explicit** labels are set (`false` for the default range
  labels), not merely whether the thread itself is defined.
- `.set()`, `.range()`, `.reset()`, `.align()`, and `.fillnull()` are non-mutating —
  each returns a new `Thread`.
- `DataFrame::label().values()` returns the frame's case-label thread as a whole;
  `Thread::label()` is where 1D relabeling and 1D label alignment actually live —
  the frame-level accessor delegates to this one under the hood.

## See Also

- [`Thread::nt`](nt.md), [`Thread::tl`](tl.md): the dedicated accessors for a
  thread's own *payload*, as opposed to its case-label axis.
- [`DataFrame::label`](../dataframe/label.md): the frame-level counterpart, which adds
  multi-level naming, ordering, and codes/levels machinery on top of this 1D surface.
- [`Thread::append`](append.md): case-label handling during concatenation depends
  directly on `.label().has()`'s corrected semantics — both operands must agree on
  whether they carry explicit labels, or it throws.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="kurtosis.html">
    <span class="label">Previous</span>
    <span class="title">kurtosis</span>
  </a>
  <a class="next-link" href="land.html">
    <span class="label">Next</span>
    <span class="title">land</span>
  </a>
</div>
