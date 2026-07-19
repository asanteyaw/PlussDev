# Thread::align

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::align(const Thread& target_labels, const AlignOptions& options = {}) -> Thread
```

Realign one thread to a target label thread using positional, case-label, or feature-label rules.

`Thread::align(...)` is the typed column-level alignment surface behind many higher-level DataFrame operations. Use it when you are working with one extracted column directly and you need to reorder or relabel it against another label vector before comparison, arithmetic, export, or manual reconstruction.

## Parameters

**target_labels**

Target label vector that defines the output order.

Its interpretation depends on `options.align`:

- `AlignBy::Caselbl`: treat `target_labels` as the desired case axis.
- `AlignBy::Feature`: treat `target_labels` as the desired feature-label axis.
- `AlignBy::Pos`: validate strict positional compatibility without doing label-based remapping.

**options**

Alignment policy.

`AlignOptions` exposes two public fields:

- `align`: chooses the matching rule.
  - `AlignBy::Pos`: strict positional alignment.
  - `AlignBy::Caselbl`: match by case labels.
  - `AlignBy::Feature`: match by feature labels.
  - `AlignBy::Both`: not supported on `Thread`.
- `fill`: optional numeric fill scalar for newly introduced gaps.

## Returns

`Thread`
    New thread whose values and label provenance reflect the requested alignment mode.

    - `AlignBy::Caselbl` gathers values by case label.
    - `AlignBy::Feature` gathers one value per feature label.
    - `AlignBy::Pos` keeps positional matching strict.

## See Also

- <a href="../dataframe/align.html"><code>DataFrame::align</code></a>: frame-level alignment for several features at once.
- <a href="label.html"><code>Thread::label</code></a>: use <code>label().set(...)</code> to attach explicit labels before aligning by label.

## Notes

`AlignBy::Both` is a DataFrame-oriented concept and is rejected for `Thread`.

When `fill` is omitted, unmatched target labels produce missing output entries. When `fill` is provided, newly introduced numeric gaps are filled with that scalar.

`AlignBy::Pos` does not broadcast. It expects compatible lengths and preserves the current thread's value order and visible labels.

## Examples

Example 1. Align by case labels and let unmatched labels stay missing.

```cpp
using namespace pluss::table;

auto labeled = Thread(pluss::make_series({10.0, 20.0, 30.0}))
                   .label().set(pluss::make_note({"b", "a", "c"}));

auto out = labeled.align(
    pluss::make_note({"a", "b", "d"}),
    AlignOptions{.align = AlignBy::Caselbl}
);
```

### Output

```text
a  20
b  10
d  NA
ptype: Series
```

Example 2. Fill newly introduced numeric slots during case-label alignment.

```cpp
using namespace pluss::table;

auto labeled = Thread(pluss::make_series({10.0, 20.0, 30.0}))
                   .label().set(pluss::make_note({"b", "a", "c"}));

auto out = labeled.align(
    pluss::make_note({"a", "b", "d"}),
    AlignOptions{.align = AlignBy::Caselbl, .fill = 0.0}
);
```

### Output

```text
a  20
b  10
d  0
ptype: Series
```

Example 3. Align a one-value-per-feature thread by feature label.

```cpp
using namespace pluss::table;

auto feature_thread = Thread(pluss::make_series({1.5, 2.5}))
                          .label().set(pluss::make_note({"A2", "A1"}));

auto out = feature_thread.align(
    pluss::make_note({"A1", "A2"}),
    AlignOptions{.align = AlignBy::Feature}
);
```

### Output

```text
A1  2.5
A2  1.5
ptype: Series
```

Example 4. Use strict positional alignment when you want no label-based remapping at all.

```cpp
using namespace pluss::table;

auto labeled = Thread(pluss::make_series({10.0, 20.0, 30.0}))
                   .label().set(pluss::make_note({"b", "a", "c"}));

auto out = labeled.align(
    pluss::make_note({"x", "y", "z"}),
    AlignOptions{.align = AlignBy::Pos}
);
```

### Output

```text
b  10
a  20
c  30
ptype: Series
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="add.html">
    <span class="label">Previous</span>
    <span class="title">add</span>
  </a>
  <a class="next-link" href="all.html">
    <span class="label">Next</span>
    <span class="title">all</span>
  </a>
</div>
