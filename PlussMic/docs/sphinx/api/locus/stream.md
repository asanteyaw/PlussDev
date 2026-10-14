<!-- pluss-hand-authored -->

# Locus::to_string, Locus::show, operator<<(Locus)

[Back to Locus API Reference](../../locus_api_reference)

## Signature

```cpp
Locus::to_string() const -> std::string
Locus::show(std::ostream& os) const -> void
Locus::show() const -> void
operator<<(std::ostream& os, const Locus& value) -> std::ostream&
```

## What It Does

`to_string()` renders the workspace's tables and declared links as text, with no
trailing newline. `operator<<` writes exactly that string to a stream -- it matches
`to_string()` byte for byte, the ordinary `std::cout << someLocus` pattern. `show(os)`
writes the same text *plus* a trailing newline, meant for terminal-style printing
rather than for building up a string you'll compare or concatenate; the no-argument
`show()` writes to `std::cout`.

## Parameters

| Parameter | Meaning |
|---|---|
| `os` | The destination stream, for `show(os)` and `operator<<`. |
| `value` | The `Locus` to print, for `operator<<`. |

## Returns

`to_string` returns a `std::string`. `show` returns `void`. `operator<<` returns `os`,
for chaining.

## Examples

Example 1. All three forms, on a linked two-table workspace.

```cpp
Catalog customers(Streams{{"id", nums({1.0, 2.0, 3.0})}, {"name", words({"Ada", "Ben", "Cy"})}});
Catalog orders(Streams{{"order_id", nums({10.0, 11.0, 12.0})}, {"cust_id", nums({1.0, 1.0, 2.0})}});
const auto shop = Locus({{"customers", customers}, {"orders", orders}})
                       .link("orders", "cust_id", "customers", "id", RelationshipCardinality::ManyToOne);

const auto text = shop.to_string();
std::cout << "to_string mentions customers: " << (text.find("customers") != std::string::npos) << '\n';
std::cout << "to_string mentions orders: " << (text.find("orders") != std::string::npos) << '\n';
std::cout << "to_string mentions the link: " << (text.find("orders->customers") != std::string::npos) << '\n';

std::ostringstream shown;
shop.show(shown);
std::cout << "show(os) == to_string() + \"\\n\": " << (shown.str() == text + "\n") << '\n';

std::ostringstream streamed;
streamed << shop;
std::cout << "operator<< matches to_string() exactly: " << (streamed.str() == text) << '\n';
```

### Output

```text
to_string mentions customers: true
to_string mentions orders: true
to_string mentions the link: true
show(os) == to_string() + "\n": true
operator<< matches to_string() exactly: true
```

`show(os)` is `to_string()` with a trailing newline appended, not a different
rendering -- `operator<<` is the one that matches `to_string()`'s own output exactly,
with nothing added.

## See Also

- [`link`](link.md): the relationships `to_string()`'s rendering names.
