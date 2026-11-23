# PlussII Transient

The latest validated development build. It carries no version number and is replaced on every update, so expect it to change. Only Steady releases are versioned and tagged.

## Changes since 0.0.6

- PlussView: Save As → PNG and PDF now write only the content. A plot is saved at its own size with no window chrome, and a table is saved from its column headers down, with no title card, search box or scrollbars, cropped to the columns and rows that are shown.
- PlussView: every table size (column limits, sort-arrow size, row heights) is now one block of values in units of the font size, `HtmlTableGeometry`, so tables scale with the font and the numbers are changed in one place.
- PlussView on Windows and Linux now matches macOS: the toolbar has View and other menus and a "Save As" button sized to its label (Linux gained a toolbar), PDF export works on every platform, and on Linux Save As asks where to save when zenity or kdialog is installed.
- PlussView: table text no longer wraps by default, like a spreadsheet: one line per cell ending in an ellipsis, so a row's height follows the font and resizing a column never changes any row (set `value_max_lines` in `HtmlTableGeometry` to let long text wrap). A number that does not fit is shown as `#` repeated across the cell instead of being cut.
- PlussView: dragging a column of a table with many columns is about ten times faster to lay out (the table is measured once per frame instead of twice with no cache), headers stay on one line and end in an ellipsis when their column is narrow, and the window no longer flickers while macOS decides to activate it.
- PlussView: resizing a column of a large table is much faster. Styles are resolved once per kind of cell and text widths once per cell, so a drag does arithmetic instead of re-resolving every cell.
- PlussView: dragging or auto-fitting a column changes only that column. The last column is the table's automatic column: it takes whatever room the others leave, so the table always fills the panel, and it has no divider of its own. Past its minimum the table scrolls sideways. `html_table_geometry().slack` switches to a spreadsheet-style blank grid instead (`HtmlTableSlack::Continuation`).
- PlussView: clicking a column header no longer sorts the table; only the up and down triangles do.
- PlussView: the last column really is the table's automatic column by default. An earlier build documented this but still ended a resized table with a blank continuation column; `HtmlTableGeometry::slack` now defaults to `HtmlTableSlack::LastColumn`.
- PlussView: a NaN or infinity cell in a table is now a light tint with readable text, not a solid dark block. The tint is computed from the theme (the built-in renderer does not read CSS `color-mix`).
- PlussView: the guide is rewritten around `.show()`, with pictures of the real window: views, number formats, themes, sorting, column widths, the plot window and Save As. The pixel-level canvas, scene and renderer pages are removed from the public reference: PlussView is a window, not a library.
- PlussView: table columns now behave like browser tabs. A few columns grow to fill the window; as more are added they shrink together, down to a minimum width, with the text ending in an ellipsis, and then the table scrolls sideways. Drag a column border, or double-click it, to see the full text of a column.
- PlussView: tables fit their content. A short table is only as tall as its rows, the header band covers just the columns, and an unresized column is as wide as its header and first values need (up to a limit) instead of a fixed width, so short names no longer wrap.
- PlussView: the sort arrows are a stacked pair of triangles, which leaves more room for column names.
- PlussView: saved PNG and PDF tables leave out the sort arrows.
- PlussView: dragging or double-clicking a column border is much more responsive. Only the column that moved is measured again, instead of the whole table.
- PlussView: the Save As button is sized from its own title, so the label is never clipped.

## Compatibility

The distribution requires CMake 3.28 or newer and a C++20 compiler. See the download page for the archive for your system.
