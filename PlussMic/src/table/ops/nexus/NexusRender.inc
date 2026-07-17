std::string Nexus::to_string(const display::DisplayOptions&) const {
  const auto& src = impl_ref_();
  std::ostringstream os;
  os << "Nexus\n";
  os << "-----\n";
  os << "tables: " << src.keys_.size() << '\n';
  for (std::size_t i = 0; i < src.keys_.size(); ++i) {
    os << src.keys_[i] << " -> [" << src.frames_[i].ncases() << " cs x " << src.frames_[i].nfeatures()
       << " feat]\n";
  }
  return os.str();
}

void Nexus::show(const display::DisplayOptions& options) const {
  show(std::cout, options);
}

void Nexus::show(std::ostream& os, const display::DisplayOptions& options) const {
  os << to_string(options);
}

std::ostream& operator<<(std::ostream& os, const Nexus& nexus) {
  os << nexus.to_string();
  return os;
}

void output_render(std::ostream& os, const Nexus& nexus) {
  os << nexus.to_string();
}
