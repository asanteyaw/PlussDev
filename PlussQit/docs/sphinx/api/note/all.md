<!-- pluss-hand-authored -->
# Note::all

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::all() const
```

## Require Every Boolean Lane

`all()` reduces an entire Bool `Note` to one rank-zero Bool `Note`. The result
is `true` only when every lane of the receiver is `true`. There is no axis
parameter: rank and shape determine how many lanes are visited, but all axes
participate in this reduction.

The result deliberately remains a `Note`. That keeps a predicate pipeline
inside the closed Note type: compose cellwise conditions first, reduce them,
then either keep the scalar Note for further Note operations or cross the host
boundary with `bool_item()` when a C++ `bool` is required.

`all()` accepts only a defined Bool Note. Text that happens to contain
`"true"`, and integers that happen to be zero or one, are not coerced. Build a
Bool Note directly or obtain one from a predicate, comparison, or Boolean
operator.

## Reduction Contract

| Receiver | Result |
|---|---|
| every lane is `true` | scalar Bool Note containing `true` |
| at least one lane is `false` | scalar Bool Note containing `false` |
| scalar Bool Note | the same truth value in a newly allocated scalar Note |
| empty Bool Note of any shape | scalar `true` |
| Text, Bytes, Int64, or Float64 Note | error |
| undefined Note | error |

The empty result is `true` because `true` is the identity for conjunction.
This makes `all()` compositional: appending no additional conditions cannot
turn a previously valid conjunction into a failure. It also means that callers
whose domain requires at least one observation must test `empty()` separately.

The implementation scans contiguous one-byte Boolean lanes and can stop at the
first `false`. Runtime is `O(k)` in the worst case for `k = numel()`, with no
dependence on the receiver's rank. The only result allocation is one Boolean
lane plus scalar metadata; text arenas are never inspected.

## Example 1: All Lanes Pass

```cpp
const auto checks = pluss::note::make({true, true});
std::cout << pluss::note::print(checks.all()) << '\n';
```

### Output

```text
Note(shape=[], dim=0, numel=1, dtype=Bool)
true
```

## Example 2: One Failure Is Enough

```cpp
const auto checks = pluss::note::make({true, false});
std::cout << pluss::note::print(checks.all()) << '\n';
```

### Output

```text
Note(shape=[], dim=0, numel=1, dtype=Bool)
false
```

The returned shape is `[]`, not `[1]`: reduction produces a scalar even when
the receiver is a vector.

## Example 3: Reduce A Higher-Rank Predicate

```cpp
const auto values = pluss::note::make(
    std::vector<std::string>{"Ada", "Grace", "Linus", "Alan"},
    std::array<std::int64_t, 2>{2, 2});
const auto nonempty = values.lengths_chars() > pluss::note::make(std::int64_t{0});

std::cout << pluss::note::print(nonempty) << '\n';
std::cout << pluss::note::print(nonempty.all()) << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, dtype=Bool)
[
  [true, true], 
  [true, true]
]
Note(shape=[], dim=0, numel=1, dtype=Bool)
true
```

`all()` does not reduce each row separately. It consumes all four matrix
lanes. Use shape operations to isolate a subset before reducing when the
question concerns only part of a Note.

## Example 4: Compose Conditions Before Reducing

```cpp
const auto names = pluss::note::make({"Ada", "Grace", "Linus"});
const auto acceptable = names.contains("a") |
                        (names.lengths_chars() >=
                         pluss::note::make(std::int64_t{5}));

std::cout << pluss::note::print(acceptable) << '\n';
std::cout << "all acceptable=" << std::boolalpha
          << acceptable.all().bool_item() << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, dtype=Bool)
[true, true, true]
all acceptable=true
```

The first line stays in Note space. `bool_item()` is used only for the final
host control-flow value.

## Example 5: Scalar Input

```cpp
std::cout << pluss::note::print(pluss::note::make(false).all()) << '\n';
```

### Output

```text
Note(shape=[], dim=0, numel=1, dtype=Bool)
false
```

Scalar reduction still returns a fresh scalar Bool Note rather than a native
`bool`.

## Example 6: Empty Conjunction

```cpp
const auto empty = pluss::note::make(
    std::vector<bool>{}, std::array<std::int64_t, 2>{0, 3});
std::cout << pluss::note::print(empty.all()) << '\n';
```

### Output

```text
Note(shape=[], dim=0, numel=1, dtype=Bool)
true
```

The original empty shape does not survive a full reduction; only the scalar
identity does.

## Example 7: Invalid Receivers Reject

```cpp
try { (void)pluss::note::make({"true", "false"}).all(); }
catch (const pluss::Error&) { std::cout << "all Text: rejected\n"; }

try { (void)pluss::Note::undefined().all(); }
catch (const pluss::Error&) { std::cout << "all undefined: rejected\n"; }
```

### Output

```text
all Text: rejected
all undefined: rejected
```

## See Also

- [`any`](any.md): ask whether at least one Boolean lane passes.
- [`operators`](operators.md): construct and combine Bool Notes with comparisons
  and `&`, `|`, `^`, and `~`.
- [`empty`](empty.md): distinguish an empty domain from a nonempty all-true
  domain when the application requires observations.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="any.html"><span class="label">Next</span><span class="title">any</span></a>
</div>
