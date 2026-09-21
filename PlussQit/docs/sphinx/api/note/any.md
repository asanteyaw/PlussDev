<!-- pluss-hand-authored -->
# Note::any

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::any() const
```

## Detect At Least One Passing Lane

`any()` reduces an entire Bool `Note` to one rank-zero Bool `Note`. It returns
`true` as soon as one receiver lane is `true`; it returns `false` only when
every lane is false or the receiver has no lanes.

There is no axis overload. A vector, matrix, or rank-three Note is treated as
one Boolean population in row-major storage order. Shape affects which lanes
belong to the receiver, but not the scalar shape of the result.

The operation is intentionally strict about dtype. It does not treat nonempty
text as true, parse `"true"`, or convert nonzero integers. Its input must be a
defined Bool Note, normally produced by a predicate, comparison, Boolean
composition, or `pluss::note::make` with Boolean values.

## Reduction Contract

| Receiver | Result |
|---|---|
| at least one lane is `true` | scalar Bool Note containing `true` |
| every lane is `false` | scalar Bool Note containing `false` |
| scalar Bool Note | the same truth value in a newly allocated scalar Note |
| empty Bool Note of any shape | scalar `false` |
| Text, Bytes, Int64, or Float64 Note | error |
| undefined Note | error |

The empty result is `false`, the identity for disjunction. No candidate lane
exists that could satisfy the condition.

`any()` scans contiguous one-byte Boolean storage and short-circuits at the
first true lane. Its worst-case cost is `O(numel())`; a leading true lane makes
the scan constant-time apart from creating the scalar result. It does not
materialize a flattened Note or inspect text payloads.

## Example 1: Find One Passing Lane

```cpp
const auto checks = pluss::note::make({true, false, false});
std::cout << pluss::note::print(checks.any()) << '\n';
```

### Output

```text
Note(shape=[], dim=0, numel=1, dtype=Bool)
true
```

## Example 2: No Lane Passes

```cpp
const auto checks = pluss::note::make({false, false});
std::cout << pluss::note::print(checks.any()) << '\n';
```

### Output

```text
Note(shape=[], dim=0, numel=1, dtype=Bool)
false
```

## Example 3: Reduce A Matrix Predicate

```cpp
const auto cells = pluss::note::make(
    std::vector<std::string>{"draft", "ready", "hold", "sent"},
    std::array<std::int64_t, 2>{2, 2});
const auto ready = cells == "ready";

std::cout << pluss::note::print(ready) << '\n';
std::cout << pluss::note::print(ready.any()) << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, dtype=Bool)
[
  [false, true], 
  [false, false]
]
Note(shape=[], dim=0, numel=1, dtype=Bool)
true
```

All axes participate. The method answers whether the complete matrix contains
a match, not which row contains one.

## Example 4: Build A Compound Search

```cpp
const auto names = pluss::note::make({"Ada", "Grace", "Linus"});
const auto selected = names.starts_with("A") | names.ends_with("s");

std::cout << pluss::note::print(selected) << '\n';
std::cout << "found=" << std::boolalpha << selected.any().bool_item() << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, dtype=Bool)
[true, false, true]
found=true
```

Use the scalar Note result directly when the next operation accepts a Note.
Call `bool_item()` only when native C++ control flow needs a `bool`.

## Example 5: Scalar Input

```cpp
std::cout << pluss::note::print(pluss::note::make(true).any()) << '\n';
```

### Output

```text
Note(shape=[], dim=0, numel=1, dtype=Bool)
true
```

## Example 6: Empty Disjunction

```cpp
const auto empty = pluss::note::make(
    std::vector<bool>{}, std::array<std::int64_t, 2>{0, 3});
std::cout << pluss::note::print(empty.any()) << '\n';
```

### Output

```text
Note(shape=[], dim=0, numel=1, dtype=Bool)
false
```

If an empty domain should be an application error rather than a negative
answer, check `empty()` before reducing.

## Example 7: Invalid Receivers Reject

```cpp
try { (void)pluss::note::make({"", "value"}).any(); }
catch (const pluss::Error&) { std::cout << "any Text: rejected\n"; }

try { (void)pluss::Note::undefined().any(); }
catch (const pluss::Error&) { std::cout << "any undefined: rejected\n"; }
```

### Output

```text
any Text: rejected
any undefined: rejected
```

## See Also

- [`all`](all.md): require every Boolean lane to pass.
- [`operators`](operators.md): produce and combine shape-preserving Bool Notes.
- [`empty`](empty.md): distinguish absence of lanes from an all-false population.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="all.html"><span class="label">Previous</span><span class="title">all</span></a>
  <a class="next-link" href="argsort.html"><span class="label">Next</span><span class="title">argsort</span></a>
</div>
