#include "kindle/catalog.h"

#include <iostream>
#include <ostream>

#include "scribble/registry.h"
#include "loader/Loader.h"
#include "kindle/display/Display.h"
#include "kindle/display/GridModel.h"
#include "table/DataFrame.h"
#include "table/display/Display.h"
#include <pluss/tils/kit/output.h>

namespace pluss::kindle {

namespace {

std::string json_escape(std::string_view in) {
  std::string out;
  out.reserve(in.size() + 8);
  for (char c : in) {
    switch (c) {
      case '\\': out += "\\\\"; break;
      case '"': out += "\\\""; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default: out.push_back(c); break;
    }
  }
  return out;
}

pluss::table::DataFrame catalog_to_io_frame(const Catalog& catalog) {
  TabelizePolicy policy;
  policy.align = Align::Outer;
  policy.align_by = AlignBy::Pos;
  policy.fill = FillPolicy::Null;
  policy.dtype = DTypePolicy::BestEffort;
  policy.collision = CollisionPolicy::Suffix;
  policy.include_label_feature = false;
  return catalog.tabelize(policy);
}

} // namespace

pluss::scribble::Registry Catalog::scribbelize() const {
  PLUSS_CHECK(defined(), "Catalog::scribbelize: catalog undefined");
  std::vector<std::pair<std::string, pluss::scribble::Slot>> out;
  out.reserve(static_cast<std::size_t>(nkeys()));
  for (const auto& key : keys()) {
    out.emplace_back(key, keys(key).scribbelize());
  }
  return pluss::scribble::Registry(std::move(out));
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
  if (!defined()) return "{ \"_type\": \"Catalog\", \"defined\": false }";

  std::ostringstream oss;
  oss << "{\n";
  oss << "  \"_type\": \"Catalog\",\n";
  oss << "  \"defined\": true,\n";
  oss << "  \"nkeys\": " << nkeys() << ",\n";
  oss << "  \"nrecords\": " << nentries() << ",\n";
  oss << "  \"keys\": [";
  const auto& names = keys();
  for (std::size_t i = 0; i < names.size(); ++i) {
    if (i == 0) oss << "\n";
    oss << "    \"" << json_escape(names[i]) << "\"";
    if (i + 1 < names.size()) oss << ",";
    oss << "\n";
  }
  if (!names.empty()) oss << "  ";
  oss << "],\n";
  oss << "  \"streams\": [";
  for (std::size_t i = 0; i < names.size(); ++i) {
    if (i == 0) oss << "\n";
    oss << "    {\n";
    oss << "      \"key\": \"" << json_escape(names[i]) << "\",\n";
    oss << "      \"value\": " << at(names[i]).to_json() << "\n";
    oss << "    }";
    if (i + 1 < names.size()) oss << ",";
    oss << "\n";
  }
  if (!names.empty()) oss << "  ";
  oss << "]\n";
  oss << "}";
  return oss.str();
}

void Catalog::show(std::ostream& os) const { display::print(os, *this); }

pluss::table::display::ModelPage catalog_page(const Catalog& catalog) {
  PLUSS_CHECK(catalog.defined(), "Catalog::show: catalog undefined");
  // The keys side by side as a ragged grid: each column is as long as its own stream and nothing is padded.
  auto model = std::make_shared<display::CatalogGridModel>(catalog);
  pluss::table::display::ModelPage page;
  page.title = "Catalog";
  const bool ragged = model->longest() != model->shortest();
  page.subtitle = ragged ? std::to_string(catalog.nkeys()) + " keys, " + std::to_string(model->shortest()) + " to " +
                               std::to_string(model->longest()) + " entries"
                         : std::to_string(catalog.nkeys()) + " keys, " + std::to_string(model->longest()) + " entries";
  page.cards = {{"Keys", std::to_string(catalog.nkeys())},
                {"Longest", std::to_string(model->longest())},
                {"Shortest", std::to_string(model->shortest())}};
  page.model = model;
  for (const auto& name : catalog.keys()) {
    page.streams.emplace_back(name, std::make_shared<const pluss::table::Shelf>(catalog.keys(name).to_shelf()));
  }
  page.offer_align = ragged;
  page.on_options = [model](const pluss::table::display::DisplayOptions& options) {
    model->set_alignment(options.align);
    model->set_filter(options.filter);
    model->set_window(pluss::table::display::row_window_for(options));
    model->set_number_format(options.number_format);
  };
  return page;
}

void Catalog::show() const {
  // In a notebook a catalog is a ragged HTML grid, not a window.
  if (pluss::table::display::show_in_notebook(pluss::table::display::render_notebook_html(*this))) {
    return;
  }
  const auto page = catalog_page(*this);
  // A catalog is not a DataFrame, and opens in Ember so it looks like it unless a default theme was set.
  pluss::table::display::DisplayOptions options;
  if (pluss::table::display::default_theme() == pluss::table::display::DisplayTheme::Default) {
    options.theme = pluss::table::display::DisplayTheme::Ember;
  }
  if (!pluss::table::display::show_model(page, options)) {
    show(std::cout);
  }
}

std::ostream& operator<<(std::ostream& os, const Catalog& catalog) {
  catalog.show(os);
  return os;
}

} // namespace pluss::kindle

namespace pluss::table::display {

// A catalog in a notebook: the keys side by side as a grid, each column exactly as long as its own stream, blank where a
// stream has no entry (which is not a NULL), under a footer that says how ragged it is.
std::string render_notebook_html(const pluss::kindle::Catalog& catalog) {
  if (!catalog.defined()) {
    return "<div class=\"pluss-table\">Catalog &lt;undefined&gt;</div>";
  }
  const pluss::kindle::display::CatalogGridModel model(catalog);
  const auto entries = model.shortest() == model.longest()
                           ? pluss::tils::OutputEngine::format("{} entries", model.longest())
                           : pluss::tils::OutputEngine::format("{} to {} entries", model.shortest(), model.longest());
  return render_notebook_html(model, pluss::tils::OutputEngine::format("{} keys &middot; {}", model.keys(), entries));
}

}  // namespace pluss::table::display
