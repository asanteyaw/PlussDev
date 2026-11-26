std::string Slot::to_string() const {
  return to_json();
}

std::string Slot::to_json() const {
  if (!defined()) return "{ \"_type\": \"Slot\", \"defined\": false }";
  std::ostringstream oss;
  oss << "{\n";
  oss << "  \"_type\": \"Slot\",\n";
  oss << "  \"defined\": true,\n";
  oss << "  \"size\": " << size() << ",\n";
  oss << "  \"values\": [";
  if (!values().empty()) oss << "\n";
  for (std::size_t i = 0; i < values().size(); ++i) {
    oss << "    " << value_to_json(values()[i]);
    if (i + 1 < values().size()) oss << ",";
    oss << "\n";
  }
  if (!values().empty()) oss << "  ";
  oss << "]\n";
  oss << "}";
  return oss.str();
}

namespace detail {

std::string slot_held_text(const Slot& slot) {
  const auto& cells = slot.values();
  std::vector<SlotKind> kinds;  // the kinds present, in the order they first appear
  for (const auto& cell : cells) {
    const auto kind = slot_kind_of_value(cell);
    if (kind != SlotKind::Null && std::find(kinds.begin(), kinds.end(), kind) == kinds.end()) {
      kinds.push_back(kind);
    }
  }
  const auto kind_name = [](const SlotKind kind) -> std::string {
    switch (kind) {
      case SlotKind::Series: return "Series";
      case SlotKind::Note: return "Note";
      case SlotKind::Timeline: return "Timeline";
      case SlotKind::Duration: return "Duration";
      default: return "Mixed";
    }
  };
  if (cells.empty()) {
    return "Empty";
  }
  if (kinds.empty()) {
    return "Null";
  }
  if (kinds.size() == 1) {
    return kind_name(kinds.front());
  }
  std::string held = "Mixed (";
  for (std::size_t i = 0; i < kinds.size(); ++i) {
    held += (i > 0 ? ", " : "") + kind_name(kinds[i]);
  }
  return held + ")";
}

std::string slot_cell_text(const Slot::Value& cell) {
  return std::visit(
      [](const auto& v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
          return "NULL";
        } else if (v.numel() == 1) {
          return pluss::table::display::first_cell_text(pluss::table::Shelf(v));
        } else {
          return pluss::tils::OutputEngine::format("[{} values]", v.numel());
        }
      },
      cell);
}

}  // namespace detail

// A slot prints as a Shelf does (see table::display::SequenceView): its position and its value, the first and last
// rows with dots between, and a footer saying what it holds. Each cell holds one value, so a cell prints as that value;
// only the rows that are shown are formatted, however long the slot is.
namespace {

void print_slot_(std::ostream& os, const Slot& slot) {
  if (!slot.defined()) {
    os << "Slot <undefined>";
    return;
  }
  const auto& cells = slot.values();
  pluss::table::display::SequenceView view;
  view.size = static_cast<std::int64_t>(cells.size());
  view.value = [&](const std::int64_t row) { return detail::slot_cell_text(cells[static_cast<std::size_t>(row)]); };
  view.footer = "ptype: " + detail::slot_held_text(slot);
  pluss::table::display::print_sequence(os, view);
}

}  // namespace

const detail::SlotImpl& Slot::impl_ref_() const {
  PLUSS_CHECK(defined(), "Slot: undefined handle");
  return *impl_;
}

detail::SlotImpl& Slot::mut_impl_() {
  PLUSS_CHECK(defined(), "Slot: undefined handle");
  if (impl_.use_count() > 1) {
    impl_ = pluss::msharp::make_intrusive<detail::SlotImpl>(impl_ref_().values_);
  }
  return *impl_;
}

std::ostream& operator<<(std::ostream& os, const Slot& slot) {
  print_slot_(os, slot);
  return os;
}
