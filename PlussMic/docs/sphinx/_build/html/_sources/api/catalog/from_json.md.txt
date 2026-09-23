<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 43067c36529a -->

# Catalog::from_json

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
static Catalog Catalog::from_json(std::string_view json_text)
```

## What It Does

Parses a JSON array of flat objects directly from an in-memory string — the same shape and
dtype-inference rules as [`load_json()`](load_json.md), just without touching the
filesystem. Useful for JSON that's already in memory (an HTTP response body, a test
fixture, a string literal) rather than sitting in a file.

## Parameters

**json_text** (`std::string_view`)
    A JSON array of flat (non-nested) objects.

## Returns

`Catalog`
    A new catalog with one stream per distinct key found across the JSON objects.

## Examples

Example 1. Parse an inline JSON literal.

```cpp
auto ct = pluss::kindle::Catalog::from_json(
    R"([{"id":1,"name":"Ada"},{"id":2,"name":"Bola"}])");
std::cout << ct;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=2
keys: [id, name]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
id                       | Series       | Series[int64]    | 2       | [1, 2]                                           
name                     | Note         | Note             | 2       | [Ada, Bola]                                      
========================================================================================================================
```

## Notes

- Expects a flat array-of-objects JSON shape — the same shape [`load_json()`](load_json.md)
  reads from a file.
- Column dtype is inferred per-column from its values.

## See Also

- [`Catalog::load_json`](load_json.md): the same parsing, reading from a file instead of
  an in-memory string.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="freq.html">
    <span class="label">Previous</span>
    <span class="title">freq</span>
  </a>
  <a class="next-link" href="get.html">
    <span class="label">Next</span>
    <span class="title">get</span>
  </a>
</div>
