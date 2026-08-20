#pragma once

/*
PlussBI public surface seed.

PlussBI is the programmatic dashboard and analytics layer built around:
  - pluss::table::Nexus as the data workbench
  - Flash / PlussPl as the visualization engine
  - PlussView as the first native runtime host

This header intentionally stays small at the start. The implementation should
centralize shared behavior instead of duplicating query, filter, rendering, and
serialization logic across many UI objects.
*/

namespace pluss::bi {

class Portfolio;
class Board;
class Tile;
class Visual;
class Control;
class Filter;
class Session;

struct LayoutSpec;
struct ThemeSpec;
struct QuerySpec;
struct BindSpec;
struct ExportSpec;

} // namespace pluss::bi
