# Find current documentation

Plussible publishes a documentation index at `https://plussible.org/docs-manifest.json`. Each entry has a `slug`, `source`, `title`, and `space`. Search the titles and slugs to identify the relevant method and guide pages. Fetch a selected Markdown source at `https://plussible.org/docs-content/<source>` and read its signature, parameter descriptions, examples, and linked options. The reader-facing page is `https://plussible.org/docs/<slug>`.

For example, a Series mean request maps to the source page `PlussQit/docs/sphinx/api/series/mean.md`. A DataFrame describe request maps to `PlussMic/docs/sphinx/api/dataframe/describe.md`. A DataFrame line chart request maps to `PlussMic/docs/sphinx/api/dataframe-plot/line.md`. Those paths identify source pages, not proof that an arbitrary overload exists in the user's installed version.

If the published documentation is unavailable, use the corresponding documentation in a user-provided checkout or release tag. If no authoritative page is reachable, do not claim a generated call was verified.
