<!-- pluss-hand-authored -->
# pluss::note::format

[Back to Note Factory API Reference](../../note_factory_api_reference)

## Signatures

```cpp
Note pluss::note::format(string_view tpl,
                         std::initializer_list<Binding> bindings)
```

## Fill A Template, Get A Scalar `Note`

`format()` renders a template string against a set of named bindings and
returns the result as a **scalar** (`dim=0`) UTF-8 `Note` — one cell holding
the finished string. It is the assembly counterpart to the splitting
factories: where [`from_lines`](from_lines.md) and [`from_words`](from_words.md)
break a string apart, `format` builds one up.

Use it whenever a string has to be built from named parts: a labelled
message, a generated identifier, a one-line report, a path or query. Beyond
plain substitution it also evaluates a small **expression language** inside
each placeholder, so light computation — arithmetic, choices, list joins, a
bound callback — can happen in the template instead of in the surrounding
code.

The result is a scalar `Note` and never larger; `format` produces a single
string, not a column. To build many strings, call it per row, or use the
vectorised `Note` methods on an existing `Note`.

## Bindings

Each entry in `bindings` is a `Binding` — a name paired with either a
**value** or a **callable**:

```cpp
{"user", "Ada"}                       // string value
{"count", 42}                         // integer value
{"ratio", 0.75}                       // floating-point value
{"ok", true}                          // bool value
{"tags", std::vector<std::string>{"a", "b"}}   // list value
{"shout", [](std::string s) { return s + "!"; }}  // callable
```

Value bindings accept `bool`, any signed or unsigned integer, any
floating-point type, `char`, `const char*` / `std::string_view` /
`std::string`, and `std::vector<T>` (which becomes a list). Binding names
must be non-empty and unique across the list.

## Template Syntax

- Text outside `{ }` is copied verbatim.
- `{{` emits a literal `{`; `}}` emits a literal `}`.
- `{ name }` substitutes the value of binding `name`.
- `{ expression }` evaluates the expression and splices in its stringified
  result.

### The Expression Language

Inside `{ }` the following is available, loosely in C++ spelling:

- **Literals** — integers and decimals (`42`, `3.14`, `1e3`); string
  literals in `'...'` or `"..."` with `\n`, `\t`, `\\`, `\"`, `\'` escapes.
- **Identifiers** — a bare name is that binding's value; `::` qualified
  names are allowed (used by the built-in functions).
- **Arithmetic** — `+ - * /` and unary `+ - !`. Arithmetic is done in
  floating point; an integer-valued result prints without a decimal point.
- **`+` on non-numbers** — string concatenation, after stringifying each
  side.
- **Comparisons** — `<  <=  >  >=` compare numerically; `==` and `!=`
  compare the stringified operands.
- **Logic and choice** — `&&`, `||`, and the ternary `cond ? a : b`.
  Truthiness: non-zero numbers, non-empty strings/lists, `true`.
- **Indexing and members** — `x[i]` and `x.at(i)` take a list element or a
  one-character slice of a string; `x.size()` and `x.empty()` work on both
  strings and lists.
- **Built-in functions** —
  `std::abs` / `std::fabs`, `std::sqrt`, `std::floor`, `std::ceil`,
  `std::round`, `std::pow(a, b)`, `std::min(...)` / `std::max(...)` (two or
  more arguments), `std::accumulate(list, init)`,
  `pluss::join(list)` or `pluss::join(list, delimiter)` (default delimiter
  `", "`), and `std::to_string(x)` / `pluss::str(x)`.
- **Callable bindings** — `{ fn(args...) }` invokes a bound C++ callable.
  Arguments are converted to the callable's parameter types; the return
  value is stringified. A callable binding used without `()` is an error,
  and a value binding used with `()` is an error.

### How Values Stringify

`true` / `false` for bools; plain decimal for integers; up to 15
significant digits for floating point (so `5.0` prints as `5`, `2.5` as
`2.5`); `[a, b, c]` for a list.

## Example 1: A Plain Substitution

```cpp
std::cout << pluss::note::print(
    pluss::note::format("Hello, {name}!", {{"name", "Ada"}})) << "\n";
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"Hello, Ada!"
```

The result is a scalar `Note` — the finished string in one cell.

## Example 2: Literal Braces

```cpp
const pluss::Note n = pluss::note::format("{{literal}} and {x}", {{"x", 7}});
std::cout << n << "\n";
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"{literal} and 7"
```

`{{` and `}}` are the escapes for literal braces; `{x}` still substitutes.

## Example 3: Arithmetic In The Template

```cpp
std::cout << pluss::note::print(
    pluss::note::format("{a} + {b} = {a + b}", {{"a", 2}, {"b", 3}})) << "\n";
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"2 + 3 = 5"
```

`{a + b}` is evaluated; the integer-valued result prints as `5`, not `5.0`.

## Example 4: Division Keeps The Fraction

```cpp
const pluss::Note n = pluss::note::format("{p / q}", {{"p", 10}, {"q", 4}});
std::cout << n << "\n";
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"2.5"
```

Arithmetic is floating point, so `10 / 4` is `2.5`.

## Example 5: String Concatenation

```cpp
pluss::note::print(std::cout, pluss::note::format(
    "{greet + ' ' + who}", {{"greet", "hi"}, {"who", "there"}})) << "\n";
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"hi there"
```

`+` with non-numeric operands joins them; `' '` is a string literal inside
the placeholder.

## Example 6: A Ternary Choice

```cpp
std::cout << pluss::note::print(
    pluss::note::format("{n > 1 ? 'many' : 'one'}", {{"n", 5}})) << "\n";
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"many"
```

## Example 7: Boolean Logic

```cpp
const pluss::Note n = pluss::note::format("{ok && n == 3}",
                                          {{"ok", true}, {"n", 3}});
std::cout << n << "\n";
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"true"
```

`==` compares the stringified operands; the `&&` result stringifies as
`true`.

## Example 8: Indexing And `.size()` On A List Binding

```cpp
std::cout << pluss::note::print(pluss::note::format(
    "first={xs[0]} count={xs.size()}",
    {{"xs", std::vector<std::string>{"a", "b", "c"}}})) << "\n";
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"first=a count=3"
```

## Example 9: A Built-In Function

```cpp
const pluss::Note n = pluss::note::format("{std::sqrt(x)}", {{"x", 16}});
std::cout << n << "\n";
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"4"
```

## Example 10: Variadic `std::min` / `std::max`

```cpp
std::cout << pluss::note::print(pluss::note::format(
    "{std::min(a, b, c)}/{std::max(a, b, c)}",
    {{"a", 4}, {"b", 1}, {"c", 9}})) << "\n";
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"1/9"
```

## Example 11: Joining A List

```cpp
const pluss::Note n = pluss::note::format(
    "{pluss::join(xs, '-')}",
    {{"xs", std::vector<std::string>{"x", "y", "z"}}});
std::cout << n << "\n";
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"x-y-z"
```

Without the second argument, `pluss::join` uses `", "`.

## Example 12: Reducing A Numeric List

```cpp
std::cout << pluss::note::print(pluss::note::format(
    "{std::accumulate(xs, 0)}",
    {{"xs", std::vector<int>{1, 2, 3, 4}}})) << "\n";
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"10"
```

The second argument is the initial value.

## Example 13: A Callable Binding

```cpp
const pluss::Note n = pluss::note::format(
    "{shout('hi')}",
    {{"shout", [](std::string s) {
        for (auto& ch : s) ch = std::toupper(ch);
        return s + "!";
      }}});
std::cout << n << "\n";
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"HI!"
```

The string literal `'hi'` is converted to the lambda's `std::string`
parameter; the return value is spliced in.

## Example 14: A Missing Binding Is Rejected

```cpp
try {
  pluss::note::format("{absent}", {{"x", 1}});
} catch (const pluss::Error& e) {
  const std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
pluss::note::format: missing binding 'absent'
```

## Example 15: An Unmatched Brace Is Rejected

```cpp
try {
  pluss::note::format("{x", {{"x", 1}});
} catch (const pluss::Error& e) {
  const std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
pluss::note::format: unmatched '{' in format template
```

Use `{{` for a literal `{`.

## Example 16: A Duplicate Binding Name Is Rejected

```cpp
try {
  pluss::note::format("{x}", {{"x", 1}, {"x", 2}});
} catch (const pluss::Error& e) {
  const std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
pluss::note::format: duplicate binding 'x'
```

Binding names must be unique.

## See Also

- [`pluss::note::make`](make.md): wrap a string you have already assembled.
- [`pluss::note::select`](select.md): choose among prepared strings by
  boolean conditions instead of rendering one.
- [`pluss::note::from_lines`](from_lines.md) /
  [`pluss::note::from_words`](from_words.md): the inverse direction — take a
  string apart.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="empty_like.html">
    <span class="label">Previous</span>
    <span class="title">empty_like</span>
  </a>
  <a class="next-link" href="from_lines.html">
    <span class="label">Next</span>
    <span class="title">from_lines</span>
  </a>
</div>
