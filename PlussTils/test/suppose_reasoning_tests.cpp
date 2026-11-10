#include <pluss/tils/suppose.h>

#include <stdexcept>
#include <string>
#include <vector>

namespace pluss::codem {
// A small, move-only stand-in keeps this header-only test independent of CodeM.
class Series {
 public:
  explicit Series(int value) : value_(value) {}
  Series(const Series&) = delete;
  Series& operator=(const Series&) = delete;
  Series(Series&&) = default;
  Series& operator=(Series&&) = default;
  int value() const { return value_; }
  Series scaled(int factor) const { return Series(value_ * factor); }

 private:
  int value_;
};
}  // namespace pluss::codem

namespace {
void expect(bool condition) {
  if (!condition) throw std::runtime_error("suppose reasoning test failed");
}

template <class T>
concept CanSuppose = requires(T& value) { pluss::tils::suppose(value); };
static_assert(CanSuppose<pluss::codem::Series>);
static_assert(!CanSuppose<int>);
static_assert(!CanSuppose<std::vector<int>>);
}  // namespace

int main() {
  using pluss::codem::Series;
  using pluss::tils::suppose;

  Series input(4);
  auto algorithm = suppose(input)
      .then("triple", [](const Series& x) { return x.scaled(3); })
      .then("double the triple", [](const Series&, const Series& triple) {
        return triple.scaled(2);
      })
      .finish();
  expect(algorithm.completed() && !algorithm.verified());
  expect(algorithm.derived<0>()->value() == 12);
  expect(algorithm.derived<1>()->value() == 24);
  expect(algorithm.steps()[1].status == pluss::tils::SuppositionStepStatus::computed);

  int calls = 0;
  auto checked = suppose("input is four", input,
      [](const Series& x) { return x.value() == 4; })
      .then("triple is twelve",
          [&](const Series& x) { ++calls; return x.scaled(3); },
          [&](const Series&, const Series& triple) {
            ++calls; return triple.value() == 12;
          })
      .conclude("positive", [&](const Series&, const Series& triple) {
        ++calls; return triple.value() > 0;
      });
  expect(checked.verified() && checked.completed() && calls == 3);

  auto absent = suppose("input is five", input,
      [](const Series& x) { return x.value() == 5; })
      .then("never computed", [&](const Series& x) { ++calls; return x.scaled(3); })
      .conclude("never checked", [&](const Series&, const Series&) {
        ++calls; return true;
      });
  expect(!absent.applicable() && calls == 3 && !absent.derived<0>());
  expect(absent.steps()[1].status == pluss::tils::SuppositionStepStatus::skipped);

  auto failed = suppose(input)
      .then("wrong relation", [](const Series& x) { return x.scaled(3); },
            [](const Series&, const Series& triple) { return triple.value() == 13; })
      .then("skipped", [&](const Series&, const Series& triple) {
        ++calls; return triple.scaled(2);
      })
      .finish();
  expect(failed.status() == pluss::tils::SuppositionStatus::derivation_failed);
  expect(failed.failed_step() == "wrong relation" && calls == 3);
  expect(failed.derived<0>()->value() == 12 && !failed.derived<1>());

  auto plan = suppose.blueprint<Series>("variance computation");
  plan.given("observations").step("compute mean").step("center values").goal("variance");
  expect(plan.ready() == std::vector<std::size_t>{1});
  bool blocked = false;
  try { plan.mark_done(2); } catch (const std::logic_error&) { blocked = true; }
  expect(blocked);
  plan.mark_done(1);
  expect(plan.ready() == std::vector<std::size_t>{2});
  const auto independent = plan.step_after("inspect dtype", {});
  expect(plan.nodes()[independent].dependencies.empty());

  auto outline = suppose.outline<Series>("why variance is nonnegative");
  outline.given("real observations")
      .claim("squares are nonnegative", "each real square is nonnegative")
      .claim("their mean is nonnegative", "a mean of nonnegative reals is nonnegative")
      .goal("variance is nonnegative");
  expect(outline.nodes()[1].kind == pluss::tils::BlueprintKind::claim);
  expect(!outline.nodes()[3].done);

  const auto p = suppose.atom<Series>("P");
  const auto q = suppose.atom<Series>("Q");
  auto proof = suppose.proof<Series>();
  const auto both = proof.assume(p.and_(q));
  const auto first = proof.and_left(both);
  const auto theorem = proof.discharge(both, first);
  const auto certificate = proof.conclude(theorem, p.and_(q).implies(p));
  expect(certificate.unconditional());
  const auto conditional = proof.conclude(first, p);
  expect(conditional.assumptions().size() == 1);
  bool invalid = false;
  try { static_cast<void>(proof.conclude(first, q)); }
  catch (const std::logic_error&) { invalid = true; }
  expect(invalid);
  const auto implies = proof.assume(p.implies(q));
  const auto given_p = proof.assume(p);
  const auto consequence = proof.modus_ponens(given_p, implies);
  expect(proof.conclude(consequence, q).assumptions().size() == 2);
}
