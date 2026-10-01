#include <pluss/pluss.h>

#include <iostream>

namespace {
void heading(const char* label) {
  std::cout << "\n=== " << label << " ===\n";
}
}  // namespace

int main() {
  // ---- clamp / clamp_ ----
  heading("clamp: basic two-sided bound");
  {
    auto line = pluss::make_series({-3.0, -1.0, 0.0, 2.0, 5.0}, {5});
    std::cout << line.clamp(-1.0, 2.0);
  }

  heading("clamp: gradient, inclusive boundary");
  {
    auto x = pluss::make_series({-3.0, -1.0, 0.0, 2.0, 5.0}, {5});
    x.tracks_slope_(true);
    auto y = x.clamp(-1.0, 2.0);
    y.sum().reverse();
    std::cout << "y=" << y << "\n";
    std::cout << "dx=" << x.sensitivity() << "\n";
  }

  heading("clamp_: in-place mutation");
  {
    auto line = pluss::make_series({-3.0, -1.0, 0.0, 2.0, 5.0}, {5});
    line.clamp_(-1.0, 2.0);
    std::cout << line;
  }

  heading("clamp: min > max behavior");
  {
    auto line = pluss::make_series({-3.0, -1.0, 0.0, 2.0, 5.0}, {5});
    try {
      auto out = line.clamp(2.0, -1.0);
      std::cout << "no throw: " << out;
    } catch (const std::exception& e) {
      std::cout << "THREW: " << e.what();
    }
  }

  heading("clamp vs clamp_min+clamp_max composition");
  {
    auto line = pluss::make_series({-3.0, -1.0, 0.0, 2.0, 5.0}, {5});
    auto composed = line.clamp_min(-1.0).clamp_max(2.0);
    auto direct = line.clamp(-1.0, 2.0);
    std::cout << "composed=" << composed << "\n";
    std::cout << "direct=" << direct;
  }

  // ---- add_ ----
  heading("add_: basic in-place elementwise sum");
  {
    auto x = pluss::make_series({1.0, 2.0, 3.0}, {3});
    auto y = pluss::make_series({10.0, 20.0, 30.0}, {3});
    x.add_(y);
    std::cout << x;
  }

  heading("add_: returns a reference to the same object");
  {
    auto x = pluss::make_series({1.0, 2.0}, {2});
    auto y = pluss::make_series({5.0, 5.0}, {2});
    auto& ref = x.add_(y);
    std::cout << std::boolalpha << (&ref == &x);
  }

  heading("add_: scalar-shaped rhs broadcasts");
  {
    auto x = pluss::make_series({1.0, 2.0, 3.0}, {3});
    auto scalar = pluss::make_series({100.0}, {1});
    x.add_(scalar);
    std::cout << x;
  }

  // ==================== Batch 1: core arithmetic + comparison in-place ====================
  heading("sub_: Series rhs");
  {
    auto x = pluss::make_series({10.0, 20.0, 30.0}, {3});
    auto y = pluss::make_series({1.0, 2.0, 3.0}, {3});
    x.sub_(y);
    std::cout << x;
  }
  heading("mul_: Series rhs");
  {
    auto x = pluss::make_series({2.0, 3.0, 4.0}, {3});
    auto y = pluss::make_series({5.0, 5.0, 5.0}, {3});
    x.mul_(y);
    std::cout << x;
  }
  heading("mul_: scalar (double) rhs");
  {
    auto x = pluss::make_series({2.0, 3.0, 4.0}, {3});
    x.mul_(10.0);
    std::cout << x;
  }
  heading("div_: Series rhs");
  {
    auto x = pluss::make_series({10.0, 20.0, 30.0}, {3});
    auto y = pluss::make_series({2.0, 4.0, 5.0}, {3});
    x.div_(y);
    std::cout << x;
  }
  heading("div_: scalar (double) rhs");
  {
    auto x = pluss::make_series({10.0, 20.0, 30.0}, {3});
    x.div_(2.0);
    std::cout << x;
  }
  heading("neg_: no args");
  {
    auto x = pluss::make_series({1.0, -2.0, 3.0}, {3});
    x.neg_();
    std::cout << x;
  }
  heading("pow_: Series rhs");
  {
    auto x = pluss::make_series({2.0, 3.0, 4.0}, {3});
    auto y = pluss::make_series({2.0, 2.0, 2.0}, {3});
    x.pow_(y);
    std::cout << x;
  }
  heading("pow_: scalar (double) rhs");
  {
    auto x = pluss::make_series({2.0, 3.0, 4.0}, {3});
    x.pow_(2.0);
    std::cout << x;
  }
  heading("abs_: no args");
  {
    auto x = pluss::make_series({-1.0, 2.0, -3.0}, {3});
    x.abs_();
    std::cout << x;
  }
  heading("fmod_: Series rhs");
  {
    auto x = pluss::make_series({10.0, 11.0, -7.0}, {3});
    auto y = pluss::make_series({3.0, 3.0, 3.0}, {3});
    x.fmod_(y);
    std::cout << x;
  }
  heading("fmod_: scalar (double) rhs");
  {
    auto x = pluss::make_series({10.0, 11.0, -7.0}, {3});
    x.fmod_(3.0);
    std::cout << x;
  }
  heading("remainder_: Series rhs");
  {
    auto x = pluss::make_series({10.0, 11.0, -7.0}, {3});
    auto y = pluss::make_series({3.0, 3.0, 3.0}, {3});
    x.remainder_(y);
    std::cout << x;
  }
  heading("remainder_: scalar (double) rhs, sign differs from fmod_ on negatives");
  {
    auto x = pluss::make_series({10.0, 11.0, -7.0}, {3});
    x.remainder_(3.0);
    std::cout << x;
  }
  heading("floor_divide_: Series rhs");
  {
    auto x = pluss::make_series({7.0, -7.0, 8.0}, {3});
    auto y = pluss::make_series({2.0, 2.0, 2.0}, {3});
    x.floor_divide_(y);
    std::cout << x;
  }
  heading("floor_divide_: scalar (double) rhs");
  {
    auto x = pluss::make_series({7.0, -7.0, 8.0}, {3});
    x.floor_divide_(2.0);
    std::cout << x;
  }
  heading("eq_: Series rhs, dtype becomes bool");
  {
    auto x = pluss::make_series({1.0, 2.0, 3.0}, {3});
    auto y = pluss::make_series({1.0, 5.0, 3.0}, {3});
    x.eq_(y);
    std::cout << x;
  }
  heading("ne_: Series rhs");
  {
    auto x = pluss::make_series({1.0, 2.0, 3.0}, {3});
    auto y = pluss::make_series({1.0, 5.0, 3.0}, {3});
    x.ne_(y);
    std::cout << x;
  }
  heading("gt_: Series rhs");
  {
    auto x = pluss::make_series({1.0, 2.0, 3.0}, {3});
    auto y = pluss::make_series({0.0, 2.0, 5.0}, {3});
    x.gt_(y);
    std::cout << x;
  }
  heading("ge_: Series rhs");
  {
    auto x = pluss::make_series({1.0, 2.0, 3.0}, {3});
    auto y = pluss::make_series({1.0, 3.0, 3.0}, {3});
    x.ge_(y);
    std::cout << x;
  }
  heading("lt_: Series rhs");
  {
    auto x = pluss::make_series({1.0, 2.0, 3.0}, {3});
    auto y = pluss::make_series({2.0, 2.0, 1.0}, {3});
    x.lt_(y);
    std::cout << x;
  }
  heading("le_: Series rhs");
  {
    auto x = pluss::make_series({1.0, 2.0, 3.0}, {3});
    auto y = pluss::make_series({1.0, 1.0, 5.0}, {3});
    x.le_(y);
    std::cout << x;
  }
  heading("gradient probe: sub_/mul_/div_/neg_/pow_/abs_ -- do in-place ops track gradients?");
  {
    auto x = pluss::make_series({2.0, 3.0}, {2});
    x.tracks_slope_(true);
    try {
      x.mul_(2.0);
      std::cout << "mul_ tracks after: " << x.tracks_slope() << "\n";
    } catch (const std::exception& e) {
      std::cout << "mul_ THREW: " << e.what() << "\n";
    }
  }

  heading("add_ on a tracked Series: also guarded?");
  {
    auto x = pluss::make_series({1.0, 2.0}, {2});
    x.tracks_slope_(true);
    auto y = pluss::make_series({1.0, 1.0}, {2});
    try {
      x.add_(y);
      std::cout << "no throw\n";
    } catch (const std::exception& e) {
      std::cout << "THREW: " << e.what() << "\n";
    }
  }
  heading("clamp_ on a tracked Series: also guarded?");
  {
    auto x = pluss::make_series({1.0, 2.0}, {2});
    x.tracks_slope_(true);
    try {
      x.clamp_(-1.0, 5.0);
      std::cout << "no throw\n";
    } catch (const std::exception& e) {
      std::cout << "THREW: " << e.what() << "\n";
    }
  }
  heading("in-place op on a NOT-tracked Series works fine (baseline confirmation)");
  {
    auto x = pluss::make_series({1.0, 2.0}, {2});
    std::cout << "tracks_slope before: " << std::boolalpha << x.tracks_slope() << "\n";
    x.add_(pluss::make_series({10.0, 10.0}, {2}));
    std::cout << x;
  }

  heading("tracked-series guard survey across batch 1");
  {
    auto probe = [](const char* name, auto fn) {
      auto x = pluss::make_series({2.0, 3.0}, {2});
      x.tracks_slope_(true);
      try {
        fn(x);
        std::cout << name << ": no throw\n";
      } catch (const std::exception& e) {
        std::cout << name << ": THREW\n";
      }
    };
    auto y = pluss::make_series({1.0, 1.0}, {2});
    probe("sub_", [&](pluss::Series& s) { s.sub_(y); });
    probe("mul_", [&](pluss::Series& s) { s.mul_(y); });
    probe("div_", [&](pluss::Series& s) { s.div_(y); });
    probe("neg_", [&](pluss::Series& s) { s.neg_(); });
    probe("pow_", [&](pluss::Series& s) { s.pow_(y); });
    probe("abs_", [&](pluss::Series& s) { s.abs_(); });
    probe("fmod_", [&](pluss::Series& s) { s.fmod_(y); });
    probe("remainder_", [&](pluss::Series& s) { s.remainder_(y); });
    probe("floor_divide_", [&](pluss::Series& s) { s.floor_divide_(y); });
    probe("eq_", [&](pluss::Series& s) { s.eq_(y); });
    probe("gt_", [&](pluss::Series& s) { s.gt_(y); });
    probe("clamp_", [&](pluss::Series& s) { s.clamp_(-1.0, 5.0); });
    probe("add_", [&](pluss::Series& s) { s.add_(y); });
  }

  heading("DIAGNOSTIC: does add_ (invoke_ dispatch path) bump the storage version?");
  {
    auto x = pluss::make_series({2.0, 3.0}, {2});
    auto before = x.storage().version();
    x.add_(pluss::make_series({1.0, 1.0}, {2}));
    auto after = x.storage().version();
    std::cout << "before=" << before << " after=" << after
              << " bumped=" << std::boolalpha << (after != before) << "\n";
  }
  heading("DIAGNOSTIC: does mul_ (hand-written guarded path) bump the storage version?");
  {
    auto x = pluss::make_series({2.0, 3.0}, {2});
    auto before = x.storage().version();
    x.mul_(pluss::make_series({1.0, 1.0}, {2}));
    auto after = x.storage().version();
    std::cout << "before=" << before << " after=" << after
              << " bumped=" << std::boolalpha << (after != before) << "\n";
  }
  heading("DIAGNOSTIC: does in-place mutation of a saved non-leaf get caught at backward (staleness check)?");
  {
    // y is non-leaf (has a source_link, parent x). z = y * w saves y's VALUE
    // for its own backward (d(z)/d(w) = y). If we mutate y in place via
    // add_ (invoke_ dispatch) after z is built but before reverse(), a
    // correct version-counter safety net must catch this at reverse() time,
    // the same way libtorch raises "one of the variables needed for
    // gradient computation has been modified by an inplace operation".
    auto x = pluss::make_series({2.0, 3.0}, {2});
    x.tracks_slope_(true);
    auto y = x.add(pluss::make_series({0.0, 0.0}, {2}));  // non-leaf, tracks_slope true
    std::cout << "y.tracks_slope()=" << std::boolalpha << y.tracks_slope()
              << " y is leaf (no source)=" << (!y.source().has_value()) << "\n";
    auto w = pluss::make_series({5.0, 7.0}, {2});
    w.tracks_slope_(true);
    auto z = y.mul(w);  // z's backward needs y's original value to compute dw

    // Mutate y in place AFTER it was captured as a parent of z.
    y.add_(pluss::make_series({100.0, 100.0}, {2}));

    try {
      z.sum().reverse();
      std::cout << "no throw -- dw=" << w.sensitivity() << "\n";
      std::cout << "(if dw reflects the MUTATED y instead of the original, "
                   "gradients are silently wrong)\n";
    } catch (const std::exception& e) {
      std::cout << "THREW (staleness caught): " << e.what() << "\n";
    }
  }

  heading("add.md merged in-place example: x.add_(y), same-object check");
  {
    auto x = pluss::make_series({1.0, 2.0, 3.0}, {3});
    auto y = pluss::make_series({10.0, 20.0, 30.0}, {3});

    auto& ref = x.add_(y);
    std::cout << "x=" << x << "\n";
    std::cout << std::boolalpha << "same object: " << (&ref == &x);
  }

  heading("DIAGNOSTIC: does public x.put_(idx, src) wire src into the autodiff graph?");
  {
    auto x = pluss::make_series({0.0, 0.0, 0.0}, {3});
    x.tracks_slope_(true);
    auto idx = pluss::make_series({1.0}, {1});  // placeholder; real index dtype handled below
    auto src = pluss::make_series({5.0}, {1});
    src.tracks_slope_(true);
    try {
      auto idx_i = pluss::series::arange(1, 2, 1, pluss::ScalarType::Int64);
      x.put_(idx_i, src, /*accumulate=*/false);
      std::cout << "no throw; x=" << x << "\n";
      x.sum().reverse();
      std::cout << "src.has_sensitivity()=" << std::boolalpha << src.has_sensitivity() << "\n";
      if (src.has_sensitivity()) std::cout << "src grad=" << src.sensitivity() << "\n";
    } catch (const std::exception& e) {
      std::cout << "THREW: " << e.what() << "\n";
    }
  }

  heading("RECHECK: put_ on tracked LEAF -- is_leaf just before call, throw?");
  {
    auto x = pluss::make_series({0.0, 0.0, 0.0}, {3});
    x.tracks_slope_(true);
    std::cout << "x.source().has_value()=" << std::boolalpha << x.source().has_value() << "\n";
    auto idx_i = pluss::series::arange(1, 2, 1, pluss::ScalarType::Int64);
    auto src = pluss::make_series({5.0}, {1});
    src.tracks_slope_(true);
    try {
      x.put_(idx_i, src, false);
      std::cout << "put_: no throw\n";
    } catch (const std::exception& e) {
      std::cout << "put_: THREW: " << e.what() << "\n";
    }
  }

  heading("RECHECK: index_put_ on tracked LEAF -- throw?");
  {
    auto x = pluss::make_series({0.0, 0.0, 0.0}, {3});
    x.tracks_slope_(true);
    std::cout << "x.source().has_value()=" << std::boolalpha << x.source().has_value() << "\n";
    auto idx_i = pluss::series::arange(1, 2, 1, pluss::ScalarType::Int64);
    auto src = pluss::make_series({5.0}, {1});
    src.tracks_slope_(true);
    try {
      x.index_put_({idx_i}, src, false);
      std::cout << "index_put_: no throw; x=" << x << "\n";
      x.sum().reverse();
      std::cout << "src grad=" << (src.has_sensitivity() ? src.sensitivity() : pluss::make_series(-999.0)) << "\n";
    } catch (const std::exception& e) {
      std::cout << "index_put_: THREW: " << e.what() << "\n";
    }
  }

  heading("MINIMAL: is_leaf/source() timeline for a fresh series");
  {
    auto x = pluss::make_series({0.0, 0.0, 0.0}, {3});
    std::cout << "immediately after make_series: source().has_value()=" << std::boolalpha << x.source().has_value() << "\n";
    x.tracks_slope_(true);
    std::cout << "immediately after tracks_slope_(true): source().has_value()=" << std::boolalpha << x.source().has_value() << "\n";
  }

  heading("what op_name does a fresh make_series bind?");
  {
    auto x = pluss::make_series({0.0, 0.0, 0.0}, {3});
    if (auto src = x.source()) {
      std::cout << "op_name=" << src->op_name << " parent_count=" << src->parent_count << "\n";
    } else {
      std::cout << "no source (true leaf)\n";
    }
    std::cout << "is_leaf()=" << std::boolalpha << x.is_leaf() << "\n";
  }

  heading("does detach_() produce a true leaf, and does index_put_ THEN block it?");
  {
    auto x = pluss::make_series({0.0, 0.0, 0.0}, {3});
    x.tracks_slope_(true);
    x.detach_();
    x.tracks_slope_(true);
    std::cout << "after detach_+retrack: is_leaf()=" << std::boolalpha << x.is_leaf() << "\n";
    auto idx_i = pluss::series::arange(1, 2, 1, pluss::ScalarType::Int64);
    auto src = pluss::make_series({5.0}, {1});
    src.tracks_slope_(true);
    try {
      x.index_put_({idx_i}, src, false);
      std::cout << "index_put_ on true leaf: no throw\n";
    } catch (const std::exception& e) {
      std::cout << "index_put_ on true leaf: THREW: " << e.what() << "\n";
    }
  }

  return 0;
}
