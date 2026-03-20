# PlussMic Docs Hub

The table/plot API references live in the PlussMic Sphinx site, which is built separately from the root PlussQit docs.

Use these links directly:

- <a href="file:///Users/yawasante/software/cxx/PlussDev/PlussMic/docs/sphinx/_build/html/index.html">PlussMic Docs Landing</a>
- <a href="file:///Users/yawasante/software/cxx/PlussDev/PlussMic/docs/sphinx/_build/html/dataframe_api_reference.html">DataFrame API Reference</a>
- <a href="file:///Users/yawasante/software/cxx/PlussDev/PlussMic/docs/sphinx/_build/html/dataframe_plot_api_reference.html">DataFrame Plot API Reference</a>
- <a href="file:///Users/yawasante/software/cxx/PlussDev/PlussMic/docs/sphinx/_build/html/column_ref_api_reference.html">ColumnRef API Reference</a>
- <a href="file:///Users/yawasante/software/cxx/PlussDev/PlussMic/docs/sphinx/_build/html/thread_api_reference.html">Thread API Reference</a>
- <a href="file:///Users/yawasante/software/cxx/PlussDev/PlussMic/docs/sphinx/_build/html/atlas_api_reference.html">Atlas API Reference</a>

If any link opens stale content, rebuild PlussMic docs first:

```bash
cd /Users/yawasante/software/cxx/PlussDev/PlussMic/docs/sphinx
/Users/yawasante/software/cxx/PlussDev/.venv_docs/bin/sphinx-build -b html . _build/html
```
