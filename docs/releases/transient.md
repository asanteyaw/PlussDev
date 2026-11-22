# PlussII Transient

The latest validated development build. It carries no version number and is replaced on every update, so expect it to change. Only Steady releases are versioned and tagged.

## Changes since 0.0.6

- PlussView: Save As → PNG and PDF now write only the content. A plot is saved at its own size with no window chrome, and a table is saved from its column headers down, with no title card, search box or scrollbars, cropped to the columns and rows that are shown.
- PlussView: dragging or double-clicking a column border is much more responsive. Only the column that moved is measured again, instead of the whole table.
- PlussView: the Save As button is sized from its own title, so the label is never clipped.

## Compatibility

The distribution requires CMake 3.28 or newer and a C++20 compiler. See the download page for the archive for your system.
