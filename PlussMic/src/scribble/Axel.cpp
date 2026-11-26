#include "scribble/Axel.h"
#include "scribble/RegistryGrid.h"
#include "scribble/SlotDisplay.h"
#include "table/display/StackPage.h"
#include "table/display/Display.h"
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <map>
#include <ostream>
#include <sstream>
#include <unordered_set>
#include <utility>

#include "CodeM/core/Errors.h"
#include "kindle/Archive.h"
#include "table/Atlas.h"

namespace pluss::scribble {

namespace {

std::vector<std::string> normalize_names(std::span<const std::string_view> keys) {
  std::vector<std::string> out;
  out.reserve(keys.size());
  std::unordered_set<std::string> seen;
  for (auto sv : keys) {
    std::string key(sv);
    PLUSS_CHECK(!key.empty(), "Axel: field cannot be empty");
    PLUSS_CHECK(seen.insert(key).second, "Axel: duplicate field '" + key + "'");
    out.push_back(std::move(key));
  }
  return out;
}

std::pair<std::int64_t, std::int64_t> normalize_slice(std::int64_t start, std::int64_t stop, std::int64_t size) {
  std::int64_t s = start < 0 ? start + size : start;
  std::int64_t e = stop < 0 ? stop + size : stop;
  s = std::clamp<std::int64_t>(s, 0, size);
  e = std::clamp<std::int64_t>(e, 0, size);
  if (e < s) e = s;
  return {s, e};
}

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

} // namespace

namespace detail {
class AxelImpl final : public pluss::msharp::RefCounted {
public:
  AxelImpl() = default;
  AxelImpl(std::vector<Registry> registries, std::optional<Slot> labels)
      : registries_(std::move(registries)), labels_(std::move(labels)) {
    if (labels_.has_value()) {
      PLUSS_CHECK(labels_->defined(), "Axel: labels slot undefined");
      if (labels_->size() != static_cast<std::int64_t>(registries_.size())) {
        throw pluss::Error("Axel: labels size must match depth");
      }
    }
  }

  std::vector<Registry> registries_{};
  std::optional<Slot> labels_{std::nullopt};
};
} // namespace detail


#include "scribble/ops/axel/AxelLifecycleAndOps.inc"
#include "scribble/ops/axel/AxelAccessors.inc"
#include "scribble/ops/axel/AxelRenderAndInternals.inc"

} // namespace pluss::scribble

namespace pluss::table::display {

// An axel in a notebook: the same short list of facts the terminal shows.
std::string render_notebook_html(const pluss::scribble::Axel& axel) {
  if (!axel.defined()) {
    return "<div class=\"pluss-table\">Axel &lt;undefined&gt;</div>";
  }
  return render_notebook_stack_facts(pluss::scribble::display::axel_facts(axel));
}

}  // namespace pluss::table::display

namespace pluss::scribble {

// The axel in the PlussView window: one registry at a time as the ragged grid a registry opens as (see StackPage.h).
void Axel::show() const {
  if (pluss::table::display::show_in_notebook(pluss::table::display::render_notebook_html(*this))) {
    return;
  }
  PLUSS_CHECK(defined(), "Axel::show: axel undefined");
  const Axel held = *this;
  pluss::table::display::StackSource source;
  source.title = "Axel";
  source.unit = "registry";
  source.depth = depth();
  source.ragged = true;
  source.label = [held](const std::int64_t i) {
    return held.labels().has_value() ? detail::slot_cell_text(held.labels()->at(i)) : std::string{};
  };
  source.table = [held](const std::int64_t i) -> std::shared_ptr<pluss::table::display::OptionsModel> {
    return std::make_shared<display::RegistryGridModel>(held.registry(i));
  };
  source.cards = [held](const std::int64_t i) {
    const display::RegistryGridModel model(held.registry(i));
    return std::vector<std::pair<std::string, std::string>>{{"Fields", std::to_string(model.fields())},
                                                              {"Longest", std::to_string(model.longest())},
                                                              {"Shortest", std::to_string(model.shortest())}};
  };
  pluss::table::display::DisplayOptions options;
  if (pluss::table::display::default_theme() == pluss::table::display::DisplayTheme::Default) {
    options.theme = pluss::table::display::DisplayTheme::Ember;  // a ragged type opens in Ember
  }
  if (!pluss::table::display::show_model(pluss::table::display::stack_model_page(std::move(source)), options)) {
    show(std::cout);
  }
}

}  // namespace pluss::scribble
