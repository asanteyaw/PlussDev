<!-- pluss-hand-authored -->

# LocusTraverse::options

[Back to LocusTraverse API Reference](../../locus_traverse_api_reference)

## Signature

```cpp
LocusTraverse LocusTraverse::options(
    const pluss::bundle::locus::JoinOptions& options) const
```

## Controlling Relational Materialization

`options()` replaces the policy used by every join step currently in the
builder and every step appended later. It does not add a hop, alter the current
anchor, or affect a zero-hop collection. The returned builder owns a copy of
the options, so the caller may modify or destroy the original record afterward.

Use it when ragged streams need an explicit domain or alignment, omitted
records must become a hard error, trailing absence must be padded, or matched
same-named keys must remain separate. Join mode and key selection do not belong
here; those are properties of each `JoinSpec` passed to `join()` or the `how`
argument passed to `via()`.

## JoinOptions Fields

| Field | Meaning | Default |
|---|---|---|
| `ragged` | Shared record-domain, alignment, absence, materialization, and loss policy for both sides of each hop. | `RaggedPolicy::defaults(OpClass::Relational)` |
| `coalesce_keys` | For `Inner`, `Left`, and `LeftOnly`, fold a same-named right key into its left key when doing so cannot discard a right-only value. | `true` |

### RaggedPolicy Fields For Joins

| Field | Meaning for traversal joins | Default | Join constraints |
|---|---|---|---|
| `domain` | Domain used to read each input Catalog: longest stream, shortest stream, or exact equal lengths. | `Longest` | `Anchor` is unsupported. `Shortest` can trim records; `Exact` rejects ragged catalogs. |
| `alignment` | Position of shorter streams in the chosen domain. | `Head` | `Head` and `Tail` are valid. `Label` is unsupported because joins align by key values. |
| `absent` | Representation of an input position or partner that does not exist. | `Null` | Must remain `Null`; relational absence is explicit. |
| `nulls` | Treatment of null key values. | `Skip` | Must remain `Skip`; null keys never match. |
| `materialize` | Whether trailing absent values remain ragged or are emitted as explicit null records. | `Preserve` | `Preserve` or `Pad`. It does not change which join pairs match. |
| `loss` | Whether records omitted or trimmed by the selected join/domain are reported or rejected. | `Report` | `Error` makes any such loss fail collection. |
| `min_support` | Minimum contributing-stream count used by other ragged operations. | `1` | Must remain `1`; it has no join meaning. |
| `fill` | Scalar fill used by `RaggedAbsent::Fill` in applicable operations. | unset | Must remain unset because joins require `absent == Null`. |
| `anchor_stream` | Stream used by an anchor domain. | unset | Must remain unset because `Anchor` is unsupported. |

`RaggedLoss::Report` does not attach a report to `collect()`; it means the join
may proceed. Use the detailed Locus/Catalog join APIs when the numerical match
report itself is required.

## Returns

`LocusTraverse`
: A new immutable builder whose option record replaces the prior one. A later
  `options()` call replaces this record again; the source builder is unchanged.

## Examples

All outputs are emitted by the permanent Locus traversal reference executable.

Example 1. Disable key coalescing to retain both same-named join keys. The right
copy receives the builder's catalog-specific suffix.

```cpp
JoinOptions separate_keys;
separate_keys.coalesce_keys = false;
const auto result = data.from("orders")
    .options(separate_keys)
    .join("customers")
    .collect();
print_keys(result);
print_numbers(result, "customer_id_customers");
```

### Output

```text
keys=[order_id, customer_id, amount, state, customer_id_customers, name] rows=4
customer_id_customers=[1, 2, 2, 3]
```

Example 2. `Preserve` keeps a short trailing stream short. `Pad` gives it the
full output domain and an explicit null.

```cpp
const auto preserved = ragged.from("orders")
    .join("customers", mode(JoinHow::Left)).collect();
std::cout << "preserve amount size=" << preserved.keys("amount").size()
          << " rows=" << preserved.maxrows() << "\n";

JoinOptions padded;
padded.ragged.materialize = RaggedMaterialize::Pad;
const auto rectangular = ragged.from("orders")
    .options(padded)
    .join("customers", mode(JoinHow::Left))
    .collect();
print_numbers(rectangular, "amount");
```

### Output

```text
preserve amount size=4 rows=5
amount=[50, 75, 20, 90, null]
```

Example 3. Tail alignment places the short stream against the end of the common
domain before gathering.

```cpp
JoinOptions tails = padded;
tails.ragged.alignment = pluss::kindle::RaggedAlignment::Tail;
print_numbers(ragged.from("orders")
    .options(tails)
    .join("customers", mode(JoinHow::Left))
    .collect(), "amount");
```

### Output

```text
amount=[null, 50, 75, 20, 90]
```

Example 4. `Shortest` cuts each input to the extent shared by all its streams.
`Exact` instead rejects the same ragged Catalog.

```cpp
JoinOptions shortest;
shortest.ragged.domain = pluss::kindle::RaggedDomain::Shortest;
const auto cut = ragged.from("orders")
    .options(shortest)
    .join("customers", mode(JoinHow::Left))
    .collect();
std::cout << "shortest rows=" << cut.maxrows()
          << " amount size=" << cut.keys("amount").size() << "\n";

JoinOptions exact;
exact.ragged.domain = pluss::kindle::RaggedDomain::Exact;
print_error("exact", [&] {
  (void)ragged.from("orders").options(exact)
      .join("customers", mode(JoinHow::Left)).collect();
});
```

### Output

```text
shortest rows=4 amount size=4
exact: Catalog::join: streams must have equal length but 'amount' has 4 records and 'order_id' has 5
```

Example 5. `RaggedLoss::Error` turns an otherwise valid inner join into a failure
when either side has an unmatched record.

```cpp
JoinOptions lossless;
lossless.ragged.loss = RaggedLoss::Error;
print_error("loss", [&] {
  (void)data.from("orders").options(lossless)
      .join("customers").collect();
});
```

### Output

```text
loss: Catalog::join: loss is set to Error but the join omits records (4 output record(s); 4 matched pair(s). left: 1 of 5 record(s) without a partner (0 without a key), 1 omitted. right: 1 of 4 without a partner (0 without a key), 1 omitted.)
```

Example 6. Policy consistency is validated at collection because a builder with
no joins never consumes join options.

```cpp
JoinOptions invalid;
invalid.ragged.materialize = RaggedMaterialize::Pad;
invalid.ragged.absent = pluss::kindle::RaggedAbsent::Skip;
const auto lazy = data.from("orders").options(invalid).join("customers");
std::cout << "invalid options plan built\n";
print_error("invalid options collect", [&] { (void)lazy.collect(); });
```

### Output

```text
invalid options plan built
invalid options collect: Catalog::join: a row with no partner is always an explicit null; absent must be Null
```

Example 7. Option changes branch immutably.

```cpp
const auto base = data.from("orders").join("customers");
std::cout << "default rows=" << base.collect().maxrows() << "\n";
print_error("derived loss", [&] { (void)base.options(lossless).collect(); });
std::cout << "default again rows=" << base.collect().maxrows() << "\n";
```

### Output

```text
default rows=4
derived loss: Catalog::join: loss is set to Error but the join omits records (4 output record(s); 4 matched pair(s). left: 1 of 5 record(s) without a partner (0 without a key), 1 omitted. right: 1 of 4 without a partner (0 without a key), 1 omitted.)
default again rows=4
```

## Cost

Calling `options()` copies a small policy record and the builder state; it does
not scan data. Domain resolution and padding occur independently for each
collected hop. `Pad` may allocate validity and full-domain payload storage that
`Preserve` can avoid. `Exact` is useful when rejecting malformed ragged inputs
is more important than permissive materialization, while `loss == Error`
protects against both domain trimming and match omission.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="join.html"><span class="label">Previous</span><span class="title">join</span></a>
  <a class="next-link" href="pick.html"><span class="label">Next</span><span class="title">pick</span></a>
</div>
