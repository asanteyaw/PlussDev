# Timeline operators

[Back to Timeline API Reference](../../timeline_api_reference)

## Signatures

```cpp
Timeline Timeline::operator+(const Duration& duration) const
Timeline operator+(const Duration& duration, const Timeline& timeline)
Timeline Timeline::operator-(const Duration& duration) const
Duration Timeline::operator-(const Timeline& other) const
Timeline& Timeline::operator+=(const Duration& duration)
Timeline& Timeline::operator-=(const Duration& duration)

Series Timeline::operator==(const Timeline& other) const
Series Timeline::operator!=(const Timeline& other) const
Series Timeline::operator<(const Timeline& other) const
Series Timeline::operator<=(const Timeline& other) const
Series Timeline::operator>(const Timeline& other) const
Series Timeline::operator>=(const Timeline& other) const

std::ostream& operator<<(std::ostream& stream, const Timeline& timeline)
```

## Temporal Algebra

A `Timeline` stores instants on the nanosecond epoch axis. A `Duration` stores
signed elapsed lengths. The operators preserve that distinction instead of
treating both values as interchangeable integers.

| Expression | Meaning | Result type |
|---|---|---|
| `timeline + duration` | move each instant by a signed elapsed length | `Timeline` |
| `duration + timeline` | the same shift with reversed operands | `Timeline` |
| `timeline - duration` | move each instant by the negated elapsed length | `Timeline` |
| `timeline - timeline` | measure signed elapsed distance | `Duration` |
| `timeline relation timeline` | compare paired instants | boolean `Series` |
| `stream << timeline` | write the public representation | `std::ostream&` |

The operand types are part of the meaning. `arrival - travel_time` asks when a
journey began and returns instants. `arrival - departure` asks how long the
journey took and returns elapsed durations.

There is no implicit bare-integer timestamp arithmetic. An integer alone does
not communicate whether its unit is nanoseconds, seconds, or days. Construct a
`Duration` through an explicit unit factory such as `nanos`, `seconds`,
`minutes`, `hours`, or `days`.

## Shared Execution Contract

Arithmetic and comparisons are elementwise and rank-polymorphic. Operands
align from the trailing axis, singleton axes may expand, and the result has the
broadcast shape. A scalar has shape `[]`, so it broadcasts across every lane of
the other operand. Shapes `[2, 1]` and `[1, 3]`, for example, produce `[2, 3]`.

Every binary operation allocates a fresh contiguous result. `+`, binary `-`,
and comparisons never rewrite their inputs. Compound assignment computes the
same checked out-of-place result and then replaces the left-hand handle.

Arithmetic uses exact signed 64-bit nanoseconds. It does not pass through a
floating-point representation. A result that would overflow the epoch range,
the Duration range, or collide with the reserved `NaT` sentinel is rejected
instead of wrapping.

A Timeline's timezone controls wall-clock presentation, not the stored
instant. Shift results retain the Timeline operand's timezone. A `Duration`
has no timezone. Timeline subtraction and comparison operate on epoch instants,
so differently displayed values can compare equal when they identify the same
instant.

## Stream Insertion

`operator<<` writes the same public representation as
`pluss::timeline::print(...)`. It returns the stream, so normal stream chaining
works. It is a presentation operation: it neither exposes the epoch buffer nor
transfers ownership.

```cpp
const auto times = pluss::timeline::make({
    "2024-01-01T00:00:00Z", "2024-01-01T02:00:00Z"});

std::cout << times << '\n';
```

### Output

```text
Timeline(shape=[2], tz=UTC)
[ 2024-01-01 00:00:00, 2024-01-01 02:00:00 ]
```

Use [`epoch_nanos()`](epoch_nanos.md) when exact machine values are the subject
of the output. Readable Timeline rendering intentionally does not expose every
sub-second digit.

## Addition

`timeline + duration` moves each defined instant later by a positive duration,
earlier by a negative duration, and not at all by zero. The result timezone is
copied from the Timeline operand.

`duration + timeline` is equally supported because elapsed displacement plus
an instant is commutative. It reaches the same implementation and follows the
same broadcasting, precision, absence, and overflow rules.

### Scalar Shift

A rank-zero Timeline and rank-zero Duration produce a rank-zero Timeline.

```cpp
const auto start = pluss::timeline::make("2024-01-01T00:00:00Z");
const auto shifted = start + pluss::duration::minutes(90);

std::cout << pluss::timeline::print(shifted) << '\n';
```

### Output

```text
Timeline(shape=[], tz=UTC)
2024-01-01 01:30:00
```

### Signed Values And Reversed Operands

The sign belongs to each Duration lane. Here the first instant moves later and
the second moves earlier. Reversing the operands does not reverse direction,
because both expressions are addition.

```cpp
const auto times = pluss::timeline::make({
    "2024-01-01T00:00:00Z", "2024-01-01T02:00:00Z"});
const auto offsets = pluss::duration::hours({1, -2});

std::cout << pluss::timeline::print(times + offsets) << '\n';
std::cout << pluss::timeline::print(offsets + times) << '\n';
```

### Output

```text
Timeline(shape=[2], tz=UTC)
[ 2024-01-01 01:00:00, 2024-01-01 00:00:00 ]

Timeline(shape=[2], tz=UTC)
[ 2024-01-01 01:00:00, 2024-01-01 00:00:00 ]
```

Use whichever operand order makes the surrounding expression clearer. There
is no semantic or performance distinction.

### N-D Broadcasting

A `[2, 1]` Timeline can provide two day anchors while a `[1, 3]` Duration
provides three intraday offsets. Pairing each anchor with every offset produces
a `[2, 3]` Timeline.

```cpp
const std::vector<std::string> values{
    "2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z"};
const auto anchors = pluss::timeline::make(values, {2, 1});
const auto offsets = pluss::duration::minutes({{0, 30, 60}});

std::cout << pluss::timeline::print(anchors + offsets) << '\n';
```

### Output

```text
Timeline(shape=[2, 3], tz=UTC)
[ 2024-01-01 00:00:00 2024-01-01 00:30:00 2024-01-01 01:00:00 ]
[ 2024-01-02 00:00:00 2024-01-02 00:30:00 2024-01-02 01:00:00 ]
```

Broadcasting uses virtual singleton strides. It does not first materialize
expanded copies of both operands.

## Timeline Minus Duration

`timeline - duration` shifts instants by the negation of the Duration. A
positive lane moves an instant earlier; a negative lane moves it later.

```cpp
const auto times = pluss::timeline::make({
    "2024-01-01T00:00:00Z", "2024-01-01T02:00:00Z"});
const auto offsets = pluss::duration::hours({1, -2});

std::cout << pluss::timeline::print(times - offsets) << '\n';
```

### Output

```text
Timeline(shape=[2], tz=UTC)
[ 2023-12-31 23:00:00, 2024-01-01 04:00:00 ]
```

There is no `duration - timeline` overload. An elapsed length minus an epoch
instant has no stable temporal result type. To measure elapsed distance, use a
Timeline on both sides of `-`.

## Timeline Minus Timeline

With two Timeline operands, subtraction measures elapsed distance and returns
`Duration`. Each lane computes `left_epoch_nanos - right_epoch_nanos`.

- Positive means the left instant is later.
- Negative means the left instant is earlier.
- Zero means both operands identify the same instant.

```cpp
const auto left = pluss::timeline::make({
    "2024-01-01T00:00:00Z", "2024-01-01T03:30:00Z"});
const auto right = pluss::timeline::make({
    "2024-01-01T00:00:00Z", "2024-01-01T02:00:00Z"});
const auto elapsed = left - right;

std::cout << pluss::series::print(elapsed.total().seconds()) << '\n';
```

### Output

```text
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.0000, 5400.0000]
```

The seconds projection is only for readable output. The returned Duration
retains exact integer nanoseconds. Keep it as a Duration for further temporal
arithmetic and project to a numeric unit only when the next calculation needs
that unit.

This operator is the expression-oriented form of [`diff()`](diff.md). Both
allocate a Duration, preserve broadcast shape, propagate absent lanes, and
leave their Timeline operands unchanged.

## Nanosecond Precision And Absence

Timeline arithmetic preserves every nanosecond. `NaT` is a reserved sentinel,
not an ordinary early timestamp. If either paired Timeline or Duration lane is
absent, the arithmetic result lane is absent.

The Timeline printer hides sub-second digits, so this example uses
`epoch_nanos()` to make the exact seven-nanosecond shift visible. The second
lane remains `NaT` instead of performing integer arithmetic on the sentinel.

```cpp
const auto precise =
    pluss::timeline::from_epoch({100LL, pluss::Timeline::kNaT});
const auto shifted = precise + pluss::duration::nanos({7, 9});

std::cout << pluss::series::print(shifted.epoch_nanos()) << '\n';
```

### Output

```text
Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[107, -9223372036854775808]
```

Timeline-to-Timeline subtraction follows the same propagation rule and emits
`Duration::kNaT` when either paired instant is absent.

## Fixed Elapsed Time Across Clock Changes

A Duration is fixed elapsed time. Adding two hours always adds exactly
`7,200,000,000,000` nanoseconds. It does not promise that a displayed local
clock advances by two labels if its timezone changes offset in that interval.

The next instant displays as `01:30` before a spring clock change. Two elapsed
hours later it displays as `04:30`, because that local calendar skips the
`02:00` hour. The result retains the Timeline's timezone.

```cpp
const auto local = pluss::timeline::make("2024-03-10T08:30:00Z")
                       .tz_convert("America/Edmonton");
const auto later = local + pluss::duration::hours(2);

std::cout << pluss::timeline::print(later) << '\n';
```

### Output

```text
Timeline(shape=[], tz=America/Edmonton)
2024-03-10 04:30:00
```

Use Duration arithmetic when elapsed time is the requirement. Use named
calendar transformations when the requirement is expressed as preserving or
changing a wall-clock field.

## Compound Assignment

`+=` and `-=` return `Timeline&`. They follow a checked replacement sequence:

1. Validate and broadcast both operands.
2. Compute a fresh Timeline through binary `+` or `-`.
3. Replace the left-hand handle only after success.
4. Return a reference to that left-hand object.

They do not write through the old epoch buffer lane by lane. Consequently,
assigning through a copied handle does not rewrite the original handle.

```cpp
const auto original = pluss::timeline::make({
    "2024-01-01T00:00:00Z", "2024-01-01T02:00:00Z"});
auto changed = original;

changed += pluss::duration::hours(1);
changed -= pluss::duration::minutes(30);

std::cout << pluss::timeline::print(changed) << '\n';
std::cout << pluss::timeline::print(original) << '\n';
```

### Output

```text
Timeline(shape=[2], tz=UTC)
[ 2024-01-01 00:30:00, 2024-01-01 02:30:00 ]

Timeline(shape=[2], tz=UTC)
[ 2024-01-01 00:00:00, 2024-01-01 02:00:00 ]
```

Use binary arithmetic when both old and new values remain meaningful. Use
compound assignment when the variable represents evolving state and its old
value is no longer needed. A failed compound assignment leaves the left-hand
Timeline unchanged because replacement happens only after validation and
checked arithmetic succeed.

## Comparison Operators

All comparisons are elementwise and return boolean `Series`; none collapses an
entire Timeline to one C++ `bool`.

| Operator | True for a defined pair when |
|---|---|
| `==` | both operands identify the same instant |
| `!=` | the operands identify different instants |
| `<` | the left instant occurs earlier |
| `<=` | the left instant occurs earlier or at the same instant |
| `>` | the left instant occurs later |
| `>=` | the left instant occurs later or at the same instant |

The named methods [`eq()`](eq.md), [`ne()`](ne.md), [`lt()`](lt.md),
[`le()`](le.md), [`gt()`](gt.md), and [`ge()`](ge.md) expose the same relations
for method-oriented code.

### Every Relation

The boundary below is scalar, so it broadcasts over the two-element Timeline.

```cpp
const auto times = pluss::timeline::make({
    "2024-01-01T00:00:00Z", "2024-01-01T02:00:00Z"});
const auto boundary = pluss::timeline::make("2024-01-01T02:00:00Z");

std::cout << pluss::series::print(times == boundary) << '\n';
std::cout << pluss::series::print(times != boundary) << '\n';
std::cout << pluss::series::print(times < boundary) << '\n';
std::cout << pluss::series::print(times <= boundary) << '\n';
std::cout << pluss::series::print(times > boundary) << '\n';
std::cout << pluss::series::print(times >= boundary) << '\n';
```

### Output

```text
Series(shape=[2], strides=[1], dtype=bool, platform=cpu, numel=2, contiguous=true)
data=[false, true]

Series(shape=[2], strides=[1], dtype=bool, platform=cpu, numel=2, contiguous=true)
data=[true, false]

Series(shape=[2], strides=[1], dtype=bool, platform=cpu, numel=2, contiguous=true)
data=[true, false]

Series(shape=[2], strides=[1], dtype=bool, platform=cpu, numel=2, contiguous=true)
data=[true, true]

Series(shape=[2], strides=[1], dtype=bool, platform=cpu, numel=2, contiguous=true)
data=[false, false]

Series(shape=[2], strides=[1], dtype=bool, platform=cpu, numel=2, contiguous=true)
data=[false, true]
```

When one answer is required, reduce explicitly. In this example one lane
matches and one does not, so `all()` and `any()` answer different questions:

```cpp
const auto left = pluss::timeline::make({
    "2024-01-01T00:00:00Z", "2024-01-01T02:00:00Z"});
const auto right = pluss::timeline::make({
    "2024-01-01T00:00:00Z", "2024-01-01T03:00:00Z"});
const auto matches = left == right;

std::cout << std::boolalpha
          << "all=" << matches.all().item<bool>()
          << ", any=" << matches.any().item<bool>() << '\n';
```

### Output

```text
all=false, any=true
```

Explicit reduction prevents a multi-element Timeline from acquiring an
ambiguous truth value.

### Higher-Rank Comparison Broadcasting

Here `[2, 2]` is compared with `[2]`. The row operand aligns with the matrix's
final axis and is reused for both rows.

```cpp
const auto matrix = pluss::timeline::make({
    {"2024-01-01T00:00:00Z", "2024-01-01T02:00:00Z"},
    {"2024-01-02T00:00:00Z", "2024-01-02T02:00:00Z"},
});
const auto row = pluss::timeline::make({
    "2024-01-01T01:00:00Z", "2024-01-02T02:00:00Z"});

std::cout << pluss::series::print(matrix < row) << '\n';
```

### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=bool, platform=cpu, numel=4, contiguous=true)
data=
⎡  true  true ⎤
⎣ false false ⎦
```

Read the result after broadcasting: each matrix row is compared lane by lane
with the same two row boundaries.

### Timezones Compare Instants

Timezone conversion changes presentation while retaining the epoch instant.
Therefore a UTC value compares equal to its converted local display.

```cpp
const auto utc = pluss::timeline::make("2024-07-01T18:00:00Z");
const auto local = utc.tz_convert("America/Edmonton");

std::cout << pluss::series::print(utc == local) << '\n';
```

### Output

```text
Series(shape=[], strides=[], dtype=bool, platform=cpu, numel=1, contiguous=true)
data=true
```

This is instant equality, not equality of timezone labels or displayed fields.
Compare `tz()` explicitly if the metadata label is part of the condition.

### Absent Instants Are Unordered

`NaT` follows an unordered absent-instant contract:

- `NaT == x` is false, including when `x` is `NaT`.
- `NaT != x` is true, including when `x` is `NaT`.
- `<`, `<=`, `>`, and `>=` are false when either lane is `NaT`.

The second lane below is defined and equal to itself, making the contrast
visible for every relation.

```cpp
const auto values =
    pluss::timeline::make({"", "2024-01-01T00:00:00Z"});

std::cout << pluss::series::print(values == values) << '\n';
std::cout << pluss::series::print(values != values) << '\n';
std::cout << pluss::series::print(values < values) << '\n';
std::cout << pluss::series::print(values <= values) << '\n';
std::cout << pluss::series::print(values > values) << '\n';
std::cout << pluss::series::print(values >= values) << '\n';
```

### Output

```text
Series(shape=[2], strides=[1], dtype=bool, platform=cpu, numel=2, contiguous=true)
data=[false, true]

Series(shape=[2], strides=[1], dtype=bool, platform=cpu, numel=2, contiguous=true)
data=[true, false]

Series(shape=[2], strides=[1], dtype=bool, platform=cpu, numel=2, contiguous=true)
data=[false, false]

Series(shape=[2], strides=[1], dtype=bool, platform=cpu, numel=2, contiguous=true)
data=[false, true]

Series(shape=[2], strides=[1], dtype=bool, platform=cpu, numel=2, contiguous=true)
data=[false, false]

Series(shape=[2], strides=[1], dtype=bool, platform=cpu, numel=2, contiguous=true)
data=[false, true]
```

Prefer [`isvalid()`](isvalid.md) or [`isnat()`](isnat.md) when validity itself
is the question; those methods state the intent directly.

### Building Interval Masks

Comparison results compose with Series logical operations. A closed interval
includes both boundaries, so it uses `>=` and `<=` joined by `logical_and()`.

```cpp
const auto samples = pluss::timeline::make({
    "2024-01-01T00:00:00Z", "2024-01-01T01:00:00Z",
    "2024-01-01T02:00:00Z", "2024-01-01T03:00:00Z"});
const auto start = pluss::timeline::make("2024-01-01T00:00:00Z");
const auto stop = pluss::timeline::make("2024-01-01T02:00:00Z");

const auto inside =
    (samples >= start).logical_and(samples <= stop);
std::cout << pluss::series::print(inside) << '\n';
```

### Output

```text
Series(shape=[4], strides=[1], dtype=bool, platform=cpu, numel=4, contiguous=true)
data=[true, true, true, false]
```

Use `< stop` for a half-open interval. Keeping boolean composition on the
returned Series is why Timeline does not overload `&`, `|`, `^`, or `~`.

## Empty Shapes

An empty broadcast is valid when the other operand contributes only compatible
singleton axes. The result preserves its zero-length dimension and performs no
lane reads. Empty shape is not scalar absence; the shape remains meaningful to
later operations.

```cpp
const std::vector<std::int64_t> no_ticks;
const std::vector<std::int64_t> shape{0, 2};
const auto empty = pluss::timeline::from_epoch(no_ticks, shape);
const auto boundary = pluss::timeline::make("2024-01-01T00:00:00Z");

std::cout << pluss::timeline::print(
    empty + pluss::duration::hours(1)) << '\n';
std::cout << pluss::series::print(empty == boundary) << '\n';
```

### Output

```text
Timeline(shape=[0, 2], tz=UTC)

Series(shape=[0,2], strides=[2,1], dtype=bool, platform=cpu, numel=0, contiguous=true)
[]
```

## Failure Modes

### Incompatible Shapes

Dimensions are compatible when equal or when either is `1`. Otherwise the
operation throws before producing a result. `[2, 2]` and `[3]` fail because
their trailing dimensions are `2` and `3`.

```cpp
try {
  const auto matrix = pluss::timeline::make({
      {"2024-01-01T00:00:00Z", "2024-01-01T01:00:00Z"},
      {"2024-01-01T02:00:00Z", "2024-01-01T03:00:00Z"},
  });
  const auto three = pluss::timeline::make({
      "2024-01-01T00:00:00Z", "2024-01-01T01:00:00Z",
      "2024-01-01T02:00:00Z"});
  (void)(matrix == three);
} catch (const std::exception&) {
  std::cout << "rejected: shapes do not broadcast\n";
}
```

### Output

```text
rejected: shapes do not broadcast
```

### Arithmetic Overflow

The valid Timeline range reserves one signed 64-bit value for `NaT`. Addition,
shift subtraction, and Timeline difference are checked independently. None can
wrap to a plausible but incorrect instant or duration.

```cpp
const auto latest = pluss::timeline::from_epoch(
    std::numeric_limits<std::int64_t>::max());
const auto earliest = pluss::timeline::from_epoch(
    std::numeric_limits<std::int64_t>::min() + 1);

try {
  (void)(latest + pluss::duration::nanos(1));
} catch (const std::exception&) {
  std::cout << "rejected: result is outside Timeline range\n";
}
try {
  (void)(earliest - pluss::duration::nanos(1));
} catch (const std::exception&) {
  std::cout << "rejected: result is outside Timeline range\n";
}
try {
  (void)(latest - earliest);
} catch (const std::exception&) {
  std::cout << "rejected: result is outside Duration range\n";
}
```

### Output

```text
rejected: result is outside Timeline range
rejected: result is outside Timeline range
rejected: result is outside Duration range
```

## Deliberately Unsupported Operators

The absence of an overload is part of the contract. Operators that erase
units, expose epoch encoding, or invent an arbitrary calendar step are
intentionally unavailable.

| Unsupported form | Why it is not Timeline algebra | Use instead |
|---|---|---|
| `timeline + timeline` | adding epoch origins does not produce an instant | subtract Timelines to measure distance |
| `duration - timeline` | elapsed length minus an instant has no temporal result | reorder the intended Timeline expression |
| `timeline * x`, `timeline / x`, `timeline % x` | scaling an instant is epoch-dependent | scale a Duration |
| unary `+timeline`, unary `-timeline` | reflecting around the epoch is not a date operation | project epoch values only when encoding is intended |
| `++timeline`, `--timeline` | no uniquely correct step exists | add an explicit Duration or use a calendar transform |
| `timeline & timeline`, `timeline | timeline`, `timeline ^ timeline`, `~timeline` | bitwise operations expose storage rather than time semantics | compare first, then combine boolean Series |
| `timeline && timeline`, `timeline || timeline`, `!timeline` | an array has no single truth value | create and explicitly reduce a mask |

Multiplication and division may be meaningful for elapsed durations, but not
for absolute instants. A fixed 24-hour Duration and a local calendar-day move
also differ near clock changes. Requiring an explicit operation keeps the unit
and temporal interpretation visible.

Ordinary copy and move assignment are standard C++ object mechanics rather
than temporal operators. They copy or move a Timeline handle; only `+=` and
`-=` carry Timeline-specific arithmetic meaning.

## Choosing The Right Form

- Use `timeline + duration` or `duration + timeline` for fixed elapsed shifts.
- Use `timeline - duration` for the opposite elapsed direction.
- Use `timeline - timeline` for signed elapsed measurement.
- Use comparisons for lane-wise temporal masks.
- Reduce a comparison explicitly when one bool is required.
- Use named calendar transformations when wall-clock fields must be preserved.
- Use `epoch_nanos()` only when storage-scale integers are intentionally part
  of the next operation.

## See Also

- [`Timeline::add_duration`](add_duration.md)
- [`Timeline::sub_duration`](sub_duration.md)
- [`Timeline::diff`](diff.md)
- [`Timeline::eq`](eq.md)
- [`Timeline::isnat`](isnat.md)
- [`Timeline::epoch_nanos`](epoch_nanos.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="numel.html"><span class="label">Previous</span><span class="title">numel</span></a>
  <a class="next-link" href="period_start.html"><span class="label">Next</span><span class="title">period_start</span></a>
</div>
