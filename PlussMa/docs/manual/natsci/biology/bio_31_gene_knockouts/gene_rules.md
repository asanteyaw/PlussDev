# Gene rules and deletions

[Back to Biology 31: Gene Knockouts](../bio_31_gene_knockouts.md)

You know which genes code for the enzymes of a network and you want to know what the cell can still do when some of them are deleted. A reaction is not removed by deleting one gene when an isozyme can replace it, and a complex stops when any one of its subunits is missing. This page writes the gene-protein-reaction rules as text, finds which reactions survive a deletion, and solves the knocked-out network for the whole set of deletions in a single call.

## How it works

A gene-protein-reaction (GPR) rule is a logical formula in the gene names: `g1 and g2` is a complex that needs both genes, `g1 or g2` is a pair of isozymes of which either suffices, and parentheses group. A reaction is *active* when its rule is true given which genes are present, and an empty rule means that no gene is needed. `metabolism::GeneRules(rules)` takes one rule string for each reaction (the genes are collected in order of appearance, or you can pass the list of names as a second argument); `and` binds tighter than `or`, `&`, `&&`, `|` and `||` work as well, and capitals do not matter for the operators. `rules.active(present)` takes a table `[K, genes]` with 1 for a gene that is present and 0 for a deleted one and returns the table `[K, reactions]` of which reactions are active, and `rules.reaction_mask(knocked)` returns the same with the roles reversed, 1 for a reaction that is forced to zero flux. `metabolism::gene_knockouts(net, rules, objective, knocked)` deletes each row of `knocked` `[K, genes]` and solves the flux-balance problem of the network for every row in one batched call, returning the objective `objective`, the solver `status` and the `ratio` to the wild type; `single_gene_knockouts` deletes each gene in turn. All programs use the bypass network of [Biology 4](../bio_04_metabolism/flux_balance.md): uptake into A, A to B directly or through C, and B to biomass, with the reactions governed by the rules `g1`, `g2`, `g3 and g4`, `g3 or g5` and none for biomass.

## Which reactions survive

The program builds the rules, prints the genes in the order they were found, and shows which of the five reactions are active in the wild type and with `g3`, `g4` or `g5` deleted.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace metabolism = biology::metabolism;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
// metabolites A, B, C; reactions: 0 uptake -> A, 1 A -> B, 2 A -> C, 3 C -> B, 4 B -> biomass
static metabolism::Network bypass() {
  metabolism::Network net;
  net.stoichiometry = mat({{1.0, -1.0, -1.0, 0.0, 0.0}, {0.0, 1.0, 0.0, 1.0, -1.0}, {0.0, 0.0, 1.0, -1.0, 0.0}});
  net.lower = vec({0.0, 0.0, 0.0, 0.0, 0.0});
  net.upper = vec({10.0, 6.0, 100.0, 8.0, 100.0});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // each reaction has a gene rule: "g3 and g4" is an enzyme complex that needs both genes, "g3 or g5" is a pair of isozymes of which one suffices,
  // and an empty rule means no gene is needed. Reactions: uptake, A->B, A->C, C->B, biomass
  const metabolism::GeneRules rules({"g1", "g2", "g3 and g4", "g3 or g5", ""});
  out.writeln("genes found, in order of appearance ", std::vector<double>{static_cast<double>(rules.gene_count())});
  for (const auto &g : rules.genes()) out.writeln("   ", g);
  // which reactions work when one gene is deleted? present = 1 means the gene is there
  const auto present = pluss::series::make({{1.0, 1.0, 1.0, 1.0, 1.0}, {1.0, 1.0, 0.0, 1.0, 1.0}, {1.0, 1.0, 1.0, 0.0, 1.0}, {1.0, 1.0, 1.0, 1.0, 0.0}}).to(pluss::ScalarType::Float64);
  const auto active = rules.active(present);
  out.writeln("activity of the five reactions, wild type          ", flat(active.select(0, 0)));
  out.writeln("activity with g3 deleted (complex and isozyme)     ", flat(active.select(0, 1)));
  out.writeln("activity with g4 deleted (the complex fails)       ", flat(active.select(0, 2)));
  out.writeln("activity with g5 deleted (the isozyme is covered)  ", flat(active.select(0, 3)));
  out.writeln("reaction mask for g3 deleted (1 = forced to zero)  ", flat(rules.reaction_mask(pluss::series::make({{0.0, 0.0, 1.0, 0.0, 0.0}}).to(pluss::ScalarType::Float64))));
}
```

Output

```text
genes found, in order of appearance [5.0000]
   g1
   g2
   g3
   g4
   g5
activity of the five reactions, wild type          [1.0000, 1.0000, 1.0000, 1.0000, 1.0000]
activity with g3 deleted (complex and isozyme)     [1.0000, 1.0000, 0.0000, 1.0000, 1.0000]
activity with g4 deleted (the complex fails)       [1.0000, 1.0000, 0.0000, 1.0000, 1.0000]
activity with g5 deleted (the isozyme is covered)  [1.0000, 1.0000, 1.0000, 1.0000, 1.0000]
reaction mask for g3 deleted (1 = forced to zero)  [0.0000, 0.0000, 1.0000, 0.0000, 0.0000]
```

The five genes are found in the order `g1` to `g5`. In the wild type all five reactions are active. Deleting `g3` stops the complex `g3 and g4` (reaction 2), but reaction 3, `g3 or g5`, survives on `g5`. Deleting `g4` does the same to the complex, since a missing subunit is as bad as a missing enzyme. Deleting `g5` alone changes nothing: `g3` covers the isozyme. The mask is the complement of the activity: with `g3` deleted only reaction 2 is forced to zero.

## Every single deletion, then pairs

The program solves all five single-gene deletions at once, then two chosen pairs, and prints the biomass and its ratio to the wild type for each.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace metabolism = biology::metabolism;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
// metabolites A, B, C; reactions: 0 uptake -> A, 1 A -> B, 2 A -> C, 3 C -> B, 4 B -> biomass
static metabolism::Network bypass() {
  metabolism::Network net;
  net.stoichiometry = mat({{1.0, -1.0, -1.0, 0.0, 0.0}, {0.0, 1.0, 0.0, 1.0, -1.0}, {0.0, 0.0, 1.0, -1.0, 0.0}});
  net.lower = vec({0.0, 0.0, 0.0, 0.0, 0.0});
  net.upper = vec({10.0, 6.0, 100.0, 8.0, 100.0});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto net = bypass();
  const auto biomass = vec({0.0, 0.0, 0.0, 0.0, 1.0});
  const metabolism::GeneRules rules({"g1", "g2", "g3 and g4", "g3 or g5", ""});
  // all single-gene deletions in one batched call: one lane per gene
  const auto single = metabolism::single_gene_knockouts(net, rules, biomass);
  for (std::size_t i = 0; i < rules.genes().size(); ++i)
    out.writeln("delete ", rules.genes()[i], "   biomass ", std::vector<double>{flat(single.objective)[i]}, "  fraction ", std::vector<double>{flat(single.ratio)[i]});
  // chosen sets of genes: g2 + g3 together, and g3 + g5 together (the two bypass enzymes and their backup)
  const auto sets = pluss::series::make({{0.0, 1.0, 1.0, 0.0, 0.0}, {0.0, 0.0, 1.0, 0.0, 1.0}}).to(pluss::ScalarType::Float64);
  const auto pair = metabolism::gene_knockouts(net, rules, biomass, sets);
  out.writeln("delete g2 and g3 together, biomass ", std::vector<double>{flat(pair.objective)[0]}, "  fraction ", std::vector<double>{flat(pair.ratio)[0]});
  out.writeln("delete g3 and g5 together, biomass ", std::vector<double>{flat(pair.objective)[1]}, "  fraction ", std::vector<double>{flat(pair.ratio)[1]});
}
```

Output

```text
delete g1   biomass [0.0000]  fraction [0.0000]
delete g2   biomass [8.0000]  fraction [0.8000]
delete g3   biomass [6.0000]  fraction [0.6000]
delete g4   biomass [6.0000]  fraction [0.6000]
delete g5   biomass [10.0000]  fraction [1.0000]
delete g2 and g3 together, biomass [0.0000]  fraction [0.0000]
delete g3 and g5 together, biomass [6.0000]  fraction [0.6000]
```

Deleting `g1` removes the uptake and biomass is `0`. Deleting `g2` loses the direct route but the detour through C still carries `8`, a ratio of `0.8`. Deleting `g3` or `g4` loses the detour and the direct route carries `6`, a ratio of `0.6`, and deleting `g5` changes nothing (`1.0`). Deleting both bypass routes, `g2` and `g3` together, leaves nothing (`0`): neither deletion is lethal alone, and the pair is, a synthetic lethal pair. Deleting `g3` and `g5` together costs no more than `g3` alone, `6`.

## The rule grammar

The program writes five different rules with the operators in different spellings, uses three genes, and prints which reactions are active for all eight combinations of the genes being present or absent.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace metabolism = biology::metabolism;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
// metabolites A, B, C; reactions: 0 uptake -> A, 1 A -> B, 2 A -> C, 3 C -> B, 4 B -> biomass
static metabolism::Network bypass() {
  metabolism::Network net;
  net.stoichiometry = mat({{1.0, -1.0, -1.0, 0.0, 0.0}, {0.0, 1.0, 0.0, 1.0, -1.0}, {0.0, 0.0, 1.0, -1.0, 0.0}});
  net.lower = vec({0.0, 0.0, 0.0, 0.0, 0.0});
  net.upper = vec({10.0, 6.0, 100.0, 8.0, 100.0});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // the rule grammar: "and", "or", parentheses and the symbols & && | ||, any capitalisation; "and" binds tighter than "or"
  const metabolism::GeneRules rules({"a & b", "a && (b || c)", "a AND b OR c", "(a or b) and c", "a | b and c"}, {"a", "b", "c"});
  out.writeln("genes ", std::vector<double>{static_cast<double>(rules.gene_count())});
  // all eight combinations of three genes present/absent
  std::vector<double> rows;
  for (int bits = 0; bits < 8; ++bits)
    for (int g = 0; g < 3; ++g) rows.push_back((bits >> g) & 1 ? 1.0 : 0.0);
  const auto present = pluss::make_series(rows, {8, 3}).to(pluss::ScalarType::Float64);
  const auto active = rules.active(present);
  for (int bits = 0; bits < 8; ++bits)
    out.writeln("a,b,c present = ", std::vector<double>{static_cast<double>(bits & 1), static_cast<double>((bits >> 1) & 1), static_cast<double>((bits >> 2) & 1)}, "  active reactions ", flat(active.select(0, bits)));
}
```

Output

```text
genes [3.0000]
a,b,c present = [0.0000, 0.0000, 0.0000]  active reactions [0.0000, 0.0000, 0.0000, 0.0000, 0.0000]
a,b,c present = [1.0000, 0.0000, 0.0000]  active reactions [0.0000, 0.0000, 0.0000, 0.0000, 1.0000]
a,b,c present = [0.0000, 1.0000, 0.0000]  active reactions [0.0000, 0.0000, 0.0000, 0.0000, 0.0000]
a,b,c present = [1.0000, 1.0000, 0.0000]  active reactions [1.0000, 1.0000, 1.0000, 0.0000, 1.0000]
a,b,c present = [0.0000, 0.0000, 1.0000]  active reactions [0.0000, 0.0000, 1.0000, 0.0000, 0.0000]
a,b,c present = [1.0000, 0.0000, 1.0000]  active reactions [0.0000, 1.0000, 1.0000, 1.0000, 1.0000]
a,b,c present = [0.0000, 1.0000, 1.0000]  active reactions [0.0000, 0.0000, 1.0000, 1.0000, 1.0000]
a,b,c present = [1.0000, 1.0000, 1.0000]  active reactions [1.0000, 1.0000, 1.0000, 1.0000, 1.0000]
```

The rules `a & b`, `a && (b || c)`, `a AND b OR c`, `(a or b) and c` and `a | b and c` are all accepted. Reading the table: with only `a` present just rule 5, `a | b and c`, holds, which is `a or (b and c)` and shows that `and` binds tighter than `or`; with `a` and `b` present the rule `a AND b OR c` is true, with only `c` it is true as well, which is the `or c` part; and with all three present every reaction is active.

## Decisions

- Write the rules as the genome annotation gives them, and let the library parse them: the parsing is done once and applying it to thousands of deletions costs only elementwise operations.
- Use `single_gene_knockouts` for a screen and `gene_knockouts` with your own table for pairs, triples or a chosen strain: both are one batched call whatever the number of rows.
- Give the list of gene names yourself when you need a fixed column order, for example to line up with an experimental table.
- Read the `ratio` rather than the raw objective when you compare networks, and the `status` before you trust a lane.

## Pitfalls

Rules that cannot be parsed, rules that do not cover every reaction and tables of the wrong width are refused. The program tries each.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace metabolism = biology::metabolism;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
// metabolites A, B, C; reactions: 0 uptake -> A, 1 A -> B, 2 A -> C, 3 C -> B, 4 B -> biomass
static metabolism::Network bypass() {
  metabolism::Network net;
  net.stoichiometry = mat({{1.0, -1.0, -1.0, 0.0, 0.0}, {0.0, 1.0, 0.0, 1.0, -1.0}, {0.0, 0.0, 1.0, -1.0, 0.0}});
  net.lower = vec({0.0, 0.0, 0.0, 0.0, 0.0});
  net.upper = vec({10.0, 6.0, 100.0, 8.0, 100.0});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // malformed rules and mismatched inputs are rejected
  try {
    metabolism::GeneRules bad({"g1 and"});
  } catch (const pluss::Error &e) {
    out.writeln("incomplete rule: ", e.message());
  }
  try {
    metabolism::GeneRules bad({"g1 g2"});
  } catch (const pluss::Error &e) {
    out.writeln("two genes with no operator: ", e.message());
  }
  const auto net = bypass();
  const metabolism::GeneRules short_rules({"g1", "g2"});
  try {
    metabolism::single_gene_knockouts(net, short_rules, vec({0.0, 0.0, 0.0, 0.0, 1.0}));
  } catch (const pluss::Error &e) {
    out.writeln("rules for two of five reactions: ", e.message());
  }
  const metabolism::GeneRules rules({"g1", "g2", "g3 and g4", "g3 or g5", ""});
  try {
    rules.active(pluss::series::make({{1.0, 1.0, 1.0}}).to(pluss::ScalarType::Float64));
  } catch (const pluss::Error &e) {
    out.writeln("three genes given for five: ", e.message());
  }
}
```

Output

```text
incomplete rule: metabolism: malformed gene rule 'g1 and'
two genes with no operator: metabolism: trailing text in gene rule 'g1 g2'
rules for two of five reactions: natsci: gene_knockouts: rules must cover every reaction
three genes given for five: natsci: GeneRules::active: present must be [K, genes]
```

An incomplete rule such as `g1 and` and two genes with no operator are both refused, with the rule quoted. A list of rules for two of five reactions is refused because the rules must cover every reaction, with an empty string for a reaction that needs no gene, and a table with three columns for five genes is refused because it must be `[K, genes]`.

## Where next

Continue with [Biology 4: Metabolism](../bio_04_metabolism.md) for flux balance and its single-reaction knockouts, and with [Biology 32: The Flux Space](../bio_32_flux_space.md) for the whole set of pathways and for sampling the fluxes the network allows.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
