# PlussII Transient

The latest validated development build. It carries no version number and is replaced on every update, so expect it to change. Only Steady releases are versioned and tagged.

## Changes since 0.0.6

- PlussView: Save As → PNG and PDF now write only the content. A plot is saved at its own size with no window chrome, and a table is saved from its column headers down, with no title card, search box or scrollbars, cropped to the columns and rows that are shown.
- PlussView: every table size (column limits, sort-arrow size, row heights) is now one block of values in units of the font size, `HtmlTableGeometry`, so tables scale with the font and the numbers are changed in one place.
- PlussView on Windows and Linux now matches macOS: the toolbar has View and other menus and a "Save As" button sized to its label (Linux gained a toolbar), PDF export works on every platform, and on Linux Save As asks where to save when zenity or kdialog is installed.
- PlussView: resizing a column of a large table is much faster. Styles are resolved once per kind of cell and text widths once per cell, so a drag does arithmetic instead of re-resolving every cell.
- PlussView: a column you drag or auto-fit keeps that width and the other columns share the rest, so narrowing or restoring a column never leaves a gap.
- PlussView: table columns now behave like browser tabs. A few columns grow to fill the window; as more are added they shrink together, down to a minimum width, with the text ending in an ellipsis, and then the table scrolls sideways. Drag a column border, or double-click it, to see the full text of a column.
- PlussView: tables fit their content. A short table is only as tall as its rows, the header band covers just the columns, and an unresized column is as wide as its header and first values need (up to a limit) instead of a fixed width, so short names no longer wrap.
- PlussView: the sort arrows are a stacked pair of triangles, which leaves more room for column names.
- PlussView: saved PNG and PDF tables leave out the sort arrows.
- PlussView: dragging or double-clicking a column border is much more responsive. Only the column that moved is measured again, instead of the whole table.
- PlussView: the Save As button is sized from its own title, so the label is never clipped.

## Compatibility

The distribution requires CMake 3.28 or newer and a C++20 compiler. See the download page for the archive for your system.
