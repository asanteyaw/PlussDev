# PlussMic Docs Hub

The table/plot API references live in the PlussMic Sphinx site, which is built separately from the root PlussQit docs.

If you are working with `DataFrame`, `Slate`, joins, renaming, text cleanup,
or day-to-day table workflows, use the PlussMic docs site directly. The root docs
site links out to it because the table docs are built from the PlussMic tree.

Recommended reading order:

1. `DataFrame Guide`
2. `Part V: DataFrame Workflows`
3. `DataFrame API Reference`
4. `Slate API Reference`

Use these links directly:

- <a href="file:///Users/yawasante/software/cxx/PlussDev/PlussMic/docs/sphinx/_build/html/index.html">PlussMic Docs Landing</a>
- <a href="file:///Users/yawasante/software/cxx/PlussDev/PlussMic/docs/sphinx/_build/html/manual.html">PlussMic User Manual</a>
- <a href="file:///Users/yawasante/software/cxx/PlussDev/PlussMic/docs/sphinx/_build/html/dataframe.html">DataFrame Guide</a>
- <a href="file:///Users/yawasante/software/cxx/PlussDev/PlussMic/docs/sphinx/_build/html/manual/part05_dataframe_workflows.html">Part V: DataFrame Workflows</a>
- <a href="file:///Users/yawasante/software/cxx/PlussDev/PlussMic/docs/sphinx/_build/html/dataframe_api_reference.html">DataFrame API Reference</a>
- <a href="file:///Users/yawasante/software/cxx/PlussDev/PlussMic/docs/sphinx/_build/html/dataframe_plot_api_reference.html">DataFrame Plot API Reference</a>
- <a href="file:///Users/yawasante/software/cxx/PlussDev/PlussMic/docs/sphinx/_build/html/column_ref_api_reference.html">ColumnRef API Reference</a>
- <a href="file:///Users/yawasante/software/cxx/PlussDev/PlussMic/docs/sphinx/_build/html/slate_api_reference.html">Slate API Reference</a>
- <a href="file:///Users/yawasante/software/cxx/PlussDev/PlussMic/docs/sphinx/_build/html/atlas_api_reference.html">Atlas API Reference</a>

If any link opens stale content, rebuild PlussMic docs first:

```bash
cd /Users/yawasante/software/cxx/PlussDev/PlussMic/docs/sphinx
/Users/yawasante/software/cxx/PlussDev/.venv_docs/bin/sphinx-build -b html . _build/html
```
