#include "kindle/catalog.h"

#include <iostream>
#include <ostream>

#include "loader/Loader.h"
#include "kindle/display/Display.h"
#include "table/DataFrame.h"
#include "table/display/Display.h"

namespace pluss::kindle {

namespace {

pluss::table::DataFrame catalog_to_io_frame(const Catalog& catalog) {
  TabelizePolicy policy;
  policy.align = AlignPolicy::Outer;
  policy.index_mode = IndexMode::Positional;
  policy.fill = FillPolicy::Null;
  policy.dtype = DTypePolicy::BestEffort;
  policy.collision = CollisionPolicy::Suffix;
  policy.include_case_feature = false;
  return catalog.tabelize(policy);
}

} // namespace

pluss::table::DataFrame Catalog::to_table(const TabelizePolicy& policy, bool render) const {
  auto out = tabelize(policy);
  if (render) {
    pluss::table::display::print(std::cout, out, {});
  }
  return out;
}

void Catalog::display() const {
  show(std::cout);
}

void Catalog::info() const {
  display::print(std::cout, *this);
}

void Catalog::save_csv(std::string_view filename) const {
  save_csv(filename, {});
}

void Catalog::save_csv(std::string_view filename, const pluss::loader::Config& config) const {
  catalog_to_io_frame(*this).save_csv(filename, config);
}

void Catalog::save_txt(std::string_view filename) const {
  save_txt(filename, {});
}

void Catalog::save_txt(std::string_view filename, const pluss::loader::Config& config) const {
  catalog_to_io_frame(*this).save_txt(filename, config);
}

void Catalog::save_excel(std::string_view filename) const {
  save_excel(filename, {});
}

void Catalog::save_excel(std::string_view filename, const pluss::loader::Config& config) const {
  catalog_to_io_frame(*this).save_excel(filename, config);
}

void Catalog::save_json(std::string_view filename) const {
  save_json(filename, {});
}

void Catalog::save_json(std::string_view filename, const pluss::loader::Config& config) const {
  catalog_to_io_frame(*this).save_json(filename, config);
}

void Catalog::save_parquet(std::string_view filename) const {
  save_parquet(filename, {});
}

void Catalog::save_parquet(std::string_view filename, const pluss::loader::Config& config) const {
  catalog_to_io_frame(*this).save_parquet(filename, config);
}

void Catalog::save_as(std::string_view target_type,
                      std::string_view filename,
                      const pluss::loader::Config& config) const {
  catalog_to_io_frame(*this).save_as(target_type, filename, config);
}

std::string Catalog::to_string() const {
  return display::render(*this);
}

std::string Catalog::to_json() const {
  return display::render(*this);
}

void Catalog::show(std::ostream& os) const { display::print(os, *this); }

std::ostream& operator<<(std::ostream& os, const Catalog& catalog) {
  catalog.show(os);
  return os;
}

} // namespace pluss::kindle
