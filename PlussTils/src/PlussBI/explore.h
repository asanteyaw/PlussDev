#pragma once

/*
Internal BI exploration surface.

The public-facing BI API should eventually live under:
  - include/pluss/bi/
  - include/BI_tools/

This file stays in src/PlussBI as the implementation-side entrypoint for
architecture notes, development staging, and local experimentation while the
surface is still forming.

See:
  - src/PlussBI/ARCHITECTURE.md
  - include/pluss/bi/bi.h
*/

namespace pluss::bi {

class Studio;
class Board;
class Tile;
class View;
class Control;
class Filter;
class Session;

struct LayoutSpec;
struct ThemeSpec;
struct QuerySpec;
struct BindSpec;
struct ExportSpec;

} // namespace pluss::bi
