#include <pluss/pluss.h>
#include <pluss/tils/suppose.h>

#include <iostream>

int main() {
  using pluss::Series;
  using pluss::tils::suppose;

  auto values = pluss::make_series({2.0, 4.0, 6.0});

  // A named algorithm computes and retains its intermediate Pluss values.
  auto algorithm = suppose(values)
      .then("total", [](const Series& x) { return x.sum(); })
      .then("mean", [](const Series& x, const Series&) { return x.mean(); })
      .finish();
  std::cout << "algorithm: " << algorithm.derived<0>()->item<double>()
            << ", " << algorithm.derived<1>()->item<double>() << '\n';

  // A checked run links a derived fact to a conclusion for this input.
  auto checked = suppose("three observations", values,
      [](const Series& x) { return x.numel() == 3; })
      .then("total is twelve", [](const Series& x) { return x.sum(); },
            [](const Series&, const Series& total) {
              return total.item<double>() == 12.0;
            })
      .conclude("mean is positive", [](const Series& x, const Series& total) {
        return total.item<double>() / x.numel() > 0.0;
      });
  std::cout << "checked: " << std::boolalpha << checked.verified() << '\n';

  // An outline records reasoning without pretending that the reasons are proved.
  auto outline = suppose.outline<Series>("variance is nonnegative");
  outline.given("real observations")
      .claim("squares are nonnegative", "every real square is nonnegative")
      .claim("their mean is nonnegative", "mean preserves nonnegativity")
      .goal("variance is nonnegative");
  std::cout << "outline: " << outline.nodes().size() << " steps\n";

  // A blueprint tracks dependencies and identifies ready work.
  auto roadmap = suppose.blueprint<Series>("variance computation");
  roadmap.given("observations").step("compute mean").step("center values").goal("variance");
  std::cout << "roadmap next: " << roadmap.nodes()[roadmap.ready().front()].name << '\n';

  // The proof kernel checks propositional inference, here (P and Q) implies P.
  const auto P = suppose.atom<Series>("P");
  const auto Q = suppose.atom<Series>("Q");
  auto proof = suppose.proof<Series>();
  const auto both = proof.assume(P.and_(Q));
  const auto first = proof.and_left(both);
  const auto theorem = proof.discharge(both, first);
  const auto certificate = proof.conclude(theorem, P.and_(Q).implies(P));
  std::cout << "formal theorem: " << certificate.unconditional() << '\n';
}
