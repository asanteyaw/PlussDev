#pragma once

#include "pluss/export.h"

#include <cstdint>
#include <iosfwd>
#include <string>

#include "CodeN/core/Note.h"

namespace pluss::coden {

enum class NotePrintMetadataMode : ::std::uint8_t {
  Full,
  Compact,
  None,
};

enum class NotePrintContentMode : ::std::uint8_t {
  Both,
  HeaderOnly,
  ValuesOnly,
};

enum class NotePrintBytesMode : ::std::uint8_t {
  Escaped,
  Hex,
};

struct NotePrintOptions {
  ::std::int64_t edge_items = 2;
  ::std::int64_t max_elements = 256;
  ::std::int64_t max_cell_chars = 96;

  NotePrintMetadataMode metadata_mode = NotePrintMetadataMode::Full;
  NotePrintContentMode content_mode = NotePrintContentMode::Both;
  NotePrintBytesMode bytes_mode = NotePrintBytesMode::Escaped;

  bool quote_strings = true;
  bool escape_controls = true;

  bool show_shape = true;
  bool show_dim = true;
  bool show_numel = true;
  bool show_encoding = true;
};

CODEN_API ::std::string to_string(const Note& n, const NotePrintOptions& opt = {});
CODEN_API ::std::string print_note(const Note& n, const NotePrintOptions& opt = {});
CODEN_API ::std::ostream& print_note(::std::ostream& os, const Note& n, const NotePrintOptions& opt = {});
CODEN_API ::std::ostream& operator<<(::std::ostream& os, const Note& n);

} // namespace pluss::coden
