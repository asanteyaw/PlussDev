# Note::undefined

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
static Note Note::undefined() noexcept
```

## Construct The Absence State

`Note::undefined()` constructs a Note handle with no backing value. It is the
explicit absence state used when an operation has no Note to return or an
optional Note argument has not been supplied. It is not a null text lane, not
an empty string, and not a defined Note with a zero-sized dimension.

The distinction is structural. An undefined Note has no dtype or shape. The
non-throwing probes report `defined() == false`, `dim() == 0`, `numel() == 0`,
and `empty() == true`, but the zero dimension is only a safe fallback: the
handle is not a scalar, and `is_scalar()` remains false. Queries that require
an actual value, including `dtype()`, `sizes()`, and lane access, reject it.

Construction is constant-time, performs no allocation, and is `noexcept`.
Copying or moving the handle preserves the undefined state. Many
shape-preserving Note operations propagate undefined, while operations that
require concrete data may reject it; use `defined()` before entering code whose
absence behavior is not part of its contract.

## Example 1: Absence And Empty Geometry Are Different

```cpp
const auto absent = pluss::Note::undefined();
const auto empty = pluss::note::make(
    std::vector<std::string>{}, std::array<std::int64_t, 2>{2, 0});

std::cout << std::boolalpha
          << "absent: defined=" << absent.defined()
          << " dim=" << absent.dim()
          << " numel=" << absent.numel()
          << " empty=" << absent.empty()
          << " scalar=" << absent.is_scalar() << '\n'
          << "empty: defined=" << empty.defined()
          << " dim=" << empty.dim()
          << " numel=" << empty.numel()
          << " empty=" << empty.empty()
          << " shape=[" << empty.size(0) << ", " << empty.size(1) << "]\n";
```

### Output

```text
absent: defined=false dim=0 numel=0 empty=true scalar=false
empty: defined=true dim=2 numel=0 empty=true shape=[2, 0]
```

Both are empty in the cardinality sense, but only the second carries valid Text
dtype and rank-2 geometry.

## Example 2: Dtype Access Requires A Defined Value

```cpp
try {
  (void)pluss::Note::undefined().dtype();
} catch (const pluss::Error&) {
  std::cout << "undefined dtype: rejected\n";
}
```

### Output

```text
undefined dtype: rejected
```

Check `defined()` before requesting metadata that cannot exist without storage.

## Example 3: Copying Preserves Absence

```cpp
const auto absent = pluss::Note::undefined();
const auto copied = absent;
std::cout << std::boolalpha << "copied defined=" << copied.defined() << '\n';
```

### Output

```text
copied defined=false
```

No placeholder allocation is introduced by the copy.

## Example 4: Absence Survives A Binary Round Trip

```cpp
const auto restored = pluss::note::unpack(pluss::Note::undefined().pack());
std::cout << std::boolalpha << "defined=" << restored.defined() << '\n';
```

### Output

```text
defined=false
```

The stable Note envelope records definition state separately from dtype and
payload.

## See Also

- [`defined`](defined.md): test for this state without throwing.
- [`empty`](empty.md): test element count, which also reports true for undefined.
- [`pack`](pack.md): preserve undefined across a byte-oriented boundary.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="transpose.html"><span class="label">Previous</span><span class="title">transpose</span></a>
  <a class="next-link" href="unique.html"><span class="label">Next</span><span class="title">unique</span></a>
</div>
<!-- pluss-hand-authored -->
