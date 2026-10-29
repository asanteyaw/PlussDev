<!-- pluss-hand-authored -->

# Series::einsum

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
static Series Series::einsum(std::string_view equation, const std::vector<Series>& operands)
```

## What It Does

`einsum()` evaluates an Einstein-summation expression: one equation names
every axis of every operand with a letter, and the letters decide what is
multiplied, what is summed, and in which order the result's axes appear.

```text
"ij,jk->ik"
  |   |  |
  |   |  +-- output axes, in the order you want them
  |   +----- second operand: axis 0 is `j`, axis 1 is `k`
  +--------- first operand:  axis 0 is `i`, axis 1 is `j`
```

The rules are short:

- a letter that appears in the output is **kept** as an output axis
- a letter that appears in the inputs but **not** in the output is **summed
  over**
- a letter that appears in more than one operand pairs those axes
  element-for-element (a product, then a sum if the letter is not kept)

One equation therefore replaces a whole family of transposes, reshapes,
matrix products, traces and reductions, and states the intent in one line.

Because `einsum()` is a composite of `diagonal`, `sum`, `permute`, `reshape`
and `bmm`, it is differentiable in every operand with no extra machinery.
More than two operands are contracted pairwise from left to right, and every
pair is executed as one batched matrix product, so a contraction costs a
matrix multiplication, not a loop over elements.

## Equation Grammar

**Subscripts.** Any ASCII letter, upper or lower case. `A` and `a` are
different axes. Spaces are ignored.

**Explicit mode** (`"ij,jk->ik"`). The text after `->` is the output. Every
output letter must appear in some input, and no letter may repeat in the
output.

**Implicit mode** (`"ij,jk"`, no `->`). The output is every letter that
appears **exactly once** across all the inputs, in alphabetical order
(upper-case letters before lower-case). `"ij,jk"` therefore means
`"ij,jk->ik"`, but `"ji"` means `"ji->ij"` (a transpose), because the
output is alphabetical, not in input order.

**Ellipsis** (`"..."`). Stands for "all the leading axes I do not want to
name". Each operand may use at most one. The ellipsis axes of different
operands are aligned from the right and **broadcast** against each other, so
`"...ij,...jk->...ik"` is a batched matrix product for any batch shape. In
implicit mode the ellipsis axes come first in the output; in explicit mode
they appear wherever `...` appears after `->`. An operand with fewer ellipsis
axes than its partner is treated as if it were padded with size-1 axes.

**Repeated letters inside one operand** (`"ii"`, `"bii->bi"`). The operand is
read along its diagonal: `"ii->i"` extracts the diagonal of a matrix and
`"ii->"` is its trace. The repeated axes must have equal size.

**Size-1 broadcasting.** When two operands share a letter and one of the
axes has size 1, that axis is broadcast against the other.

## Parameters

**equation** (`std::string_view`)
    The subscripts, as described above. The number of comma-separated terms
    must equal `operands.size()`, and each term must name exactly as many axes
    as its operand has (or, if it contains `...`, at most that many).

**operands** (`const std::vector<Series>&`)
    The operands, in the order of the equation's terms. A braced list works:
    `Series::einsum("ij,jk->ik", {a, b})`. Operands should share a dtype.

## Returns

`Series`
    The contraction, with axes in the equation's output order. If the output
    names no axes the result is a scalar `Series`.

## Gradient

`einsum()` is differentiable in every operand. The gradient of each operand
is itself an einsum with that operand's term and the output term swapped, and
it is produced by the reverse rules of the underlying `bmm`, `sum`,
`permute` and `diagonal`. Operands that are not tracked simply receive no
sensitivity.

## Examples

Example 1. The matrix product, written explicitly.

```cpp
auto a = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
auto b = pluss::make_series({{1.0, 0.0}, {0.0, 1.0}, {1.0, 1.0}});

std::cout << pluss::Series::einsum("ij,jk->ik", {a, b});
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[4, 5],
  [10, 11]]
```

`j` appears in both inputs and not in the output, so it is the summed axis: `out[i,k] = sum_j a[i,j] * b[j,k]`.

Example 2. Implicit mode drops the `->`; the output is the letters seen once, alphabetically.

```cpp
auto a = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
auto b = pluss::make_series({{1.0, 0.0}, {0.0, 1.0}, {1.0, 1.0}});

std::cout << pluss::Series::einsum("ij,jk", {a, b}) << "\n";
// Implicit output is alphabetical, so "ji" is a transpose, not a copy:
std::cout << pluss::Series::einsum("ji", {a});
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[4, 5],
  [10, 11]]
pluss::Series(shape=[3,2], strides=[1,3], dtype=float64, platform=cpu, numel=6, contiguous=false)
data=[[1, 4],
  [2, 5],
  [3, 6]]
```

The first call equals Example 1. In the second, the letters `i` and `j` each appear once and are ordered `i, j`, so the 2 x 3 input `a[j,i]` comes back as a 3 x 2 matrix.

Example 3. Transposing and summing: a letter missing from the output is summed away.

```cpp
auto a = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});

std::cout << pluss::Series::einsum("ij->ji", {a}) << "\n";   // transpose
std::cout << pluss::Series::einsum("ij->i", {a}) << "\n";    // row sums
std::cout << pluss::Series::einsum("ij->", {a});              // grand total
```

### Output

```text
pluss::Series(shape=[3,2], strides=[1,3], dtype=float64, platform=cpu, numel=6, contiguous=false)
data=[[1, 4],
  [2, 5],
  [3, 6]]
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[6, 15]
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=21
```

With nothing after `->` every axis is summed and the result is a scalar `Series`.

Example 4. A repeated letter inside one operand reads its diagonal.

```cpp
auto m = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}, {7.0, 8.0, 9.0}});

std::cout << pluss::Series::einsum("ii->i", {m}) << "\n";   // diagonal
std::cout << pluss::Series::einsum("ii->", {m});             // trace
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 5, 9]
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=15
```

The repeated axes must have the same length; a rectangular matrix with `"ii"` is an error (Example 10).

Example 5. Dot products and outer products are one equation each.

```cpp
auto u = pluss::make_series({1.0, 2.0, 3.0});
auto v = pluss::make_series({4.0, 5.0, 6.0});

std::cout << pluss::Series::einsum("i,i->", {u, v}) << "\n";    // dot
std::cout << pluss::Series::einsum("i,j->ij", {u, v});           // outer
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=32
pluss::Series(shape=[3,3], strides=[3,1], dtype=float64, platform=cpu, numel=9, contiguous=true)
data=[[4, 5, 6],
  [8, 10, 12],
  [12, 15, 18]]
```

`i` is shared and summed in the first call; in the second the letters are different, so nothing is summed and every pair of elements is multiplied.

Example 6. A batched matrix product keeps the batch letter in the output.

```cpp
auto a = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0}, {2, 2, 2});
auto b = pluss::make_series({1.0, 0.0, 0.0, 1.0, 2.0, 0.0, 0.0, 2.0}, {2, 2, 2});

std::cout << pluss::Series::einsum("bij,bjk->bik", {a, b});
```

### Output

```text
pluss::Series(shape=[2,2,2], strides=[4,2,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[[1, 2],
  [3, 4]],
  [[10, 12],
  [14, 16]]]
```

Batch 0 is multiplied by the identity and batch 1 by twice the identity, so batch 0 is unchanged and batch 1 is doubled.

Example 7. An ellipsis stands for any leading batch axes and broadcasts them.

```cpp
auto batch = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0}, {2, 2, 2});
auto shared = pluss::make_series({{1.0, 1.0}, {0.0, 1.0}});

// `shared` has no batch axis: its ellipsis is empty and broadcasts against batch's.
std::cout << pluss::Series::einsum("...ij,jk->...ik", {batch, shared});
```

### Output

```text
pluss::Series(shape=[2,2,2], strides=[4,2,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[[1, 3],
  [3, 7]],
  [[5, 11],
  [7, 15]]]
```

The same equation works for any number of leading batch axes, including none.

Example 8. More than two operands are contracted pairwise, left to right.

```cpp
auto a = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});
auto b = pluss::make_series({{0.0, 1.0}, {1.0, 0.0}});
auto c = pluss::make_series({{1.0, 0.0}, {0.0, 2.0}});

// trace(a * b * c)
std::cout << pluss::Series::einsum("ij,jk,ki->", {a, b, c});
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=8
```

The pairing order is `(a, b)` first, then the result with `c`. Pick the operand order so that the early pairs are the small ones.

Example 9. The gradient flows to every tracked operand.

```cpp
auto a = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});
auto b = pluss::make_series({{5.0, 6.0}, {7.0, 8.0}});
a.tracks_slope_(true);
b.tracks_slope_(true);

pluss::Series::einsum("ij,jk->", {a, b}).reverse();

std::cout << a.sensitivity() << "\n" << b.sensitivity();
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[11, 15],
  [11, 15]]
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[4, 4],
  [6, 6]]
```

`sum_ik (a b)[i,k]` has gradient `row_sum(b)` in each column of `a` and `col_sum(a)` in each row of `b`.

Example 10. Malformed equations fail with a message naming the problem.

```cpp
auto a = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
auto b = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}, {5.0, 6.0}});

auto first_line = [](const std::exception& e) {
  std::string msg = e.what();
  return msg.substr(0, msg.find('\n'));
};
try { pluss::Series::einsum("ii", {a}); }
catch (const std::exception& e) { std::cout << first_line(e) << "\n"; }
try { pluss::Series::einsum("ij,jk->ik", {a}); }
catch (const std::exception& e) { std::cout << first_line(e) << "\n"; }
try { pluss::Series::einsum("ij,jk->iz", {a, b}); }
catch (const std::exception& e) { std::cout << first_line(e) << "\n"; }
try { pluss::Series::einsum("ij,jk->ik", {a, a}); }
catch (const std::exception& e) { std::cout << first_line(e) << "\n"; }
```

### Output

```text
einsum: repeated subscript must index axes of equal size
einsum: number of operands does not match the equation
einsum: output subscript does not appear in any input
einsum: operands do not broadcast for a shared subscript
```

## Gotchas And Failure Modes

- Implicit mode orders the output **alphabetically**, not by position:
  `"ba"` is a transpose and `"ab"` is a copy. Write `->` whenever the order
  matters.
- Every output letter must occur in an input, an output letter may not repeat,
  and a repeated input letter must index axes of equal length. Violations
  throw.
- The number of terms must match the number of operands, and a term without
  `...` must name exactly the operand's rank.
- A shared letter whose axes have different lengths is an error unless one of
  them is 1.
- Operands are contracted left to right. `"ij,jk,kl->il"` is cheapest when
  `a` and `b` are the small pair; reorder the operands (and their terms) if
  an early pair would create a large intermediate.
- Operands must share a dtype; convert with `to()` first if they differ.

## Summary

`Series::einsum(equation, operands)` is a general contraction:

- letters kept in the output stay as axes, other letters are summed
- implicit mode sorts the output alphabetically; explicit mode (`->`) is
  exact
- `...` broadcasts leading batch axes; repeated letters in one operand read a
  diagonal
- composite of `diagonal`, `sum`, `permute`, `reshape` and `bmm`, so it is
  differentiable everywhere

## See Also

- [`Series::tensordot`](tensordot.md): contract chosen axis pairs of two
  operands.
- [`Series::bmm`](bmm.md): the strict batched matrix product `einsum` lowers
  to.
- [`Series::diagonal`](diagonal.md) and [`Series::trace`](trace.md): the
  single-operand diagonal cases.
- [`Series::LinearAlgAccessor::matmul`](../series-linearalg/matmul.md): the
  rank-polymorphic matrix product.
