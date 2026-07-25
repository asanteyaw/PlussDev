<!-- pluss-hand-authored -->

# Thread::append

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::append(const Thread& other) const -> Thread
```

## What It Does

Concatenates `other` onto the end of `*this`, returning a new, longer thread. This
is a strict operation: `*this` and `other` must have the same [`kind()`](kind.md),
and for `Series`-kind threads, the same concrete scalar dtype and platform too —
`append()` never coerces or promotes types to make two mismatched threads fit
together.

## Parameters

**other** (`const Thread&`)
    The thread to append. Must match `*this` exactly in payload kind (and, for
    `Series`, scalar dtype and platform).

## Returns

`Thread`
    A new thread, length `this->size() + other.size()`, holding `*this`'s entries
    followed by `other`'s.

## Examples

Example 1. Append two compatible `Series`-kind threads.

```cpp
using namespace pluss::table;

auto first = Thread(pluss::make_series({10.0, 20.0}));
auto second = Thread(pluss::make_series({30.0, 40.0}));
auto out = first.append(second);
std::cout << out;
```

### Output

```text
0  10
1  20
2  30
3  40

ptype: Series
```

Example 2. Appending threads of different payload kinds throws.

```cpp
auto notes = Thread(pluss::make_note({"a", "b"}));
try {
  auto bad = first.append(notes);
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Thread::append: payload kinds must match
```

## Notes

- Requires identical `kind()` between `*this` and `other`; throws
  `"Thread::append: payload kinds must match"` otherwise.
- For `Series`-kind threads, also requires identical concrete scalar dtype
  (`"Thread::append: Series dtypes must match"`) and platform
  (`"Thread::append: Series platforms must match"`).
- Validity masks combine correctly even when only one side has one — a side with
  no explicit mask is treated as fully valid.
- Also requires both threads to agree on whether they carry case labels at all
  (`"Thread::append: both Threads must either have case labels or both be
  unlabeled"`); if both do, the labels are appended too.

## See Also

- [`Thread::take`](take.md): combine with `append()` for more elaborate
  reassembly.
- [`Thread::cast`](cast.md) / [`Thread::astype`](astype.md): convert dtype first
  if two `Series`-kind threads have mismatched scalar types.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="any.html">
    <span class="label">Previous</span>
    <span class="title">any</span>
  </a>
  <a class="next-link" href="as_note.html">
    <span class="label">Next</span>
    <span class="title">as_note</span>
  </a>
</div>
