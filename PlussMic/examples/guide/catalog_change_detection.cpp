// What changed between two loads of the same table? Compare the catalogs with a fingerprint, find the stream that changed, say how it differs,
// and decide which downstream reports need rebuilding. Also: how a fingerprint differs from a stamp.
#include <pluss/pluss.h>

#include <iostream>

using namespace pluss::kindle::literals;
using pluss::kindle::Catalog;
using pluss::kindle::Stream;

static Catalog load(double stock_b) {
  return Catalog({"sku"_k = Stream(pluss::make_note({"A100", "B200", "C300"})), "stock"_k = Stream(pluss::make_series({40.0, stock_b, 7.0})), "price"_k = Stream(pluss::make_series({9.5, 20.0, 31.0}))});
}

int main() {
  auto yesterday = load(12.0);
  auto today = load(9.0);

  // 1. Compare every stream of the two loads by fingerprint: a hash of the values, equal whenever the contents are equal.
  std::cout << "whole catalogs differ: " << (yesterday.fingerprint() != today.fingerprint() ? "yes" : "no") << "\n";
  bool stock_changed = false, price_changed = false;
  for (const auto &name : yesterday) {
    const bool same = yesterday.keys(name).fingerprint() == today.keys(name).fingerprint();
    std::cout << "  " << name << ": " << (same ? "unchanged" : "CHANGED") << "\n";
    if (name == "stock") stock_changed = !same;
    if (name == "price") price_changed = !same;
  }

  // 2. Say how the changed stream differs.
  auto delta = today.keys("stock") - yesterday.keys("stock");
  std::cout << "stock change: " << pluss::series::print(delta.as_series()) << "\n";

  // 3. Rebuild only what depends on a changed stream.
  std::cout << "reorder report must be rebuilt: " << (stock_changed ? "yes" : "no") << "; price list must be rebuilt: " << (price_changed ? "yes" : "no") << "\n";

  // 4. A fingerprint follows the contents. A stamp identifies the stored data a catalog shares (it is a cache token): a copy shares the
  //    storage and so the stamp, while a table built separately with the same content has its own stamp but the same fingerprint.
  auto copy = yesterday;
  auto rebuilt = load(12.0);
  std::cout << "copy has the same stamp:    " << (copy.stamp() == yesterday.stamp() ? "yes" : "no") << "\n";
  std::cout << "rebuilt has the same stamp: " << (rebuilt.stamp() == yesterday.stamp() ? "yes" : "no") << " (equal content: " << (rebuilt.keys("stock").fingerprint() == yesterday.keys("stock").fingerprint() ? "yes" : "no") << ")\n";
}
