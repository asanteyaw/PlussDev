#pragma once

#include <cstdint>
#include <iosfwd>
#include <string>

namespace pluss::kindle {
class Stream;
class Catalog;
class Archive;
}

namespace pluss::kindle::display {

struct DisplayOptions {
  std::int64_t terminal_width{0};   // <= 0 => detect from terminal
  std::int64_t terminal_height{0};  // <= 0 => detect from terminal
  std::int64_t preview_values{4};   // per stream preview values
  std::int64_t edge_items{4};       // head items before ellipsis
  std::int64_t tail_items{2};       // tail items after ellipsis
  bool show_meta{true};
};

std::string render(const Stream& stream, const DisplayOptions& options = {});
std::string render(const Catalog& catalog, const DisplayOptions& options = {});
std::string render(const Archive& archive, const DisplayOptions& options = {});

void print(std::ostream& os, const Stream& stream, const DisplayOptions& options = {});
void print(std::ostream& os, const Catalog& catalog, const DisplayOptions& options = {});
void print(std::ostream& os, const Archive& archive, const DisplayOptions& options = {});

} // namespace pluss::kindle::display
