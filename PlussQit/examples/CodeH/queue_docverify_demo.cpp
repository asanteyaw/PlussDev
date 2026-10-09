// Real, compiled, run output for every pluss::Queue reference page.
// Each section below is copied verbatim into its matching page under
// docs/sphinx/api/queue/.
#include <pluss/pluss.h>
#include <pluss/queue.h>
#include <pluss/codeh/codeh.h>

#include <iostream>
#include <string>

int main() {
  std::cout << std::boolalpha;

  // ===================== construction =====================
  std::cout << "=== construction ===\n";
  {
    pluss::Queue from_series(pluss::make_series({1.0, 2.0, 3.0}));
    pluss::Queue from_note(pluss::make_note({"a", "b"}));
    pluss::Queue from_timeline(pluss::timeline::make("1999-04-17T21:15:23Z"));
    pluss::Queue from_duration(pluss::duration::days(3));
    std::cout << "from_series.kind_name()=" << from_series.kind_name() << '\n';
    std::cout << "from_note.kind_name()=" << from_note.kind_name() << '\n';
    std::cout << "from_timeline.kind_name()=" << from_timeline.kind_name() << '\n';
    std::cout << "from_duration.kind_name()=" << from_duration.kind_name() << '\n';
  }

  // ===================== kind =====================
  std::cout << "=== kind ===\n";
  {
    pluss::Queue q(pluss::make_series({1.0, 2.0}));
    std::cout << "kind() == Queue::Kind::Series: " << (q.kind() == pluss::Queue::Kind::Series) << '\n';
    std::cout << "kind_name(): " << q.kind_name() << '\n';
    std::cout << "static kind_name(Kind::Note): " << pluss::Queue::kind_name(pluss::Queue::Kind::Note) << '\n';
    std::cout << "static kind_name(Kind::Timeline): " << pluss::Queue::kind_name(pluss::Queue::Kind::Timeline) << '\n';
    std::cout << "static kind_name(Kind::Duration): " << pluss::Queue::kind_name(pluss::Queue::Kind::Duration) << '\n';

    pluss::Queue note_q(pluss::make_note({"x"}));
    std::cout << "series.is_series(): " << q.is_series() << '\n';
    std::cout << "series.is_note(): " << q.is_note() << '\n';
    std::cout << "note.is_note(): " << note_q.is_note() << '\n';
    std::cout << "note.is_series(): " << note_q.is_series() << '\n';
    std::cout << "note.is_timeline(): " << note_q.is_timeline() << '\n';
    std::cout << "note.is_duration(): " << note_q.is_duration() << '\n';
  }

  // ===================== accessors =====================
  std::cout << "=== accessors ===\n";
  {
    pluss::Queue q(pluss::make_series({4.0, 5.0}));
    std::cout << "q.series().numel(): " << q.series().numel() << '\n';
    try {
      (void)q.note();
      std::cout << "q.note(): did not throw\n";
    } catch (const std::exception& e) {
      std::string msg = e.what();
      std::cout << "q.note() threw: " << msg.substr(0, msg.find('\n')) << '\n';
    }

    pluss::Queue mutable_q(pluss::make_series({10.0}));
    mutable_q.series().add_(pluss::make_series({1.0}));
    std::cout << "mutated through non-const series(): " << mutable_q.series().item<double>() << '\n';
  }

  // ===================== accessors: chaining into the real type =====================
  std::cout << "=== accessors chaining ===\n";
  {
    pluss::Queue series_q(pluss::make_series({1.0, 2.0, 3.0, 4.0}));
    std::cout << "series().mean(): " << series_q.series().mean().item<double>() << '\n';
    std::cout << "series().sum(): " << series_q.series().sum().item<double>() << '\n';

    pluss::Queue note_q(pluss::make_note({"  Ab  "}));
    std::cout << "note().strip(): " << note_q.note().strip() << '\n';
    std::cout << "note().upper(): " << note_q.note().upper() << '\n';

    pluss::Queue timeline_q(pluss::timeline::make("1999-04-17T21:15:23Z"));
    std::cout << "timeline().year(): " << pluss::series::print(timeline_q.timeline().year()) << '\n';
    std::cout << "timeline().add_days(1): " << pluss::timeline::print(timeline_q.timeline().add_days(1)) << '\n';

    pluss::Queue duration_q(pluss::duration::hours(-5));
    std::cout << "duration().abs(): " << pluss::duration::print(duration_q.duration().abs()) << '\n';
    std::cout << "duration().total().hours(): " << pluss::series::print(duration_q.duration().total().hours()) << '\n';
  }

  // ===================== state =====================
  std::cout << "=== state ===\n";
  {
    pluss::Queue q(pluss::make_series({1.0, 2.0, 3.0, 4.0}));
    std::cout << "defined(): " << q.defined() << '\n';
    std::cout << "dim(): " << q.dim() << '\n';
    std::cout << "numel(): " << q.numel() << '\n';

    pluss::Queue scalar_duration(pluss::duration::days(1));
    std::cout << "duration dim(): " << scalar_duration.dim() << '\n';
    std::cout << "duration numel(): " << scalar_duration.numel() << '\n';
  }

  // ===================== generic access (is<T>/as<T>) =====================
  std::cout << "=== generic access ===\n";
  {
    pluss::Queue q(pluss::make_note({"p", "q"}));
    std::cout << "is<pluss::Note>(): " << q.is<pluss::Note>() << '\n';
    std::cout << "is<pluss::Series>(): " << q.is<pluss::Series>() << '\n';
    std::cout << "as<pluss::Note>().numel(): " << q.as<pluss::Note>().numel() << '\n';
    try {
      (void)q.as<pluss::Series>();
      std::cout << "as<pluss::Series>(): did not throw\n";
    } catch (const std::bad_variant_access& e) {
      std::cout << "as<pluss::Series>() threw std::bad_variant_access: " << e.what() << '\n';
    }
  }

  // ===================== visit =====================
  std::cout << "=== visit ===\n";
  {
    pluss::Queue q(pluss::make_series({1.0, 2.0, 3.0}));
    const auto label = q.visit([](const auto& payload) -> std::string {
      using T = std::decay_t<decltype(payload)>;
      if constexpr (std::is_same_v<T, pluss::Series>) {
        return "a series with " + std::to_string(payload.numel()) + " elements";
      } else {
        return "something else";
      }
    });
    std::cout << "visit result: " << label << '\n';
  }

  // ===================== invoke =====================
  std::cout << "=== invoke ===\n";
  {
    pluss::Queue series_q(pluss::make_series({1.0, 2.0, 3.0}));
    pluss::Queue doubled = series_q.invoke("mul", {2.0});
    std::cout << "series invoke(mul, 2.0): " << pluss::series::print(doubled.series()) << '\n';

    pluss::Queue note_q(pluss::make_note({"ab", "cd"}));
    pluss::Queue note_upper = note_q.invoke("upper");
    std::cout << "note invoke(upper): " << note_upper.note() << '\n';

    pluss::Queue timeline_q(pluss::timeline::make("1999-04-17T21:15:23Z"));
    pluss::Queue year_q = timeline_q.invoke("year");
    std::cout << "timeline invoke(year) [normalized to dt_year]: "
              << pluss::series::print(year_q.series()) << '\n';

    pluss::Queue chained = series_q.invoke({"neg", "abs"});
    std::cout << "chained invoke({neg, abs}): " << pluss::series::print(chained.series()) << '\n';

    // invoke() cannot reach an in-place (trailing-underscore) method by name.
    try {
      pluss::Queue bad = series_q.invoke("add_", {1.0});
      std::cout << "invoke(add_): did not throw\n";
    } catch (const std::exception& e) {
      std::string msg = e.what();
      std::cout << "invoke(add_) threw: " << msg.substr(0, msg.find('\n')) << '\n';
    }
  }

  // ===================== invoke vs accessor =====================
  std::cout << "=== invoke vs accessor ===\n";
  {
    pluss::Queue q(pluss::make_series({1.0, 2.0, 3.0, 4.0}));

    // Same method, same result, two routes: a name string through invoke(),
    // or a real C++ call through the accessor.
    const double via_invoke = q.invoke("mean").series().item<double>();
    const double via_accessor = q.series().mean().item<double>();
    std::cout << "via invoke(\"mean\"): " << via_invoke << '\n';
    std::cout << "via series().mean(): " << via_accessor << '\n';
    std::cout << "same result: " << (via_invoke == via_accessor) << '\n';
  }

  // ===================== operators =====================
  std::cout << "=== operators ===\n";
  {
    pluss::Queue a(pluss::make_series({1.0, 2.0, 3.0}));
    pluss::Queue b(pluss::make_series({10.0, 20.0, 30.0}));
    std::cout << "+a (unary, no-op): " << pluss::series::print((+a).series()) << '\n';
    std::cout << "-a (unary neg): " << pluss::series::print((-a).series()) << '\n';
    std::cout << "a + b: " << pluss::series::print((a + b).series()) << '\n';
    std::cout << "a * b: " << pluss::series::print((a * b).series()) << '\n';
    std::cout << "a < b: " << pluss::series::print((a < b).series()) << '\n';
    std::cout << "a == a: " << pluss::series::print((a == a).series()) << '\n';

    pluss::Queue note_a(pluss::make_note({"x"}));
    pluss::Queue note_b(pluss::make_note({"y"}));
    pluss::Queue note_sum = note_a + note_b;
    std::cout << "note + note: " << note_sum.note() << '\n';
    try {
      pluss::Queue bad = note_a * note_b;
      std::cout << "note * note: did not throw\n";
    } catch (const std::exception& e) {
      std::string msg = e.what();
      std::cout << "note * note threw: " << msg.substr(0, msg.find('\n')) << '\n';
    }
  }

  // ===================== where =====================
  std::cout << "=== where ===\n";
  {
    pluss::Queue mask(pluss::make_series({true, false, true}));
    pluss::Queue x(pluss::make_series({1.0, 2.0, 3.0}));
    pluss::Queue y(pluss::make_series({10.0, 20.0, 30.0}));
    pluss::Queue picked = pluss::Queue::where(mask, x, y);
    std::cout << "where(mask, x, y): " << pluss::series::print(picked.series()) << '\n';

    try {
      pluss::Queue note_a(pluss::make_note({"a"}));
      pluss::Queue note_b(pluss::make_note({"b"}));
      pluss::Queue bad = pluss::Queue::where(mask, note_a, note_b);
      std::cout << "where on Note: did not throw\n";
    } catch (const std::exception& e) {
      std::string msg = e.what();
      std::cout << "where on Note threw: " << msg.substr(0, msg.find('\n')) << '\n';
    }
  }

  // ===================== stream =====================
  std::cout << "=== stream ===\n";
  {
    pluss::Queue series_q(pluss::make_series({1.0, 2.0}));
    pluss::Queue note_q(pluss::make_note({"a", "b"}));
    pluss::Queue timeline_q(pluss::timeline::make("1999-04-17T21:15:23Z"));
    pluss::Queue duration_q(pluss::duration::hours(2));
    std::cout << "series: " << series_q << '\n';
    std::cout << "note: " << note_q << '\n';
    std::cout << "timeline: " << timeline_q << '\n';
    std::cout << "duration: " << duration_q << '\n';
  }

  return 0;
}
