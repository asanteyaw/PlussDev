#include <array>
#include <cstdint>
#include <iostream>
#include <span>
#include <string>
#include <vector>

#include <pluss/pluss.h>

int main() {
  std::cout << "=== scalar / operator<< ===\n";
  pluss::Note scalar = pluss::make_note("Hello, Note printing!\nwith newlines and \"quotes\".");
  std::cout << scalar << "\n\n";

  std::cout << "=== 1D clipped values ===\n";
  const std::array<std::int64_t, 1> v_sizes{6};
  pluss::Note v = pluss::make_note(
      std::vector<std::string>{
          "alpha",
          "beta",
          "a very very very very very long token that should be clipped",
          "line1\nline2",
          "tab\tseparated",
          "omega",
      },
      std::span<const std::int64_t>(v_sizes.data(), v_sizes.size()));
  pluss::NotePrintOptions clip;
  clip.max_cell_chars = 24;
  std::cout << pluss::print_note(v, clip) << "\n\n";

  std::cout << "=== 2D summarized with edge_items ===\n";
  const std::array<std::int64_t, 2> m_sizes{3, 8};
  std::vector<std::string> m_flat;
  m_flat.reserve(24);
  for (int r = 0; r < 3; ++r) {
    for (int c = 0; c < 8; ++c) {
      m_flat.push_back("r" + std::to_string(r) + "_c" + std::to_string(c));
    }
  }
  pluss::Note matrix = pluss::make_note(
      std::move(m_flat),
      std::span<const std::int64_t>(m_sizes.data(), m_sizes.size()));
  pluss::NotePrintOptions sum2d;
  sum2d.max_elements = 10;
  sum2d.edge_items = 2;
  std::cout << pluss::print_note(matrix, sum2d) << "\n\n";

  std::cout << "=== 3D summarized ===\n";
  const std::array<std::int64_t, 3> t_sizes{2, 3, 4};
  std::vector<std::string> t_flat;
  t_flat.reserve(24);
  for (int i = 0; i < 24; ++i) t_flat.push_back("t" + std::to_string(i));
  pluss::Note cube = pluss::make_note(
      std::move(t_flat),
      std::span<const std::int64_t>(t_sizes.data(), t_sizes.size()));
  pluss::NotePrintOptions sum3d;
  sum3d.max_elements = 9;
  sum3d.edge_items = 1;
  sum3d.max_cell_chars = 8;
  std::cout << pluss::print_note(cube, sum3d) << "\n\n";

  std::cout << "=== header-only / values-only ===\n";
  pluss::NotePrintOptions hdr;
  hdr.content_mode = pluss::coden::NotePrintContentMode::HeaderOnly;
  std::cout << pluss::print_note(matrix, hdr) << "\n";

  pluss::NotePrintOptions val;
  val.content_mode = pluss::coden::NotePrintContentMode::ValuesOnly;
  val.edge_items = 1;
  val.max_elements = 8;
  std::cout << pluss::print_note(matrix, val) << "\n\n";

  std::cout << "=== metadata compact / none ===\n";
  pluss::NotePrintOptions compact;
  compact.metadata_mode = pluss::coden::NotePrintMetadataMode::Compact;
  compact.max_elements = 8;
  compact.edge_items = 1;
  std::cout << pluss::print_note(matrix, compact) << "\n\n";

  pluss::NotePrintOptions no_meta;
  no_meta.metadata_mode = pluss::coden::NotePrintMetadataMode::None;
  no_meta.max_elements = 8;
  no_meta.edge_items = 1;
  std::cout << pluss::print_note(matrix, no_meta) << "\n\n";

  std::cout << "=== bytes escaped vs hex ===\n";
  std::string raw = "A";
  raw.push_back('\0');
  raw += "B";
  raw.push_back(static_cast<char>(0x7F));
  raw.push_back(static_cast<char>(0xC3));
  raw.push_back(static_cast<char>(0xA9));
  pluss::Note bytes = pluss::bytes_content(raw);

  pluss::NotePrintOptions bytes_escaped;
  bytes_escaped.escape_controls = true;
  std::cout << pluss::print_note(bytes, bytes_escaped) << "\n";

  pluss::NotePrintOptions bytes_hex;
  bytes_hex.bytes_mode = pluss::coden::NotePrintBytesMode::Hex;
  std::cout << pluss::print_note(bytes, bytes_hex) << "\n\n";

  std::cout << "=== to_string API ===\n";
  std::string rendered = pluss::to_string(v, clip);
  std::cout << rendered << "\n";

  return 0;
}
