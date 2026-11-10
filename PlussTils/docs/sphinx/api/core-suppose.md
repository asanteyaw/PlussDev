# `pluss::tils::suppose`

[Back to Core Utilities](core.md) · [Value pipelines](core-flow.md)

`suppose` gives Pluss computations and mathematical arguments a common vocabulary: start with a value or a premise, name the steps, and inspect what follows. Its modes make different claims:

| Mode | What it does | What it does **not** claim |
| --- | --- | --- |
| Computation | Runs named derivations and keeps their Pluss results. | That a mathematical proposition is true. |
| Checked run | Checks a premise, derived relationships, and a conclusion for one input. | That the conclusion holds for every input. |
| Proof outline | Records claims and reasons in order. | That the reasons have been verified. |
| Blueprint | Tracks steps, dependencies, and what can proceed next. | That a pending step has been completed. |
| Proof checker | Checks a small set of propositional inference rules. | Arithmetic, quantifiers, or universal properties of Series. |

The value and each computed derivation must be a supported Pluss type. `Series`, `Note`, `Timeline`, `Duration`, `Queue`, and the public PlussMic value types are supported. The blueprint, outline, and proof checker are tagged with a Pluss type but do not need a concrete value. Native C++ values are outside this API.

## Example 1: an algorithm with named results

```cpp
#include <pluss/pluss.h>
#include <pluss/tils/suppose.h>

int main() {
  auto x = pluss::make_series({2.0, 4.0, 6.0});
  auto run = pluss::tils::suppose(x)
      .then("sum", [](const pluss::Series& x) { return x.sum(); })
      .then("mean", [](const pluss::Series& x, const pluss::Series&) {
        return x.mean();
      })
      .finish();

  auto total = run.derived<0>()->item<double>(); // 12
  auto average = run.derived<1>()->item<double>(); // 4
  return total == 12.0 && average == 4.0 ? 0 : 1;
}
```

`finish()` reports `completed()`, not `verified()`: these steps computed values without checking a mathematical claim. A two-argument `then` receives the original value and all earlier derived values. Later steps can reuse them. The results are owned by the returned object and are available through `derived<index>()`.

## Example 2: a checked run

```cpp
#include <pluss/pluss.h>
#include <pluss/tils/suppose.h>

int main() {
  auto x = pluss::make_series({2.0, 4.0, 6.0});
  auto result = pluss::tils::suppose("three observations", x,
      [](const pluss::Series& x) { return x.numel() == 3; })
      .then("sum is twelve",
        [](const pluss::Series& x) { return x.sum(); },
        [](const pluss::Series&, const pluss::Series& sum) {
          return sum.item<double>() == 12.0;
        })
      .then("mean agrees with sum",
        [](const pluss::Series& x, const pluss::Series&) { return x.mean(); },
        [](const pluss::Series& x, const pluss::Series& sum,
           const pluss::Series& mean) {
          return mean.item<double>() * x.numel() == sum.item<double>();
        })
      .conclude("mean is positive",
        [](const pluss::Series&, const pluss::Series&,
           const pluss::Series& mean) { return mean.item<double>() > 0.0; });
  return result.verified() ? 0 : 1;
}
```

For this (x), the premise is “(x) has three observations.” The first derivation yields (s=12). The second yields (m=4) and checks (3m=s). The conclusion checks (m>0). The verifier for each `then` receives the original value, earlier facts, and its new fact. `conclude` receives the original and every fact. If the premise or a check fails, subsequent callbacks are skipped. `status()`, `steps()`, and `failed_step()` explain the outcome. A skipped derivation has an empty `derived<index>()` optional.

## Example 3: a proof outline

```cpp
#include <pluss/pluss.h>
#include <pluss/tils/suppose.h>

int main() {
  auto outline = pluss::tils::suppose.outline<pluss::Series>("variance is nonnegative");
  outline.given("real observations")
      .claim("squared deviations are nonnegative", "a real square is nonnegative")
      .claim("their mean is nonnegative", "a mean of nonnegative reals is nonnegative")
      .goal("variance is nonnegative");
  return outline.nodes().size() == 4 ? 0 : 1;
}
```

This records the reasoning and its missing proof obligations. The strings are **not** machine-checked. An outline can guide a write-up or future formal rules without masquerading as a proof certificate.

## Example 4: a dependency blueprint

```cpp
#include <pluss/pluss.h>
#include <pluss/tils/suppose.h>

int main() {
  auto plan = pluss::tils::suppose.blueprint<pluss::Series>("variance computation");
  plan.given("observations")
      .step("compute mean")
      .step("center values")
      .goal("variance");
  auto next = plan.ready(); // ID of "compute mean"
  plan.mark_done(next.front());
  return plan.ready().front() == 2 ? 0 : 1;
}
```

Each one-argument `step`, `claim`, or `goal` depends on the preceding node. Use `step_after(name, {ids})`, `claim_after(name, reason, {ids})`, or `goal_after(name, {ids})` for branches and joins. `ready()` lists pending nodes whose dependencies are done. `mark_done(id)` rejects a step with incomplete dependencies. A blueprint records progress; it does not execute the algorithm or validate proof claims.

## Example 5: a checked logical proof

The proof checker handles **propositional logic** over statements about a Pluss type. Here it proves the unconditional theorem ((P\land Q)\Rightarrow P):

```cpp
#include <pluss/pluss.h>
#include <pluss/tils/suppose.h>

int main() {
  auto P = pluss::tils::suppose.atom<pluss::Series>("x is nonempty");
  auto Q = pluss::tils::suppose.atom<pluss::Series>("x is finite");
  auto proof = pluss::tils::suppose.proof<pluss::Series>();
  auto both = proof.assume(P.and_(Q));
  auto first = proof.and_left(both);
  auto theorem = proof.discharge(both, first);
  auto certificate = proof.conclude(theorem, P.and_(Q).implies(P));
  return certificate.unconditional() ? 0 : 1;
}
```

The checker also provides `and_right`, `and_intro`, and `modus_ponens`. It checks the formula structure, tracks which assumptions a line depends on, and rejects a conclusion that does not match the derived line. `discharge` turns a conclusion reached under an assumption into an implication and removes that dependency. An `assume` call does not establish that its statement is true of real data: a certificate with remaining assumptions is conditional. This kernel proves logical consequences, not numerical theorems such as variance nonnegativity. Those require sound, explicitly implemented arithmetic rules.

## Cost and errors

For computations, an lvalue input is borrowed by const reference and an rvalue is moved. Derived Pluss values are moved into the result. Callbacks are templated and invoked directly, without `std::function`; the named trace still stores strings in a vector and therefore has bookkeeping cost. `flow(value)` is simpler when no named reasoning is needed. Empty names, invalid dependencies, and invalid proof inferences throw standard exceptions. Callback exceptions propagate.
