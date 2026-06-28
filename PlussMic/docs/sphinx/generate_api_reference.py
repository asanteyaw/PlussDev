from __future__ import annotations

import pathlib
import re
from dataclasses import dataclass
from typing import Callable

ROOT = pathlib.Path('/Users/yawasante/software/cxx/PlussDev')
DOCS = ROOT / 'PlussMic/docs/sphinx'
GENERIC_CSV_PATH = "path/to/data.csv"
GENERIC_XLSX_PATH = "path/to/data.xlsx"
GENERIC_JSON_PATH = "path/to/data.json"
GENERIC_PARQUET_PATH = "path/to/data.parquet"
GENERIC_TXT_PATH = "path/to/data.txt"
GENERIC_OUTPUT_CSV_PATH = "path/to/output.csv"
GENERIC_OUTPUT_XLSX_PATH = "path/to/output.xlsx"
GENERIC_OUTPUT_JSON_PATH = "path/to/output.json"
GENERIC_OUTPUT_PARQUET_PATH = "path/to/output.parquet"
GENERIC_OUTPUT_TXT_PATH = "path/to/output.txt"

@dataclass
class Method:
    signature: str
    name: str
    return_type: str
    params: list[tuple[str, str]]
    static: bool


CLASS_CONFIG = {
    'ColumnRef': {
        'header': ROOT / 'PlussMic/src/table/DataFrame.h',
        'output': DOCS / 'column_ref_api_reference.md',
        'title': 'ColumnRef API Reference',
        'subtitle': 'Practical ColumnRef API Reference',
        'intro': 'This page documents the public `ColumnRef` helper returned by DataFrame column-reference calls.',
    },
    'DataFrame': {
        'header': ROOT / 'PlussMic/src/table/DataFrame.h',
        'output': DOCS / 'dataframe_api_reference.md',
        'title': 'DataFrame API Reference',
        'subtitle': 'Practical DataFrame API Reference',
        'intro': 'This page documents the full public `DataFrame` surface with usage-first examples generated from the current table API.',
    },
    'DataFramePlotAccessor': {
        'header': ROOT / 'PlussMic/src/table/plot/DataFramePlot.h',
        'output': DOCS / 'dataframe_plot_api_reference.md',
        'title': 'DataFramePlotAccessor API Reference',
        'subtitle': 'Practical DataFramePlotAccessor API Reference',
        'intro': 'This page documents the public plotting accessor returned by `DataFrame::plot()` and `Atlas::plot()`.',
    },
    'DataFrameSqlAccessor': {
        'header': ROOT / 'PlussMic/src/table/sql/DataFrameSql.h',
        'output': DOCS / 'dataframe_sql_accessor_api_reference.md',
        'title': 'DataFrameSqlAccessor API Reference',
        'subtitle': 'Practical DataFrameSqlAccessor API Reference',
        'intro': 'This page documents the public SQL accessor returned by `DataFrame::sql()`.',
    },
    'DataFrameSqlPrepared': {
        'header': ROOT / 'PlussMic/src/table/sql/DataFrameSql.h',
        'output': DOCS / 'dataframe_sql_prepared_api_reference.md',
        'title': 'DataFrameSqlPrepared API Reference',
        'subtitle': 'Practical DataFrameSqlPrepared API Reference',
        'intro': 'This page documents the public prepared-statement helper returned by `DataFrame::prepare_sql()` or `DataFrameSqlAccessor::prepare()`.',
    },
    'Thread': {
        'header': ROOT / 'PlussMic/src/table/Thread.h',
        'output': DOCS / 'thread_api_reference.md',
        'title': 'Thread API Reference',
        'subtitle': 'Practical Thread API Reference',
        'intro': 'This page documents the full public `Thread` surface with direct typed-column examples.',
    },
    'ThreadValue': {
        'header': ROOT / 'PlussMic/src/table/Thread.h',
        'output': DOCS / 'thread_value_api_reference.md',
        'title': 'ThreadValue API Reference',
        'subtitle': 'Practical ThreadValue API Reference',
        'intro': 'This page documents the public scalar-like `ThreadValue` wrapper returned by thread and column-value accessors.',
    },
    'Atlas': {
        'header': ROOT / 'PlussMic/src/table/Atlas.h',
        'output': DOCS / 'atlas_api_reference.md',
        'title': 'Atlas API Reference',
        'subtitle': 'Practical Atlas API Reference',
        'intro': 'This page documents the full public `Atlas` surface for frame-stack workflows.',
    },
    'Stream': {
        'header': ROOT / 'PlussMic/src/kindle/Stream.h',
        'output': DOCS / 'stream_api_reference.md',
        'title': 'Stream API Reference',
        'subtitle': 'Practical Stream API Reference',
        'intro': 'This page documents the full public `Stream` surface for the Kindle model.',
    },
    'Catalog': {
        'header': ROOT / 'PlussMic/src/kindle/catalog.h',
        'output': DOCS / 'catalog_api_reference.md',
        'title': 'Catalog API Reference',
        'subtitle': 'Practical Catalog API Reference',
        'intro': 'This page documents the full public `Catalog` surface for named stream collections.',
    },
    'Archive': {
        'header': ROOT / 'PlussMic/src/kindle/Archive.h',
        'output': DOCS / 'archive_api_reference.md',
        'title': 'Archive API Reference',
        'subtitle': 'Practical Archive API Reference',
        'intro': 'This page documents the full public `Archive` surface for depth-stacked catalogs.',
    },
    'Slot': {
        'header': ROOT / 'PlussMic/src/bundle/Slot.h',
        'output': DOCS / 'slot_api_reference.md',
        'title': 'Slot API Reference',
        'subtitle': 'Practical Slot API Reference',
        'intro': 'This page documents the full public `Slot` surface for one-dimensional heterogeneous bundles.',
    },
    'Registry': {
        'header': ROOT / 'PlussMic/src/bundle/registry.h',
        'output': DOCS / 'registry_api_reference.md',
        'title': 'Registry API Reference',
        'subtitle': 'Practical Registry API Reference',
        'intro': 'This page documents the full public `Registry` surface for named bundle slots.',
    },
    'Aisle': {
        'header': ROOT / 'PlussMic/src/bundle/Aisle.h',
        'output': DOCS / 'aisle_api_reference.md',
        'title': 'Aisle API Reference',
        'subtitle': 'Practical Aisle API Reference',
        'intro': 'This page documents the full public `Aisle` surface for depth-stacked registries.',
    },
}


def clean_space(s: str) -> str:
    return re.sub(r'\s+', ' ', s).strip()


def split_top_level(text: str) -> list[str]:
    out = []
    cur = []
    depth_angle = depth_paren = depth_brace = depth_brack = 0
    i = 0
    while i < len(text):
        ch = text[i]
        if ch == '<':
            depth_angle += 1
        elif ch == '>':
            depth_angle = max(0, depth_angle - 1)
        elif ch == '(':
            depth_paren += 1
        elif ch == ')':
            depth_paren = max(0, depth_paren - 1)
        elif ch == '{':
            depth_brace += 1
        elif ch == '}':
            depth_brace = max(0, depth_brace - 1)
        elif ch == '[':
            depth_brack += 1
        elif ch == ']':
            depth_brack = max(0, depth_brack - 1)
        elif ch == ',' and depth_angle == depth_paren == depth_brace == depth_brack == 0:
            out.append(''.join(cur).strip())
            cur = []
            i += 1
            continue
        cur.append(ch)
        i += 1
    tail = ''.join(cur).strip()
    if tail:
        out.append(tail)
    return out


def parse_params(text: str) -> list[tuple[str, str]]:
    text = text.strip()
    if not text or text == 'void':
        return []
    params = []
    for part in split_top_level(text):
        part = clean_space(part)
        part = re.sub(r'\s*=\s*.*$', '', part)
        m = re.match(r'(.+?)\s+(\w+)$', part)
        if m:
            params.append((m.group(2), m.group(1)))
        else:
            params.append((f'arg{len(params)+1}', part))
    return params


def parse_signature(sig: str, class_name: str) -> Method | None:
    sig = clean_space(sig)
    sig = sig.split('{', 1)[0].strip()
    sig = sig.rstrip(';').strip()
    sig = sig.replace(' noexcept', '')
    if not sig or '(' not in sig:
        return None
    before, rest = sig.split('(', 1)
    params_text = rest.rsplit(')', 1)[0]
    before = clean_space(before)
    name = before.split()[-1]
    if name == class_name or name == f'~{class_name}':
        return None
    if name.startswith('operator'):
        return None
    static = before.startswith('static ')
    return_type = before[: before.rfind(name)].strip()
    if static:
        return_type = return_type[len('static '):].strip()
    if not return_type:
        return None
    return Method(sig, name, return_type, parse_params(params_text), static)


def extract_class_body(text: str, class_name: str) -> str:
    m = re.search(rf'class\s+{re.escape(class_name)}\s*\{{', text)
    if not m:
        raise RuntimeError(f'class {class_name} not found')
    i = m.end()
    depth = 1
    body = []
    while i < len(text):
        ch = text[i]
        if ch == '{':
            depth += 1
        elif ch == '}':
            depth -= 1
            if depth == 0:
                break
        body.append(ch)
        i += 1
    return ''.join(body)


def extract_methods(class_name: str, header_path: pathlib.Path) -> list[Method]:
    text = header_path.read_text()
    body = extract_class_body(text, class_name)
    depth = 0
    visibility = 'private'
    decl = []
    methods: list[Method] = []
    i = 0
    while i < len(body):
        ch = body[i]
        if ch == '/' and i + 1 < len(body) and body[i + 1] == '/':
            while i < len(body) and body[i] != '\n':
                i += 1
            continue
        if ch == '/' and i + 1 < len(body) and body[i + 1] == '*':
            i += 2
            while i + 1 < len(body) and not (body[i] == '*' and body[i + 1] == '/'):
                i += 1
            i += 2
            continue
        if depth == 0:
            for marker in ('public:', 'private:', 'protected:'):
                if body.startswith(marker, i):
                    visibility = marker[:-1]
                    i += len(marker)
                    decl = []
                    break
            else:
                if ch == '{':
                    depth += 1
                elif ch == '}':
                    depth = max(0, depth - 1)
                decl.append(ch)
                if ch == ';' and visibility == 'public':
                    statement = clean_space(''.join(decl))
                    decl = []
                    if any(statement.startswith(x) for x in ('enum class ', 'struct ', 'class ', 'using ', 'friend ')):
                        pass
                    else:
                        method = parse_signature(statement, class_name)
                        if method:
                            methods.append(method)
                elif ch == ';':
                    decl = []
                elif ch == '{':
                    statement = clean_space(''.join(decl))
                    if visibility == 'public':
                        method = parse_signature(statement, class_name)
                        if method:
                            methods.append(method)
                    decl = []
                i += 1
                continue
            continue
        else:
            if ch == '{':
                depth += 1
            elif ch == '}':
                depth -= 1
            i += 1
            continue
        i += 1
    seen = set()
    out = []
    for m in methods:
        if m.signature not in seen:
            seen.add(m.signature)
            out.append(m)
    return out


def user_signature(method: Method, class_name: str) -> str:
    sig = method.signature
    sig = sig.replace('double', 'scalar')
    if not method.static:
        sig = re.sub(rf'^{re.escape(method.return_type)}\s+{re.escape(method.name)}\(', f'{method.return_type} {method.name}(self, ', sig)
        sig = sig.replace('(self, )', '(self)')
    return sig


def returns_doc(ret: str) -> str:
    base = ret.strip()
    descriptions = {
        'bool': 'Boolean status or membership result.',
        'void': 'Performs the side effect and returns no value.',
        'std::int64_t': 'Integer count, size, or positional result.',
        'std::string': 'Rendered string result.',
        'const std::vector<std::string>&': 'Borrowed list of names.',
        'std::vector<Thread>': 'Materialized collection of thread objects.',
        'std::vector<Stream>': 'Materialized collection of stream objects.',
        'std::vector<Slot>': 'Materialized collection of slot objects.',
        'const std::vector<DataFrame>&': 'Borrowed collection of frames.',
        'const std::vector<Catalog>&': 'Borrowed collection of catalogs.',
        'const std::vector<Registry>&': 'Borrowed collection of registries.',
    }
    desc = descriptions.get(base, 'Value produced by this call.')
    return f'`{base}`\n    {desc}'


def params_doc(params: list[tuple[str, str]]) -> str:
    if not params:
        return '`None`'
    param_notes = {
        'filepath': 'File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.',
        'source_type': 'Storage format selector such as `"csv"`, `"xlsx"`, `"json"`, or `"parquet"`.',
        'sheet_name': 'Worksheet name to read from or write to inside an Excel workbook.',
        'sheet_index': 'Zero-based worksheet position inside an Excel workbook.',
        'header': 'Controls whether feature names are written to, or expected from, the file header row.',
        'add_cases': 'When `true`, the current case axis is exported as its own column so row labels can be reconstructed when the file is loaded again.',
        'case_label': 'Optional column label to use for the exported case axis when `add_cases=true`.',
        'na_rep': 'String written anywhere a null or missing value must be serialized.',
        'float_format': 'Optional numeric formatting string used when floating-point values are written to text output.',
        'sep': 'Column separator used for delimited text files such as CSV.',
        'quotechar': 'Quote character used when writing delimited text output.',
        'doublequote': 'When `true`, embedded quote characters are escaped by doubling them in CSV-style output.',
        'line_terminator': 'Line ending written between serialized rows in text-based export.',
        'decimal': 'Decimal marker used when formatting floating-point output.',
        'encoding': 'Text encoding used when reading or writing text-based files.',
        'new_cases': 'Replacement case labels in the exact order you want the rows to follow.',
        'cases': 'Case-label thread to attach to the current object.',
        'drop_feature': 'When `true`, the feature promoted to cases is removed from the ordinary feature columns.',
        'feature_name': 'Name of the target feature to read, rewrite, sort by, or promote to cases.',
        'left_on': 'Key feature name(s) on the left frame used to match rows during `unify(...)`.',
        'right_on': 'Key feature name(s) on the right frame used to match rows during `unify(...)`. Leave empty to reuse the same names as `left_on`.',
        'how': 'Join mode. `DataFrame::unify(...)` currently supports `"inner"` and `"left"`.',
        'right_suffix': 'Suffix added to right-hand feature names only when a non-key right feature would otherwise collide with an existing left feature name.',
        'validate': 'Optional key-cardinality check for `unify(...)`: `"many_to_many"` (default), `"one_to_one"`, `"one_to_many"`, or `"many_to_one"`.',
        'field': 'Name of the target stream in the catalog.',
        'key': 'Name of the target slot or stream.',
        'keys': 'Collection of slot or stream names to keep, drop, or rewrite together.',
        'names': 'Collection of feature, slot, or stream names supplied in positional order.',
        'features': 'Feature name(s) that select the target DataFrame columns for the operation.',
        'n': 'Number of rows, entries, or depth items to keep in the preview.',
        'start': 'Inclusive starting position of the slice.',
        'stop': 'Exclusive ending position of the slice.',
    }
    lines = []
    for name, typ in params:
        label_type = typ.replace('double', 'scalar')
        note = param_notes.get(name, 'See the signature for the supported argument form.')
        lines.append(f'`{name}` : `{label_type}`\n    {note}')
    return '\n\n'.join(lines)


def generic_note(class_name: str, method: Method) -> str:
    name = method.name
    method_notes = {
        'DataFrame': {
            'aggregate': 'Builds a compact summary frame from one or more aggregation specs, so you can inspect computed metrics instead of the full row set.',
            'bottom': 'Returns the rows with the smallest values in the selected feature.',
            'cases_series': 'Returns the current case axis as a `Series`, which is useful when you need the row labels as data.',
            'combine': 'Combines this frame with another frame according to the supplied combine policy, such as feature-wise update or join-like alignment.',
            'copy': 'Returns a detached copy of the frame so later edits to the result do not affect the source object.',
            'count_by': 'Groups by one feature and returns per-group row counts.',
            'count_null_rows': 'Counts how many rows contain at least one null value.',
            'describe': 'Returns descriptive summary statistics for the selected numeric features.',
            'drop': 'Removes one or more features and returns the remaining frame.',
            'equals': 'Checks whether two frames have the same structure, labels, and values.',
            'group_by': 'Groups rows by one or more key features and computes the requested aggregations for each group.',
            'has_feature': 'Checks whether a feature name exists in the frame.',
            'has_feature': 'Checks whether a feature name exists in the frame.',
            'head': 'Returns the first `n` rows of the frame.',
            'with_cases': 'Replaces the current case axis with the supplied thread. The row values stay in the same order, but later case-aware operations now use the new labels.',
            'with_cases_': 'Replaces the current case axis in place. The row values stay in the same order, but later case-aware operations now use the new labels.',
            'insert': 'Inserts a new feature at the requested position and shifts the later features to the right.',
            'join': 'Adds compatible features from another frame, aligning rows by the shared case axis.',
            'set_cases': 'Promotes one feature to become the case axis. With the default `drop_feature=true`, that feature stops being a normal column and becomes the row labels instead.',
            'reset_cases': 'Materializes the current case axis as a normal feature, then restores the default positional cases `0..n-1`.',
            'align_cases': 'Reorders rows so their case labels match the supplied `new_cases` order. Every requested case must already exist, and the output case axis is rewritten to that order.',
            'sort_cases': 'Sorts rows by the current case axis rather than by a data feature.',
            'add_prefix': 'Prepends the supplied text to every feature name. Row values, row order, and case labels stay unchanged.',
            'add_suffix': 'Appends the supplied text to every feature name. Row values, row order, and case labels stay unchanged.',
            'max': 'Computes per-feature maxima for the selected numeric features.',
            'mean': 'Computes per-feature means for the selected numeric features.',
            'mean_by': 'Groups by one feature and returns the mean of another feature within each group.',
            'min': 'Computes per-feature minima for the selected numeric features.',
            'note_features': 'Returns only the note-backed features.',
            'pivot': 'Reshapes long-form rows into a wider table indexed by one feature and expanded by another.',
            'pivot_table': 'Builds a pivoted summary table using an aggregation function when multiple rows map to the same cell.',
            'relabel_features': 'Replaces the full feature label set with the supplied names in positional order.',
            'rename': 'Renames one feature without changing its values.',
            'rename_': 'Renames one feature in place without changing its values.',
            'rename_features': 'Renames several features in one call.',
            'reorder': 'Reorders the feature columns without changing any row values.',
            'repeated_cases': 'Returns the rows whose current case labels are duplicates of an earlier row.',
            'rolling': 'Computes rolling-window features and returns them as a derived frame.',
            'sample': 'Returns a random sample of rows.',
            'save_csv': 'Writes the frame to CSV. Set `add_cases=true` when you want the current case axis exported as a regular column so row labels survive the round trip.',
            'save_excel': 'Writes the frame to an Excel sheet. Set `add_cases=true` when you want the current case axis exported as a regular column.',
            'save_json': 'Writes the frame to JSON using the table export route for later reload or interchange.',
            'save_parquet': 'Writes the frame to Parquet for compact typed storage.',
            'save_txt': 'Writes the frame to a plain-text table preview suitable for inspection or lightweight logging.',
            'save_as': 'Writes the frame using the explicitly supplied output format selector rather than a dedicated saver overload.',
            'series_features': 'Returns only the numeric series-backed features.',
            'assign': 'Adds or replaces derived features computed from the current frame. Use this when the new column depends on one or more existing columns.',
            'set': 'Opens an assignment helper so you can write `df.set(...) = ...` and replace or create features declaratively.',
            'set_axis': 'Replaces the full feature label set with the supplied names in positional order.',
            'set_feature_axis': 'Replaces the full feature label set with the supplied names in positional order.',
            'set_features': 'Adds or replaces one or more features and returns the updated frame.',
            'set_features_': 'Adds or replaces one or more features in place.',
            'slice': 'Returns a contiguous row slice `[start, stop)` from the frame.',
            'sort_values': 'Reorders rows using one or more feature values as sort keys.',
            'std': 'Computes per-feature standard deviations for the selected numeric features.',
            'stitch_on': 'Matches rows from two frames using key features and appends the matched features from the right-hand frame.',
            'sum': 'Computes per-feature sums for the selected numeric features.',
            'tail': 'Returns the last `n` rows of the frame.',
            'take': 'Returns the rows at the requested integer positions.',
            'tally': 'Counts how many rows fall into each distinct value of the grouping feature.',
            'timeline_features': 'Returns only the timeline-backed features.',
            'top': 'Returns the rows with the largest values in the selected feature.',
            'transform': 'Rewrites one feature, several features, or all features by applying a user function to each targeted thread.',
            'transpose': 'Swaps rows and features so former feature names become row labels.',
            'unique_cases': 'Keeps only the first occurrence of each case label and drops later duplicates.',
            'unify': 'Joins two frames by key values. Use `right_on=""` when both frames use the same key name, `how="left"` to preserve left rows, and `validate` when you want duplicate-key checks such as `"many_to_one"`.',
            'unsqueeze': 'Promotes the frame into a depth-1 `Atlas` so it can participate in multi-frame workflows.',
            'query': 'Keeps only the rows that satisfy the supplied filter expression or mask-producing query.',
            'where': 'Keeps only the rows where the supplied boolean mask is true.',
            'filter': 'Keeps only the rows where the supplied boolean mask is true.',
            'reindex_cases': 'Builds a new case axis from the supplied labels and aligns rows onto that case order.',
            'reindex_like': 'Realigns this frame so its cases match another frame.',
            'conform_to': 'Reorders and aligns this frame so it is compatible with another frame for downstream joins or arithmetic.',
            'update': 'Overlays matching features from another frame onto this frame using aligned cases.',
            'var': 'Computes per-feature variances for the selected numeric features.',
            'view': 'Returns a lightweight row view of the requested slice.',
            'window': 'Computes one or more window-based features such as rolling means over selected columns.',
            'set_features': 'Returns a new frame with one added or replaced feature.',
            'set_features_': 'Adds or replaces one feature in place.',
        },
        'Catalog': {
            'apply': 'Applies a function to one named stream and returns the updated catalog.',
            'apply_': 'Applies a function to one named stream in place.',
            'apply_all': 'Applies the same function to every stream and returns the updated catalog.',
            'apply_all_': 'Applies the same function to every stream in place.',
            'filter': 'Keeps only the rows where the supplied mask stream is true.',
            'from_json': 'Builds a catalog from JSON input using the PlussMic JSON loader route.',
            'get_stream': 'Returns one named stream from the catalog.',
            'has': 'Checks whether a named stream exists.',
            'has_stream': 'Checks whether a named stream exists.',
            'head': 'Returns the first `n` rows across every stream in the catalog.',
            'masked_select': 'Keeps only the rows where the supplied mask stream is true.',
            'max': 'Computes the maximum value of one named numeric stream.',
            'mean': 'Computes the mean value of one named numeric stream.',
            'min': 'Computes the minimum value of one named numeric stream.',
            'note': 'Returns one named stream interpreted as a note/text stream.',
            'query': 'Keeps only the rows selected by the supplied row-wise query function or expression.',
            'rename_': 'Renames one or more streams in place without changing their values.',
            'sample': 'Returns a random sample of rows across all streams.',
            'series': 'Returns one named stream interpreted as a numeric series stream.',
            'set': 'Adds or replaces one named stream and returns the updated catalog.',
            'set_': 'Adds or replaces one named stream in place.',
            'set_stream': 'Adds or replaces one named stream and returns the updated catalog.',
            'set_stream_': 'Adds or replaces one named stream in place.',
            'set_streams': 'Adds or replaces several named streams and returns the updated catalog.',
            'set_streams_': 'Adds or replaces several named streams in place.',
            'slice': 'Returns a contiguous row slice `[start, stop)` across every stream.',
            'sum': 'Computes the sum of one named numeric stream.',
            'tail': 'Returns the last `n` rows across every stream in the catalog.',
            'take': 'Returns the rows at the requested integer positions across every stream.',
            'timeindex': 'Returns one named stream interpreted as a time-index stream.',
            'timeline': 'Returns one named stream interpreted as a timeline stream.',
            'transform': 'Rewrites one stream, several streams, or all streams by applying a user function.',
            'select': 'Keeps only the named streams and preserves their shared row alignment.',
            'drop': 'Removes the named streams and keeps the remaining catalog structure intact.',
            'rename': 'Renames one or more streams without changing their values.',
            'sort': 'Reorders rows using one stream as the sort key, while keeping the other streams aligned to the same row order.',
            'sort_by': 'Reorders rows using multiple sort keys, while keeping all streams aligned.',
            'group_by': 'Splits the catalog into sub-catalogs keyed by the distinct values in the grouping stream.',
            'merge': 'Combines two catalogs by matching rows on a key stream.',
            'save_csv': 'Writes the catalog to CSV after aligning its streams into a table-like representation.',
            'save_excel': 'Writes the catalog to an Excel workbook after aligning its streams into a table-like representation.',
            'save_json': 'Writes the catalog to JSON for interchange or later reload.',
            'save_parquet': 'Writes the catalog to Parquet for compact typed storage.',
            'save_txt': 'Writes a text rendering of the catalog for inspection or logging.',
            'save_as': 'Writes the catalog using the explicitly supplied output format selector.',
            'where': 'Keeps only the rows where the supplied mask stream is true.',
        },
        'Registry': {
            'apply': 'Applies a function to one or more named slots and returns the updated registry.',
            'apply_': 'Applies a function to one or more named slots in place.',
            'apply_all': 'Applies the same function to every slot and returns the updated registry.',
            'clear': 'Removes all slots and returns an empty registry.',
            'clear_': 'Removes all slots in place.',
            'concat_values': 'Appends extra values onto one named slot and returns the updated registry.',
            'concat_values_': 'Appends extra values onto one named slot in place.',
            'drop_empty_slots': 'Removes slots whose payload is empty and returns the reduced registry.',
            'drop_empty_slots_': 'Removes empty slots in place.',
            'ensure_stream_compatible': 'Validates that the registry can be converted to a Kindle catalog without incompatible slot contents.',
            'filter_values': 'Keeps only the values in one named slot that satisfy the supplied predicate.',
            'filter_values_': 'Filters the values in one named slot in place.',
            'get_slot': 'Returns one named slot from the registry.',
            'has': 'Checks whether a named slot exists.',
            'has_all': 'Checks whether every requested slot name exists.',
            'has_any': 'Checks whether at least one requested slot name exists.',
            'has_slot': 'Checks whether a named slot exists.',
            'head': 'Returns the first `n` values from every slot in the registry.',
            'is_stream_compatible': 'Reports whether the registry can be converted to a Kindle catalog without incompatible slot contents.',
            'map': 'Transforms every slot with the same function and returns the updated registry.',
            'map_': 'Transforms every slot in place with the same function.',
            'push': 'Appends one new value to a named slot and returns the updated registry.',
            'push_': 'Appends one new value to a named slot in place.',
            'push_all': 'Appends the same new value to every slot and returns the updated registry.',
            'push_all_': 'Appends the same new value to every slot in place.',
            'rename_': 'Renames one or more slots in place without changing their stored values.',
            'sample': 'Returns a random sample of positions across every slot.',
            'set_slot_': 'Adds or replaces one named slot in place.',
            'set_slots_': 'Adds or replaces several named slots in place.',
            'slice': 'Returns a contiguous positional slice `[start, stop)` across every slot.',
            'slot_sizes': 'Returns the current length of each slot.',
            'take': 'Returns the values at the requested integer positions across every slot.',
            'take_values': 'Selects values from one named slot by integer position and returns the updated registry.',
            'take_values_': 'Selects values from one named slot by integer position in place.',
            'transform_values': 'Rewrites the values inside one named slot with a user function and returns the updated registry.',
            'transform_values_': 'Rewrites the values inside one named slot in place.',
            'type_profiles': 'Summarizes the value types stored inside each slot.',
            'tail': 'Returns the last `n` values from every slot in the registry.',
            'select': 'Keeps only the named slots and preserves their shared positional alignment.',
            'drop': 'Removes the named slots and keeps the remaining registry structure intact.',
            'rename': 'Renames one or more slots without changing their stored values.',
            'combine': 'Merges slots from another registry into this one, applying the collision suffix when keys overlap.',
            'save_csv': 'Writes the registry to CSV after materializing each slot into a tabular export form.',
            'save_excel': 'Writes the registry to an Excel workbook after materializing each slot into a tabular export form.',
            'save_json': 'Writes the registry to JSON for interchange or later reload.',
            'save_parquet': 'Writes the registry to Parquet for compact typed storage.',
            'save_txt': 'Writes a text rendering of the registry for inspection or logging.',
            'save_as': 'Writes the registry using the explicitly supplied output format selector.',
            'set_slot': 'Adds or replaces one named slot and returns the updated registry.',
            'set_slots': 'Adds or replaces several named slots in one call and returns the updated registry.',
        },
        'Thread': {
            'abs': 'Returns a numeric thread whose values are the absolute values of the current numeric payload.',
            'as_note': 'Interprets or converts the current payload as a note/text thread.',
            'as_series': 'Interprets or converts the current payload as a numeric series thread.',
            'as_timeindex': 'Interprets or converts the current payload as a time-index thread.',
            'as_timeline': 'Interprets or converts the current payload as a timeline thread.',
            'bfill': 'Backward-fills one thread by pulling the next valid value upward within the same payload.',
            'bucketize': 'Places numeric values into interval bins and returns the assigned bin labels as a new thread.',
            'btw': 'Builds a boolean mask telling you whether each row falls between lower and upper bounds.',
            'clear_validity_mask': 'Returns a thread with the validity mask removed, so all entries are treated as valid again.',
            'clear_validity_mask_': 'Removes the validity mask in place, so all entries are treated as valid again.',
            'clip': 'Clamps numeric values to the supplied lower and upper bounds.',
            'compare_rows': 'Compares two positions inside the same thread and returns the comparison result.',
            'dtype_name': 'Returns the scalar dtype name used by the current numeric payload.',
            'exp': 'Applies the elementwise exponential to a numeric thread.',
            'filter': 'Keeps only the entries where the supplied boolean mask is true.',
            'has_validity_mask': 'Reports whether the thread currently carries a validity mask.',
            'head': 'Returns the first `n` entries of the thread.',
            'is_valid': 'Checks whether the value at one position is marked valid by the thread validity mask.',
            'kind': 'Returns which payload kind this thread currently stores, such as Series, Note, Timeline, or TimeIndex.',
            'log': 'Applies the elementwise natural logarithm to a numeric thread.',
            'max': 'Returns the maximum value of a numeric thread.',
            'mean': 'Returns the mean value of a numeric thread.',
            'min': 'Returns the minimum value of a numeric thread.',
            'note': 'Returns the current payload as a note/text object.',
            'strip': 'Trims note-backed text values. Use `strip()` for whitespace or `strip(chars)` for an explicit trim set.',
            'lstrip': 'Trims only the left edge of note-backed text values. This is useful for prefixes such as leading `#` in IDs.',
            'rstrip': 'Trims only the right edge of note-backed text values. Use it when trailing markers should be removed without touching the left side.',
            'series': 'Returns the current payload as a numeric series object.',
            'slice': 'Returns a contiguous slice `[start, stop)` of the thread.',
            'std': 'Returns the standard deviation of a numeric thread.',
            'sum': 'Returns the sum of a numeric thread.',
            'tail': 'Returns the last `n` entries of the thread.',
            'take': 'Returns the entries at the requested integer positions.',
            'timeindex': 'Returns the current payload as a time-index object.',
            'timeline': 'Returns the current payload as a timeline object.',
            'validity_mask': 'Returns the current validity mask as a boolean series.',
            'var': 'Returns the variance of a numeric thread.',
            'window': 'Computes a window-based transformation such as a rolling mean over the thread values.',
            'with_validity_mask': 'Returns a new thread with the supplied validity mask attached.',
            'with_validity_mask_': 'Attaches the supplied validity mask in place.',
        },
        'Stream': {
            'abs': 'Returns a numeric stream whose values are the absolute values of the current numeric payload.',
            'as_note': 'Interprets or converts the current payload as a note/text stream.',
            'as_series': 'Interprets or converts the current payload as a numeric series stream.',
            'as_timeindex': 'Interprets or converts the current payload as a time-index stream.',
            'as_timeline': 'Interprets or converts the current payload as a timeline stream.',
            'bfill': 'Backward-fills one stream while preserving its case labels and stream-level structure.',
            'clear_cases': 'Returns a stream with its case labels removed.',
            'clear_cases_': 'Removes the stream case labels in place.',
            'clear_validity_mask': 'Returns a stream with the validity mask removed, so all entries are treated as valid again.',
            'clear_validity_mask_': 'Removes the validity mask in place, so all entries are treated as valid again.',
            'clip': 'Clamps numeric stream values to the supplied lower and upper bounds.',
            'drop_meta': 'Returns a stream with one metadata key removed.',
            'drop_meta_': 'Removes one metadata key in place.',
            'dtype_name': 'Returns the scalar dtype name used by the current numeric payload.',
            'exp': 'Applies the elementwise exponential to a numeric stream.',
            'filter': 'Keeps only the entries where the supplied boolean mask is true.',
            'from_thread': 'Builds a Kindle stream from a table thread.',
            'has_cases': 'Reports whether the stream currently carries explicit case labels.',
            'has_meta': 'Reports whether a metadata key exists on the stream.',
            'has_validity_mask': 'Reports whether the stream currently carries a validity mask.',
            'head': 'Returns the first `n` entries of the stream.',
            'kind': 'Returns which payload kind this stream currently stores, such as Series, Note, Timeline, or TimeIndex.',
            'log': 'Applies the elementwise natural logarithm to a numeric stream.',
            'max': 'Returns the maximum value of a numeric stream.',
            'mean': 'Returns the mean value of a numeric stream.',
            'meta': 'Returns the value stored under one metadata key.',
            'min': 'Returns the minimum value of a numeric stream.',
            'slice': 'Returns a contiguous slice `[start, stop)` of the stream.',
            'std': 'Returns the standard deviation of a numeric stream.',
            'sum': 'Returns the sum of a numeric stream.',
            'tail': 'Returns the last `n` entries of the stream.',
            'take': 'Returns the entries at the requested integer positions.',
            'validity_mask': 'Returns the current validity mask as a boolean series.',
            'var': 'Returns the variance of a numeric stream.',
            'with_cases': 'Returns a new stream with the supplied case labels attached.',
            'with_cases_': 'Attaches the supplied case labels in place.',
            'with_meta': 'Returns a new stream with one metadata key/value attached.',
            'with_meta_': 'Adds or updates one metadata key/value in place.',
            'with_validity_mask': 'Returns a new stream with the supplied validity mask attached.',
            'with_validity_mask_': 'Attaches the supplied validity mask in place.',
        },
        'Atlas': {
            'aggregate_over_frames': 'Reduces the full frame stack to one output by applying an aggregation function across the contained frames.',
            'align_depth': 'Reorders frames so their depth labels match the supplied label order.',
            'head': 'Returns the first `n` frames from the atlas depth axis.',
            'map_frames': 'Applies the same function to each frame and returns the transformed atlas.',
            'relabel_depth': 'Replaces the depth-label axis with the supplied labels while keeping frame contents unchanged.',
            'slice_frames': 'Returns a contiguous depth slice `[start, stop)` from the atlas.',
            'sort_depth': 'Sorts frames by their current depth labels.',
            'squeeze': 'Collapses a depth-1 atlas back into a single `DataFrame`.',
            'tail': 'Returns the last `n` frames from the atlas depth axis.',
        },
        'Archive': {
            'aggregate_over_catalogs': 'Reduces the full catalog stack to one output by applying an aggregation function across the contained catalogs.',
            'align_depth': 'Reorders catalogs so their depth labels match the supplied label order.',
            'head': 'Returns the first `n` catalogs from the archive depth axis.',
            'map_catalogs': 'Applies the same function to each catalog and returns the transformed archive.',
            'relabel_depth': 'Replaces the depth-label axis with the supplied labels while keeping catalog contents unchanged.',
            'slice_depth': 'Returns a contiguous depth slice `[start, stop)` from the archive.',
            'sort_depth': 'Sorts catalogs by their current depth labels.',
            'tail': 'Returns the last `n` catalogs from the archive depth axis.',
        },
        'Aisle': {
            'apply_slot': 'Applies a slot transform to one named key inside every registry and returns the updated aisle.',
            'concat_depth': 'Appends the registries from another aisle onto the current depth axis.',
            'head': 'Returns the first `n` registries from the aisle depth axis.',
            'map': 'Applies the same function to each registry and returns the transformed aisle.',
            'select': 'Keeps only the named keys inside every registry in the aisle.',
            'slice': 'Returns a contiguous depth slice `[start, stop)` from the aisle.',
            'tail': 'Returns the last `n` registries from the aisle depth axis.',
        },
        'Slot': {
            'concat': 'Appends the values from another slot and returns the combined slot.',
            'filter': 'Keeps only the slot values that satisfy the supplied predicate.',
            'head': 'Returns the first `n` values from the slot.',
            'push': 'Returns a new slot with one additional value appended.',
            'push_': 'Appends one additional value to the slot in place.',
            'slice': 'Returns a contiguous positional slice `[start, stop)` from the slot.',
            'tail': 'Returns the last `n` values from the slot.',
            'take': 'Returns the values at the requested integer positions.',
            'transform': 'Applies a value-wise transform to each slot entry and returns the updated slot.',
            'type_counts': 'Counts how many slot entries belong to each stored pluss type.',
        },
        'ColumnRef': {
            'head': 'Returns the first `n` values from the referenced column as a thread.',
            'mean': 'Computes the mean of the referenced numeric column.',
            'name': 'Returns the feature name associated with this column reference.',
            'thread': 'Returns the referenced column as a `Thread` object.',
        },
        'ThreadValue': {
            'as_note': 'Converts the wrapped scalar-like value into a `pluss::Note`.',
            'as_series': 'Converts the wrapped scalar-like value into a `pluss::Series`.',
            'as_timeindex': 'Converts the wrapped scalar-like value into a `pluss::TimeIndex`.',
            'as_timeline': 'Converts the wrapped scalar-like value into a `pluss::Timeline`.',
            'kind': 'Returns the underlying thread kind carried by this value wrapper.',
            'pluss::Note': 'Uses the implicit conversion operator to materialize the wrapped value as a `pluss::Note`.',
            'pluss::Series': 'Uses the implicit conversion operator to materialize the wrapped value as a `pluss::Series`.',
            'pluss::TimeIndex': 'Uses the implicit conversion operator to materialize the wrapped value as a `pluss::TimeIndex`.',
            'pluss::Timeline': 'Uses the implicit conversion operator to materialize the wrapped value as a `pluss::Timeline`.',
            'thread': 'Returns the wrapped single-value `Thread` object.',
        },
        'DataFramePlotAccessor': {
            'bar': 'Builds a bar chart from one feature, optionally using another feature as the x-axis.',
            'bar_stacked': 'Builds stacked bars so multiple y-features accumulate on the same axis positions.',
            'bootstrap': 'Builds a bootstrap summary plot for one numeric feature using the supplied resampling options.',
            'box': 'Builds a box plot for one or more numeric features.',
            'brief': 'Builds a compact summary figure for one or more numeric features.',
            'hist': 'Builds a histogram for one feature using the requested number of bins.',
            'line': 'Builds a line chart for one or more y-features, optionally against a chosen x-feature.',
            'scatter': 'Builds a scatter plot using one feature for x and another for y.',
        },
        'DataFrameSqlAccessor': {
            'execute': 'Runs a SQL query against the current frame and returns the result as a new `DataFrame`.',
            'prepare': 'Compiles a parameterized SQL statement that can be executed repeatedly with different bindings.',
        },
        'DataFrameSqlPrepared': {
            'execute': 'Runs the prepared SQL statement with the supplied parameters and returns the result as a new `DataFrame`.',
        },
    }
    specific = method_notes.get(class_name, {}).get(name)
    if specific:
        return specific
    if name.startswith('load'):
        return 'Loads the object from disk using the corresponding PlussMic loader route.'
    if name.startswith('save'):
        return 'Writes the current object to disk using the corresponding PlussMic saver route.'
    if name == 'defined':
        return 'Reports whether the current object owns a concrete payload that can be used safely in later operations.'
    if name == 'unique':
        return 'Reports whether the object satisfies the uniqueness constraint expected by its model, such as distinct labels or keys.'
    if name == 'empty':
        return 'Reports whether the current object has no rows, entries, slots, or depth items.'
    if name in {'nrows', 'ncols', 'nfeatures', 'nstreams', 'nfields', 'nslots', 'depth', 'size', 'count'}:
        return 'Returns a size/count summary for the current object without changing it.'
    if name in {'column_names', 'feature_names', 'field_names', 'keys', 'labels', 'axis0_labels', 'depth_labels'}:
        return 'Returns the current labels used to address fields, slots, rows, or depth items.'
    if name in {'columns', 'values', 'features', 'streams', 'slots', 'frames', 'catalogs', 'registries'}:
        return 'Materializes the current members as a collection you can inspect or reuse elsewhere.'
    if name in {'get', 'feature_ref', 'stream', 'slot', 'frame', 'catalog', 'registry', 'at', 'iat', 'iloc', 'loc', 'value', 'cases'}:
        return 'Fetches one value, member, or slice from the current object without changing the source object.'
    if name in {'head', 'tail', 'slice', 'slice_depth', 'slice_frames', 'take', 'sample', 'iloc', 'loc', 'view'}:
        return 'Returns a smaller preview cut from the current object so you can inspect rows or depth items without carrying the full payload forward.'
    if name in {'fillna', 'fillna_', 'dropna', 'dropna_all', 'ffill', 'bfill', 'isna', 'isnull', 'notna', 'notnull', 'isnan', 'isfinite'}:
        return 'This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.'
    if name in {'add', 'sub', 'subtract', 'mul', 'multiply', 'div', 'divide', 'truediv', 'floordiv', 'mod', 'pow', 'radd', 'rsub', 'rmul', 'rdiv', 'rtruediv', 'rfloordiv', 'rmod', 'rpow', 'numeric_binary'}:
        return 'This applies arithmetic across the compatible numeric payloads in the current object.'
    if name in {'eq', 'ne', 'lt', 'le', 'gt', 'ge', 'compare'}:
        return 'This produces a comparison-style result you can inspect directly or reuse as a mask.'
    if name in {'sum', 'mean', 'min', 'max', 'std', 'var', 'count', 'count_by', 'mean_by', 'aggregate', 'group_by', 'describe', 'tally', 'aggregate_over_frames', 'aggregate_over_catalogs', 'type_counts', 'slot_sizes', 'type_profiles'}:
        return 'Use this when you want a summary view instead of carrying the full input forward unchanged.'
    if name in {'cast', 'astype', 'cast_series', 'cast_note', 'cast_timeline', 'cast_timeindex'}:
        return 'This changes the semantic or numeric type so later operations route through the correct implementation path.'
    if name in {'kindelize', 'tabelize', 'to_kindle', 'from_kindle', 'to_table', 'to_thread', 'as_kindle'}:
        return 'This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.'
    if name in {'query', 'where', 'filter', 'filter_values', 'masked_select', 'select', 'drop', 'select_features'}:
        return 'Use this to reduce the current object to the rows or fields that matter for the next step.'
    if name in {'rename', 'rename_', 'rename_features', 'relabel_features', 'relabel_depth', 'set_axis', 'set_feature_axis', 'add_prefix', 'add_suffix'}:
        return 'Changes names or labels only. The underlying payload values, row order, and aligned structure stay the same.'
    if name in {'join', 'update', 'combine', 'merge', 'stitch_on', 'combine_values', 'concat', 'concat_values', 'concat_depth'}:
        return 'Use this when you need to merge compatible objects or extend the current one with another input.'
    if name in {'assign', 'transform', 'apply', 'apply_', 'apply_all', 'apply_all_', 'map', 'map_', 'map_frames', 'map_catalogs', 'apply_slot', 'set', 'set_', 'set_stream', 'set_stream_', 'set_streams', 'set_streams_', 'set_slot', 'set_slot_', 'set_slots', 'set_slots_', 'set_features', 'set_features_', 'with_cases', 'with_cases_', 'with_validity_mask', 'with_validity_mask_', 'with_meta', 'with_meta_', 'push', 'push_', 'push_all', 'push_all_', 'clear', 'clear_', 'drop_empty_slots', 'drop_empty_slots_', 'filter_values', 'filter_values_', 'transform_values', 'transform_values_', 'take_values', 'take_values_', 'concat_values', 'concat_values_'}:
        return 'Adds, replaces, filters, or rewrites part of the current object while preserving the surrounding alignment model. Methods ending with `_` perform the change in place.'
    if name in {'sql', 'prepare_sql'}:
        return 'Use this when the transformation is easier to express as SQL than as a chained sequence of table verbs.'
    if name in {'plot'}:
        return 'This opens the plotting accessor for the current data object.'
    if name in {'display', 'info', 'show'}:
        return 'This renders a human-readable view for inspection instead of creating a new analytic object.'
    if name in {'to_string', 'to_json', 'info_string'}:
        return 'Serializes or renders the current object into a text form you can inspect, log, or export.'
    if name in {'begin', 'end'}:
        return 'Exposes iterator boundaries so the object can be traversed with a range-based loop or manual iteration.'
    if name in {'text_preprocess', 'extract_keywords', 'compute_similarity', 'filter_by_text', 'add_text_features', 'aggregate_text_stats', 'tfidf', 'lower', 'replace', 'contains'}:
        return 'This is part of the text-processing surface for note-backed or text-oriented data.'
    if name in {'year', 'month', 'day', 'hour', 'to_note_weekday', 'tz_convert', 'tz_localize', 'to_utc', 'shift_time', 'floor_time', 'ceil_time', 'round_time', 'resample'}:
        return 'This is part of the time-aware surface and expects timeline or time-index semantics.'
    return f'Applies `{name}` to the current `{class_name}` object. Use the example and output together to see which part of the object changes and which structure is preserved.'


STRUCTURED_TYPES = {
    'DataFrame',
    'Thread',
    'Atlas',
    'Stream',
    'Catalog',
    'Archive',
    'Slot',
    'Registry',
    'Aisle',
    'ColumnRef',
    'ThreadValue',
}


def source_var_for(class_name: str) -> str:
    return {
        'ColumnRef': 'ref',
        'DataFrame': 'df',
        'DataFramePlotAccessor': 'plot',
        'DataFrameSqlAccessor': 'sql',
        'DataFrameSqlPrepared': 'stmt',
        'Thread': 'thread',
        'ThreadValue': 'value',
        'Atlas': 'atlas',
        'Stream': 'stream',
        'Catalog': 'ct',
        'Archive': 'archive',
        'Slot': 'slot',
        'Registry': 'rg',
        'Aisle': 'aisle',
    }[class_name]


def normalize_user_names(class_name: str, text: str) -> str:
    replacements = {
        'Catalog': {'catalog': 'ct'},
        'Registry': {'registry': 'rg'},
    }.get(class_name, {})
    for old, new in replacements.items():
        text = re.sub(rf'\b{re.escape(old)}\b', new, text)
    return text


def inspect_code_for_type(type_name: str, var_name: str) -> str:
    t = type_name.strip()
    if t == 'DataFrame':
        return f'{var_name}.head(3, true);'
    if t in {'Thread', 'const Thread&'}:
        return f'{var_name}.show();'
    if t == 'Atlas':
        return f'{var_name}.show();'
    if t == 'Stream':
        return f'{var_name}.show(std::cout);'
    if t == 'Catalog':
        return f'{var_name}.display();'
    if t == 'Archive':
        return f'{var_name}.show(std::cout);'
    if t == 'Slot':
        return f'{var_name}.show(std::cout);'
    if t == 'Registry':
        return f'{var_name}.show(std::cout);'
    if t == 'Aisle':
        return f'{var_name}.show(std::cout);'
    if t == 'ColumnRef':
        return f'{var_name}.head(3).show();'
    if t == 'ThreadValue':
        return f'{var_name}.thread().show();'
    if t.startswith('pluss::Series'):
        return f'std::cout << {var_name} << "\\n";'
    if t.startswith('pluss::Note'):
        return f'std::cout << {var_name} << "\\n";'
    if t.startswith('pluss::Timeline'):
        return f'std::cout << {var_name} << "\\n";'
    if t.startswith('pluss::TimeIndex'):
        return f'std::cout << {var_name} << "\\n";'
    return ''


INSPECTABLE_RETURN_PREFIXES = (
    'DataFrame',
    'Thread',
    'Atlas',
    'Stream',
    'Catalog',
    'Archive',
    'Slot',
    'Registry',
    'Aisle',
    'ColumnRef',
    'ThreadValue',
    'pluss::Series',
    'pluss::Note',
    'pluss::Timeline',
    'pluss::TimeIndex',
)


NO_BEFORE_AFTER = {
    'defined', 'unique', 'size', 'kind', 'dtype_name', 'nrows', 'ncols', 'nfeatures', 'nstreams',
    'nfields', 'depth', 'empty', 'count', 'count_null_rows', 'has', 'has_any', 'has_all',
    'has_stream', 'has_slot', 'has_meta', 'is_stream_compatible', 'ensure_stream_compatible',
    'keys', 'field_names', 'column_names', 'feature_names', 'values', 'streams', 'slots',
    'frames', 'catalogs', 'registries', 'axis0_labels', 'depth_labels', 'labels',
    'cases_series', 'to_string', 'to_json', 'info_string', 'begin', 'end',
}


def inject_before_after(class_name: str, method: Method, code: str) -> str:
    name = method.name
    if class_name in {'DataFramePlotAccessor', 'DataFrameSqlAccessor', 'DataFrameSqlPrepared'}:
        return code
    if name in NO_BEFORE_AFTER:
        return code
    ret = method.return_type.strip()
    source_var = source_var_for(class_name)
    source_inspect = inspect_code_for_type(class_name if class_name in STRUCTURED_TYPES else ret, source_var)
    if class_name == 'ColumnRef':
        source_inspect = 'df.head(3, true);'
    elif class_name == 'ThreadValue':
        source_inspect = 'thread.show();'

    out_inspect = ''
    if any(ret.startswith(prefix) for prefix in INSPECTABLE_RETURN_PREFIXES):
        out_inspect = inspect_code_for_type(ret, 'out')

    if '\n\nauto out =' in code:
        before = f'// Before: inspect the current state.\n{source_inspect}\n\n' if source_inspect else ''
        after = f'\n\n// After: inspect the result.\n{out_inspect}' if out_inspect else ''
        return code.replace('\n\nauto out =', f'\n\n{before}auto out =', 1) + after

    if f'\n\n{source_var}.' in code and source_inspect:
        before = f'// Before: inspect the current state.\n{source_inspect}\n\n'
        return code.replace(f'\n\n{source_var}.', f'\n\n{before}{source_var}.', 1) + f'\n\n// After: inspect the updated state.\n{source_inspect}'

    if code.startswith('auto out =') and out_inspect:
        return code + f'\n\n// Inspect the returned result.\n{out_inspect}'

    return code


DF_BASE_OUT = """id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi"""

DF_PREFIX_OUT = """feat_id  feat_sales  feat_city
1        100.0       Accra
2        120.0       Tema
3         90.0       Kumasi"""

DF_SUFFIX_OUT = """id_2024  sales_2024  city_2024
1        100.0       Accra
2        120.0       Tema
3         90.0       Kumasi"""

DF_WITH_CASES_OUT = """df.cases():
[0, 1, 2]

out.cases():
["cs0", "cs1", "cs2"]

out.head():
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi"""

DF_SET_CASES_OUT = """df.head():
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi

out.cases():
["Accra", "Tema", "Kumasi"]

out.head():
id  sales
1   100.0
2   120.0
3    90.0"""

DF_RESET_CASES_OUT = """with_cases.cases():
["Accra", "Tema", "Kumasi"]

out.cases():
[0, 1, 2]

out.head():
id  sales  case
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi"""

DF_ALIGN_CASES_OUT = """df.cases():
["cs0", "cs1", "cs2"]

out.cases():
["cs1", "cs0", "cs2"]

out.head():
id  sales  city
2   120.0  Tema
1   100.0  Accra
3    90.0  Kumasi"""

DF_PLUS10_OUT = """id  sales  city
11  110.0  Accra
12  130.0  Tema
13  100.0  Kumasi"""

DF_FILTER_OUT = """id  sales  city
1   100.0  Accra
3    90.0  Kumasi"""

DF_REORDER_OUT = """sales  id  city
100.0  1  Accra
120.0  2  Tema
90.0   3  Kumasi"""

DF_DROP_OUT = """id  sales
1   100.0
2   120.0
3    90.0"""

DF_DROPNA_OUT = """id  sales  city
1   100.0  Accra
3    90.0  Kumasi"""

DF_DROPNA_ALL_OUT = """id  sales
2   null
3   90.0"""

DF_RENAME_OUT = """id  revenue  city
1   100.0    Accra
2   120.0    Tema
3    90.0    Kumasi"""

DF_GROUP_OUT = """city    sales_sum
Accra   220.0
Tema    240.0"""

DF_BOTTOM_OUT = """id  sales  city
3    90.0  Kumasi
1   100.0  Accra"""

DF_BFILL_OUT = """score  city
71.0   Accra
71.0   Tema
88.0   Kumasi
88.0   Tamale"""

DF_COUNT_BY_OUT = """city    count
Accra   2
Tema    2"""

DF_MEAN_BY_OUT = """city    sales_mean
Accra   110.0
Tema    120.0"""

DF_PIVOT_OUT = """city   A      B
Accra  100.0  120.0
Tema    90.0  150.0"""

DF_SUMMARY_OUT = """metric       value
sales_mean    115.0"""

DF_COUNT_DISTINCT_OUT = """case     id  sales  city
summary  3   3      2"""

DF_COUNT_OUT = """feature  value
id       3
sales    3
city     3"""

DF_CORR_OUT = """feature  id      sales
id       1.0000  0.3273
sales    0.3273  1.0000"""

DF_DESCRIBE_OUT = """feature  count  mean   min   max
sales    3      103.3  90.0  120.0"""

DF_TRANSPOSE_OUT = """feature  row0   row1   row2
id       1      2      3
sales    100.0  120.0  90.0"""

THREAD_SERIES_OUT = "[10.0, 20.0, 30.0]"
THREAD_NOTE_OUT = '["Accra", "Tema", "Kumasi"]'
THREAD_TIME_OUT = '["2024-01-01T09:10:00Z", "2024-02-01T11:00:00Z", "2024-03-05T15:30:00Z"]'
THREAD_BFILL_OUT = "[10.0, 30.0, 30.0, null]"
THREAD_BTW_NUMERIC_OUT = "[false, true, true, false]"
THREAD_BTW_TIME_OUT = "[true, true, false]"
THREAD_BTW_ROWWISE_OUT = "[true, true, false]"
THREAD_BUCKETIZE_OUT = "[0, 0, 1, 2, null]"
THREAD_COUNT_OUT = "[3]"
THREAD_CONTAINS_OUT = "[true, false, false]"
THREAD_COMPARE_SCALAR_OUT = "[false, true, true]"
THREAD_CORR_OUT = "0.9819805"
THREAD_DISTINCT_OUT = '["Accra", "Tema"]'
THREAD_DISPLAY_LABELS_OUT = '["a", "b", "c"]'
THREAD_DISPLAY_NAME_OUT = 'sales'
THREAD_DESCRIBE_NUMERIC_OUT = """count  3
mean   20.0
std    10.0
min    10.0
25%    15.0
50%    20.0
75%    25.0
max    30.0"""
THREAD_DESCRIBE_NOTE_OUT = """count   3
unique  2
top     Accra
freq    2"""
STREAM_SERIES_OUT = "Series([10.0, 20.0, 30.0])"
STREAM_NOTE_OUT = 'Note(["Accra", "Tema", "Kumasi"])'
STREAM_BFILL_OUT = 'Series([10.0, 30.0, 30.0, null])'
CATALOG_BASE_OUT = """id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])"""
CATALOG_BFILL_OUT = """ct.head():
Final Points Grade -> Series([71.0, null, 88.0, null])
Midterm 3          -> Series([60.0, null, 75.0, 91.0])
Name               -> Note(["A", "B", "C", "D"])

out.head():
Final Points Grade -> Series([71.0, 88.0, 88.0, null])
Midterm 3          -> Series([60.0, 75.0, 75.0, 91.0])
Name               -> Note(["A", "B", "C", "D"])"""
CATALOG_TEXT_FEATURES_OUT = """catalog.head():
text -> Note(["heavy rain on road", "clear weather downtown", "storm warning issued"])

out.head():
text            -> Note(["heavy rain on road", "clear weather downtown", "storm warning issued"])
text_char_len   -> Series([18, 22, 20])
text_word_count -> Series([4, 3, 3])"""
CATALOG_TEXT_STATS_OUT = """text_mean_char_len -> Series([20.0])"""
CATALOG_SIMILARITY_OUT = """text__text_similarity -> Series([1.0, 1.0, 1.0])"""
CATALOG_KEYWORDS_OUT = """keyword -> Note(["clear", "downtown", "heavy", "issued", "on"])
count   -> Series([1, 1, 1, 1, 1])"""
CATALOG_DROP_OUT = """ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])"""
CATALOG_DROPNA_OUT = """ct.head():
metric -> Series([10.0, null, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
metric -> Series([10.0, 30.0])
city   -> Note(["Accra", "Kumasi"])"""
REGISTRY_CLEAR_OUT = """rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
<empty registry>"""
REGISTRY_COMBINE_OUT = """rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids      -> Slot([Series(1), Series(2), Series(3)])
city     -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
flag_rhs -> Slot([Series(true), Series(false), Series(true)])"""
REGISTRY_CONCAT_OUT = """rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3), Series(4), Series(5)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])"""
REGISTRY_DROP_OUT = """rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])"""
REGISTRY_DROP_EMPTY_OUT = """rg.head():
ids    -> Slot([Series(1), Series(2), Series(3)])
empty  -> Slot([])
city   -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])"""
AISLE_CONCAT_OUT = """depth0 -> Registry(ids -> Slot([Series(1), Series(2)]))
depth1 -> Registry(ids -> Slot([Series(3), Series(4)]))
depth2 -> Registry(ids -> Slot([Series(1), Series(2)]))
depth3 -> Registry(ids -> Slot([Series(3), Series(4)]))"""
REGISTRY_BASE_OUT = """ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])"""
ATLAS_BASE_OUT = """frame0:
sales
10.0
20.0

frame1:
sales
30.0
40.0"""
ARCHIVE_BASE_OUT = """train -> Catalog(metric -> Series([10.0, 20.0]))
test  -> Catalog(metric -> Series([30.0, 40.0]))"""
ARCHIVE_DLOC_OUT = """metric -> Series([30.0, 40.0])"""
AISLE_BASE_OUT = """depth0 -> Registry(ids -> Slot([Series(1), Series(2)]))
depth1 -> Registry(ids -> Slot([Series(3), Series(4)]))"""
SLOT_BASE_OUT = "Slot([Series(1), Series(2), Series(3)])"


def labeled_pair(source_label: str, source_body: str, out_body: str, out_label: str = 'out.head()') -> str:
    return f"{source_label}:\n{source_body}\n\n{out_label}:\n{out_body}"


def output_for(class_name: str, method: Method) -> str:
    n = method.name
    ret = method.return_type.strip()

    if class_name == 'DataFrame':
        if n in {'defined', 'unique'}:
            return 'true'
        if n in {'nrows', 'ncols', 'nfeatures', 'count_null_rows'}:
            return '3'
        if n in {'column_names', 'feature_names'}:
            return '["id", "sales", "city"]'
        if n == 'cases_series':
            return '[0, 1, 2]'
        if n in {'columns', 'values', 'features'} and 'string_view' not in method.signature and 'initializer_list' not in method.signature:
            return '[Thread(id), Thread(sales), Thread(city)]'
        if n in {'at', 'iat', 'cases'} and ('feature_name' in method.signature or 'feature_idx' in method.signature):
            return 'Accra' if 'feature_name' in method.signature else '1'
        if n in {'get', 'column', 'features'} and 'string_view' in method.signature:
            return 'Thread([100.0, 120.0, 90.0])'
        if n in {'has_feature', 'equals'}:
            return 'true'
        if n in {'with_cases'}:
            return DF_WITH_CASES_OUT
        if n in {'with_cases_'}:
            return """df.cases():
["cs0", "cs1", "cs2"]

df.head():
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi"""
        if n in {'add_prefix'}:
            return f"df.head():\n{DF_BASE_OUT}\n\nout.head():\n{DF_PREFIX_OUT}"
        if n in {'add_suffix'}:
            return f"df.head():\n{DF_BASE_OUT}\n\nout.head():\n{DF_SUFFIX_OUT}"
        if n in {'rename', 'rename_', 'rename_features', 'relabel_features', 'set_axis', 'set_feature_axis'}:
            if n == 'rename_features':
                return """id  revenue  location
1   100.0    Accra
2   120.0    Tema
3    90.0    Kumasi"""
            if n in {'relabel_features', 'set_axis', 'set_feature_axis'}:
                return """revenue  location  id_new
100.0    Accra     1
120.0    Tema      2
90.0     Kumasi    3"""
            return DF_RENAME_OUT
        if n in {'reorder'}:
            return DF_REORDER_OUT
        if n in {'drop'}:
            return DF_DROP_OUT
        if n == 'dropna':
            return DF_DROPNA_OUT
        if n == 'dropna_all':
            return DF_DROPNA_ALL_OUT
        if n in {'insert', 'set_features', 'set_features_', 'assign'}:
            return """id  sales  city    profit
1   100.0  Accra   40.0
2   120.0  Tema    35.0
3    90.0  Kumasi  30.0"""
        if n == 'bottom':
            return DF_BOTTOM_OUT
        if n in {'head', 'tail', 'slice', 'take', 'sample', 'where', 'filter', 'query', 'top', 'unique_cases'}:
            return DF_FILTER_OUT if n in {'where', 'filter', 'query', 'take'} else DF_BASE_OUT
        if n in {'set_cases'}:
            return DF_SET_CASES_OUT
        if n in {'reset_cases'}:
            return DF_RESET_CASES_OUT
        if n in {'align_cases'}:
            return DF_ALIGN_CASES_OUT
        if n in {'sort_values', 'sort_cases', 'reindex_like', 'reindex_cases', 'conform_to'}:
            return DF_BASE_OUT
        if n in {'join', 'update', 'combine', 'stitch_on'}:
            return """id  sales  cost
1   100.0  60.0
2   120.0  80.0
3    90.0  55.0"""
        if n == 'unify':
            return """OrgDefinedId  midterm  Final Points Grade
10            71       null
20            82       88
30            93       99"""
        if n in {'group_by'}:
            return DF_GROUP_OUT
        if n in {'count_by', 'tally'}:
            return DF_COUNT_BY_OUT
        if n == 'count_distinct':
            return DF_COUNT_DISTINCT_OUT
        if n in {'mean_by'}:
            return DF_MEAN_BY_OUT
        if n in {'pivot', 'pivot_table'}:
            return DF_PIVOT_OUT
        if n in {'aggregate'}:
            return DF_SUMMARY_OUT
        if n == 'corr':
            return DF_CORR_OUT
        if n in {'describe'}:
            return DF_DESCRIBE_OUT
        if n in {'transpose'}:
            return DF_TRANSPOSE_OUT
        if n == 'count':
            return DF_COUNT_OUT
        if n in {'sum', 'mean', 'min', 'max', 'std', 'var'}:
            return """feature  value
sales    310.0"""
        if n == 'bfill':
            return DF_BFILL_OUT
        if n in {'fillna', 'fillna_', 'ffill', 'isna', 'isnull', 'notna', 'notnull'}:
            return DF_BASE_OUT
        if n in {'cast', 'astype', 'cast_series', 'cast_note', 'cast_timeline', 'cast_timeindex'}:
            return DF_BASE_OUT
        if n in {'resample'}:
            return """ts                   load
2024-01-01T09:00:00Z  15.0
2024-01-01T10:00:00Z  15.0"""
        if n in {'tz_convert', 'tz_localize', 'to_utc', 'shift_time', 'floor_time', 'ceil_time', 'round_time'}:
            return """ts
2024-01-01T09:00:00Z
2024-01-01T10:00:00Z"""
        if n in {'add', 'sub', 'subtract', 'mul', 'multiply', 'div', 'divide', 'truediv', 'floordiv', 'mod', 'pow', 'radd', 'rsub', 'rmul', 'rdiv', 'rtruediv', 'rfloordiv', 'rmod', 'rpow'}:
            return DF_PLUS10_OUT if 'const DataFrame&' not in method.signature else DF_BASE_OUT
        if n in {'eq', 'ne', 'lt', 'le', 'gt', 'ge', 'compare'}:
            return """id  sales
1   true
2   true
3   false"""
        if n in {'copy', 'view'}:
            return DF_BASE_OUT
        if n in {'to_string', 'info_string'}:
            return DF_BASE_OUT
        if n in {'info'}:
            return "rows: 3\nfeatures: 3\nnumeric: id, sales\nnote: city"
        if n.startswith('save_') or n == 'save_as':
            return 'saved: out.*'
        if n == 'plot':
            return 'Figure(line/scatter/bar/hist)'
        if n in {'sql', 'prepare_sql'}:
            return 'SQL accessor / prepared statement created'
        if n in {'unsqueeze'}:
            return ATLAS_BASE_OUT
        return DF_BASE_OUT if ret.startswith('DataFrame') else ''

    if class_name == 'Thread':
        if n in {'defined', 'unique', 'is_valid'}:
            return 'true'
        if n == 'size':
            return '3'
        if n == 'kind':
            return 'Series'
        if n == 'dtype_name':
            return 'Float64'
        if n in {'value'}:
            return '10.0'
        if n in {'values', 'series', 'as_series'}:
            return THREAD_SERIES_OUT
        if n in {'note', 'as_note'}:
            return THREAD_NOTE_OUT
        if n in {'timeline', 'as_timeline', 'timeindex', 'as_timeindex'}:
            return THREAD_TIME_OUT
        if n in {'isnull', 'notnull', 'isnan', 'isfinite'}:
            return '[true, false, true]'
        if n == 'contains':
            return THREAD_CONTAINS_OUT
        if n == 'distinct':
            return THREAD_DISTINCT_OUT
        if n == 'display_labels':
            return THREAD_DISPLAY_LABELS_OUT
        if n == 'display_name':
            return THREAD_DISPLAY_NAME_OUT
        if n == 'describe':
            return THREAD_DESCRIBE_NUMERIC_OUT
        if n == 'btw':
            if 'double a, double b' in method.signature:
                return THREAD_BTW_NUMERIC_OUT
            if 'std::string_view lo, std::string_view hi' in method.signature:
                return THREAD_BTW_TIME_OUT
            if 'const Thread& lo, const Thread& hi' in method.signature:
                return THREAD_BTW_ROWWISE_OUT
        if n == 'bucketize':
            return THREAD_BUCKETIZE_OUT
        if n == 'strip':
            return '["Accra", "Tema", "Kumasi"]'
        if n == 'lstrip':
            return '["203930", "7788", "4455##"]' if 'std::string_view chars' in method.signature else '["Accra", "Tema", "Kumasi"]'
        if n == 'rstrip':
            return '["#203930", "##7788", "4455"]' if 'std::string_view chars' in method.signature else '["Accra", "Tema", "Kumasi"]'
        if n == 'bfill':
            return THREAD_BFILL_OUT
        if n == 'dropna':
            return '[10.0, 30.0]'
        if n in {'abs', 'clip', 'log', 'exp', 'fillna', 'fillna_', 'head', 'tail', 'slice', 'take', 'filter', 'ffill', 'lower', 'replace', 'window', 'cast', 'astype'}:
            return THREAD_SERIES_OUT if 'Note' not in method.signature and n not in {'lower', 'replace'} else THREAD_NOTE_OUT
        if n == 'count':
            return THREAD_COUNT_OUT
        if n == 'corr':
            return THREAD_CORR_OUT
        if n in {'sum', 'mean', 'min', 'max', 'var', 'std'}:
            return '20.0'
        if n in {'year', 'month', 'day', 'hour', 'to_note_weekday'}:
            return '[2024, 2024, 2024]' if n == 'year' else '[1, 2, 3]'
        if n in {'numeric_binary', 'compare', 'compare_rows'}:
            if n == 'numeric_binary':
                return '[20.0, 30.0, 40.0]'
            if n == 'compare':
                return THREAD_COMPARE_SCALAR_OUT if 'const Thread&' not in method.signature else '[false, true, false]'
            return '1'
        if n in {'to_string'}:
            return THREAD_SERIES_OUT
        if n == 'show':
            return THREAD_SERIES_OUT
        return ''

    if class_name == 'ThreadValue':
        if n in {'defined'}:
            return 'true'
        if n in {'kind'}:
            return 'Series'
        if n in {'thread'}:
            return '[10.0]'
        if n in {'as_series', 'as_note', 'as_timeline', 'as_timeindex'}:
            return '[10.0]' if 'series' in n else '["Accra"]' if 'note' in n else '["2024-01-01T09:10:00Z"]'
        return ''

    if class_name == 'ColumnRef':
        if n == 'name':
            return 'sales'
        if n == 'thread' or n == 'values':
            return THREAD_SERIES_OUT
        if n == 'mean':
            return '103.3'
        if n == 'head':
            return '[100.0, 120.0]'
        if n == 'value':
            return '100.0'
        return ''

    if class_name == 'DataFramePlotAccessor':
        if n == 'act':
            return 'Figure(act)'
        if n == 'andrews_curves':
            return 'Figure(andrews_curves)'
        if n == 'area':
            return 'Figure(area)'
        if n == 'area_stacked':
            return 'Figure(area_stacked)'
        if n == 'auto_plot':
            return 'Figure(auto_selected)'
        if n == 'bar':
            return 'Figure(bar)'
        if n == 'bar_stacked':
            return 'Figure(bar_stacked)'
        if n == 'density':
            return 'Figure(density)'
        if n == 'box':
            return 'Figure(box)'
        if n == 'bootstrap':
            return 'Figure(bootstrap)'
        if n == 'brief':
            return 'Figure(summary)'
        if n == 'corr_heatmap':
            return 'Figure(corr_heatmap)'
        return 'Figure(...)'

    if class_name == 'DataFrameSqlAccessor':
        return DF_FILTER_OUT if n == 'execute' else 'Prepared statement ready'

    if class_name == 'DataFrameSqlPrepared':
        return DF_FILTER_OUT if n == 'execute' else ''

    if class_name == 'Atlas':
        if n in {'defined', 'unique'}:
            return 'true'
        if n in {'empty'}:
            return 'false'
        if n in {'depth'}:
            return '2'
        if n in {'nrows', 'ncols'}:
            return '2'
        if n in {'column_names'}:
            return '["sales"]'
        if n in {'axis0_labels'}:
            return '["train", "test"]'
        if n in {'frames'}:
            return '[frame0, frame1]'
        if n in {'frame', 'iloc', 'loc', 'squeeze', 'head', 'tail', 'slice_frames', 'sort_depth', 'relabel_depth', 'align_depth', 'map_frames', 'aggregate_over_frames'}:
            return ATLAS_BASE_OUT if not ret.startswith('DataFrame') else "sales\n10.0\n20.0"
        if n == 'dloc':
            return "sales\n30.0\n40.0"
        if n in {'tabelize', 'kindelize'}:
            return ARCHIVE_BASE_OUT if n == 'kindelize' else ATLAS_BASE_OUT
        if n in {'to_string', 'show'}:
            return ATLAS_BASE_OUT
        if n == 'plot':
            return 'Figure(line)'
        return ''

    if class_name == 'Stream':
        if n in {'defined', 'unique'}:
            return 'true'
        if n == 'size':
            return '3'
        if n == 'kind':
            return 'Series'
        if n == 'dtype_name':
            return 'Float64'
        if n in {'has_validity_mask', 'has_cases', 'has_meta'}:
            return 'true'
        if n in {'validity_mask'}:
            return '[true, false, true]'
        if n in {'cases'}:
            return '["cs0", "cs1", "cs2"]'
        if n in {'meta'}:
            return 'km'
        if n in {'isnull', 'notnull'}:
            return STREAM_SERIES_OUT
        if n == 'bfill':
            return STREAM_BFILL_OUT
        if n == 'dropna':
            return 'Series([10.0, 30.0])'
        if n in {'fillna', 'abs', 'clip', 'log', 'exp', 'with_validity_mask', 'clear_validity_mask', 'with_cases', 'clear_cases', 'with_meta', 'drop_meta', 'head', 'tail', 'slice', 'take', 'filter', 'ffill', 'cast', 'astype'}:
            return STREAM_SERIES_OUT if 'Note' not in method.signature and n not in {'fillna'} else STREAM_NOTE_OUT
        if n in {'sum', 'mean', 'min', 'max', 'var', 'std'}:
            return 'Series([20.0])'
        if n in {'as_series'}:
            return STREAM_SERIES_OUT
        if n in {'as_note'}:
            return STREAM_NOTE_OUT
        if n in {'as_timeline', 'as_timeindex'}:
            return 'Timeline([...])'
        if n in {'to_thread'}:
            return THREAD_SERIES_OUT
        if n in {'to_string', 'to_json', 'show'}:
            return STREAM_SERIES_OUT
        if n == 'from_thread':
            return STREAM_SERIES_OUT
        return ''

    if class_name == 'Catalog':
        if n in {'defined', 'unique'}:
            return 'true'
        if n in {'nstreams', 'nfields', 'count'}:
            return '3'
        if n == 'empty':
            return 'false'
        if n == 'keys':
            return '["id", "metric", "city"]'
        if n in {'streams'}:
            return '[Stream(id), Stream(metric), Stream(city)]'
        if n in {'has', 'has_stream'}:
            return 'true'
        if n in {'stream', 'at', 'get', 'get_stream'}:
            return 'Series([10.0, 20.0, 30.0])'
        if n == 'bfill':
            return CATALOG_BFILL_OUT
        if n == 'drop':
            return CATALOG_DROP_OUT
        if n == 'dropna':
            return CATALOG_DROPNA_OUT
        if n == 'extract_keywords':
            return CATALOG_KEYWORDS_OUT
        if n == 'add_text_features':
            return CATALOG_TEXT_FEATURES_OUT
        if n == 'aggregate_text_stats':
            return CATALOG_TEXT_STATS_OUT
        if n == 'compute_similarity':
            return CATALOG_SIMILARITY_OUT
        if n in {'series', 'note', 'timeline', 'timeindex', 'head', 'tail', 'slice', 'iloc', 'take', 'sample', 'filter', 'where', 'query', 'masked_select', 'select', 'rename', 'sort', 'sort_by', 'fillna', 'ffill', 'isnull', 'notnull', 'cast_series', 'cast_note', 'cast_timeline', 'cast_timeindex', 'astype', 'merge', 'apply', 'apply_all', 'transform', 'text_preprocess', 'filter_by_text', 'tfidf', 'set_stream', 'set', 'set_streams'}:
            return labeled_pair('ct.head()', CATALOG_BASE_OUT, CATALOG_BASE_OUT)
        if n in {'sum', 'mean', 'min', 'max'}:
            return 'Series([20.0])'
        if n in {'group_by'}:
            return '{"Accra": Catalog(...), "Tema": Catalog(...)}'
        if n in {'tabelize', 'to_table'}:
            return DF_BASE_OUT
        if n.startswith('save_') or n == 'save_as':
            return 'saved: out.*'
        if n in {'to_string', 'to_json', 'display', 'info'}:
            return f"ct.head():\n{CATALOG_BASE_OUT}"
        if n in {'from_json', 'kindelize'} or n.startswith('load'):
            return f"out.head():\n{CATALOG_BASE_OUT}"
        return ''

    if class_name == 'Archive':
        if n in {'defined', 'unique'}:
            return 'true'
        if n == 'depth':
            return '2'
        if n == 'empty':
            return 'false'
        if n in {'depth_labels'}:
            return '["train", "test"]'
        if n in {'catalogs'}:
            return '[Catalog(train), Catalog(test)]'
        if n in {'catalog', 'iloc', 'loc', 'head', 'tail', 'slice_depth', 'sort_depth', 'relabel_depth', 'align_depth', 'map_catalogs'}:
            return ARCHIVE_BASE_OUT if ret.startswith('Archive') else CATALOG_BASE_OUT
        if n == 'dloc':
            return ARCHIVE_DLOC_OUT
        if n in {'aggregate_over_catalogs'}:
            return CATALOG_BASE_OUT
        if n in {'tabelize', 'to_table'}:
            return ATLAS_BASE_OUT
        if n in {'to_string', 'show', 'kindelize'}:
            return ARCHIVE_BASE_OUT
        return ''

    if class_name == 'Slot':
        if n in {'defined', 'unique'}:
            return 'true'
        if n == 'size':
            return '3'
        if n == 'empty':
            return 'false'
        if n == 'values':
            return SLOT_BASE_OUT
        if n == 'at':
            return 'Series(1)'
        if n in {'push', 'push_', 'head', 'tail', 'slice', 'take', 'filter', 'transform', 'concat'}:
            return SLOT_BASE_OUT
        if n == 'type_counts':
            return '{"Series": 2, "Note": 1}'
        if n in {'to_kindle', 'as_kindle', 'kindelize'}:
            return STREAM_SERIES_OUT
        if n in {'from_kindle'}:
            return SLOT_BASE_OUT
        if n in {'to_string', 'to_json', 'show'}:
            return SLOT_BASE_OUT
        return ''

    if class_name == 'Registry':
        if n in {'defined', 'unique'}:
            return 'true'
        if n in {'nslots', 'nfields'}:
            return '2'
        if n == 'empty':
            return 'false'
        if n in {'keys', 'field_names'}:
            return '["ids", "city"]'
        if n in {'slots'}:
            return '[Slot(ids), Slot(city)]'
        if n in {'has', 'has_slot', 'has_all', 'has_any', 'is_stream_compatible'}:
            return 'true'
        if n in {'slot', 'at', 'get', 'get_slot'}:
            return SLOT_BASE_OUT
        if n == 'clear':
            return REGISTRY_CLEAR_OUT
        if n == 'combine':
            return REGISTRY_COMBINE_OUT
        if n == 'concat_values':
            return REGISTRY_CONCAT_OUT
        if n == 'drop':
            return REGISTRY_DROP_OUT
        if n == 'drop_empty_slots':
            return REGISTRY_DROP_EMPTY_OUT
        if n in {'head', 'tail', 'slice', 'iloc', 'take', 'sample', 'select', 'rename', 'apply', 'apply_all', 'map', 'clear', 'set_slot', 'set_slots', 'push', 'push_all', 'filter_values', 'transform_values', 'take_values', 'concat_values'}:
            return labeled_pair('rg.head()', REGISTRY_BASE_OUT, REGISTRY_BASE_OUT)
        if n in {'slot_sizes'}:
            return '{"ids": 3, "city": 3}'
        if n in {'type_profiles'}:
            return '{"ids": {"Series": 3}, "city": {"Note": 3}}'
        if n in {'to_kindle', 'as_kindle', 'kindelize'}:
            return CATALOG_BASE_OUT
        if n in {'from_kindle'} or n.startswith('load'):
            return f"out.head():\n{REGISTRY_BASE_OUT}"
        if n.startswith('save_') or n == 'save_as':
            return 'saved: out.*'
        if n in {'to_string', 'to_json', 'show'}:
            return f"rg.head():\n{REGISTRY_BASE_OUT}"
        if n == 'ensure_stream_compatible':
            return 'no error'
        return ''

    if class_name == 'Aisle':
        if n in {'defined', 'unique'}:
            return 'true'
        if n == 'depth':
            return '2'
        if n == 'empty':
            return 'false'
        if n in {'registries'}:
            return '[Registry(depth0), Registry(depth1)]'
        if n in {'labels'}:
            return '["train", "test"]'
        if n == 'concat_depth':
            return AISLE_CONCAT_OUT
        if n in {'registry', 'head', 'tail', 'slice', 'map', 'select', 'apply_slot'}:
            return AISLE_BASE_OUT if ret.startswith('Aisle') else REGISTRY_BASE_OUT
        if n in {'to_kindle', 'as_kindle', 'kindelize'}:
            return ARCHIVE_BASE_OUT
        if n in {'from_kindle'}:
            return AISLE_BASE_OUT
        if n in {'to_string', 'to_json', 'show'}:
            return AISLE_BASE_OUT
        return ''

    return ''


def df_base() -> str:
    return """auto df = pluss::table::DataFrame({
    {\"id\", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {\"sales\", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {\"city\", pluss::table::Thread(pluss::make_note({\"Accra\", \"Tema\", \"Kumasi\"}))},
});"""


def df_time() -> str:
    return """auto df = pluss::table::DataFrame({
    {\"ts\", pluss::table::Thread(pluss::make_timeline({
        \"2024-01-01T09:10:00Z\",\n        \"2024-01-01T09:40:00Z\",\n        \"2024-01-01T10:05:00Z\",\n    }))},
    {\"load\", pluss::table::Thread(pluss::make_series({10.0, 20.0, 15.0}))},
});"""


def df_two() -> str:
    return """auto left = pluss::table::DataFrame({
    {\"id\", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {\"sales\", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {\"id\", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {\"cost\", pluss::table::Thread(pluss::make_series({60.0, 80.0, 55.0}))},
});"""


def df_group() -> str:
    return """auto df = pluss::table::DataFrame({
    {\"city\", pluss::table::Thread(pluss::make_note({\"Accra\", \"Accra\", \"Tema\", \"Tema\"}))},
    {\"product\", pluss::table::Thread(pluss::make_note({\"A\", \"B\", \"A\", \"B\"}))},
    {\"sales\", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0, 150.0}))},
});"""


def thread_series() -> str:
    return 'auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));'


def thread_note() -> str:
    return 'auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));'


def thread_time() -> str:
    return 'auto thread = pluss::table::Thread(pluss::make_timeline({"2024-01-01T09:10:00Z", "2024-02-01T11:00:00Z", "2024-03-05T15:30:00Z"}));'


def atlas_base() -> str:
    return """auto f0 = pluss::table::DataFrame({
    {\"sales\", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {\"sales\", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});"""


def stream_series() -> str:
    return 'auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));'


def stream_note() -> str:
    return 'auto stream = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"}));'


def stream_time() -> str:
    return 'auto stream = pluss::kindle::Stream(pluss::make_timeline({"2024-01-01T09:10:00Z", "2024-01-01T09:40:00Z", "2024-01-01T10:05:00Z"}));'


def catalog_base() -> str:
    return """using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    \"id\"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    \"metric\"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    \"city\"_k = pluss::kindle::Stream(pluss::make_note({\"Accra\", \"Tema\", \"Kumasi\"})),
});"""


def catalog_text() -> str:
    return """using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    \"text\"_k = pluss::kindle::Stream(pluss::make_note({
        \"heavy rain on road\",\n        \"clear weather downtown\",\n        \"storm warning issued\",\n    })),
});"""


def archive_base() -> str:
    return """using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    \"metric\"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    \"metric\"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({\"train\", \"test\"})));"""


def slot_base() -> str:
    return 'auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});'


def slot_mixed() -> str:
    return 'auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_note("ok"), pluss::make_series(3)});'


def registry_base() -> str:
    return """auto rg = pluss::bundle::Registry({
    {\"ids\", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {\"city\", pluss::bundle::Slot({pluss::make_note(\"Accra\"), pluss::make_note(\"Tema\"), pluss::make_note(\"Kumasi\")})},
});"""


def aisle_base() -> str:
    return """auto r0 = pluss::bundle::Registry({
    {\"ids\", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {\"ids\", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto aisle = pluss::bundle::Aisle({r0, r1});"""


def feature_ref_base() -> str:
    return df_base() + '\n' + 'auto ref = df.feature_ref("sales");'


def sql_accessor_base() -> str:
    return df_base() + '\n' + 'auto sql = df.sql();'


def sql_prepared_base() -> str:
    return df_base() + '\n' + 'auto stmt = df.prepare_sql("SELECT id, sales FROM self WHERE sales > {min_sales}");'


def plot_accessor_base() -> str:
    return df_base() + '\n' + 'auto plot = df.plot();'


def thread_value_base() -> str:
    return 'auto thread = pluss::table::Thread(pluss::make_series({10.0}));\nauto value = thread.value();'


def thread_value_note_base() -> str:
    return 'auto thread = pluss::table::Thread(pluss::make_note({"Accra"}));\nauto value = thread.value();'


def thread_value_timeline_base() -> str:
    return 'auto thread = pluss::table::Thread(pluss::make_timeline({"2024-01-01T09:10:00Z"}));\nauto value = thread.value();'


def thread_value_timeindex_base() -> str:
    return 'auto thread = pluss::table::Thread(pluss::make_timeindex({"2024-01-01T09:10:00Z"}));\nauto value = thread.value();'


def call_line(obj: str, method: str, args: str, assign: bool = True) -> str:
    if assign:
        return f'auto out = {obj}.{method}({args});' if args else f'auto out = {obj}.{method}();'
    return f'{obj}.{method}({args});' if args else f'{obj}.{method}();'


def example_for(class_name: str, method: Method) -> str:
    n = method.name
    sig = method.signature
    # DataFrame
    if class_name == 'DataFrame':
        if n.startswith('load'):
            if 'nullptr_t' in sig:
                return f'auto out = pluss::table::DataFrame::load_excel("{GENERIC_XLSX_PATH}", nullptr, true);'
            if 'sheet_name' in sig:
                return f'auto out = pluss::table::DataFrame::load_excel("{GENERIC_XLSX_PATH}", "Sheet1", true);'
            if 'sheet_index' in sig or 'sheet_index,' in sig:
                return f'auto out = pluss::table::DataFrame::load_excel("{GENERIC_XLSX_PATH}", 0, true);'
            if 'ReadExcelOptions' in sig:
                return f'pluss::loader::ReadExcelOptions options;\nauto out = pluss::table::DataFrame::load_excel("{GENERIC_XLSX_PATH}", options);'
            if 'ReadCsvOptions' in sig:
                return f'pluss::loader::ReadCsvOptions options;\nauto out = pluss::table::DataFrame::load_csv("{GENERIC_CSV_PATH}", options);'
            if 'Config' in sig or 'ConfigEntry' in sig:
                return f'auto out = pluss::table::DataFrame::load_csv("{GENERIC_CSV_PATH}", {{"nrows", 16}});'
            if 'char sep' in sig and 'usecols' in sig:
                return f'auto out = pluss::table::DataFrame::load_csv("{GENERIC_CSV_PATH}", \',\', 16, std::vector<std::string>{{"Agency Name"}}, std::nullopt);'
            if 'source_type' in sig:
                return f'auto out = pluss::table::DataFrame::load("csv", "{GENERIC_CSV_PATH}");'
            path = GENERIC_CSV_PATH
            if 'excel' in n:
                path = GENERIC_XLSX_PATH
            elif 'json' in n:
                path = GENERIC_JSON_PATH
            elif 'parquet' in n:
                path = GENERIC_PARQUET_PATH
            elif 'txt' in n:
                path = GENERIC_TXT_PATH
            return f'auto out = pluss::table::DataFrame::{n}("{path}");'
        if n == 'tabelize':
            return 'using namespace pluss::kindle::literals;\nauto catalog = pluss::kindle::Catalog({"sales"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}))});\nauto out = pluss::table::DataFrame::tabelize(catalog);'
        if n == 'kindelize':
            return df_base() + '\n\nauto out = df.kindelize();'
        if n in {'nrows','ncols','nfeatures','values','feature_names','cases_series'}:
            return df_base() + f'\n\nauto out = df.{n}();'
        if n == 'cases':
            if 'pair<' in sig:
                return df_base() + '\n\nauto out = df.cases({0, 2});'
            if 'feature_idx' in sig:
                return df_base() + '\n\nauto out = df.cases(0, 0);'
            if 'feature_name' in sig:
                return df_base() + '\n\nauto out = df.cases(0, "city");'
            if 'row' in sig:
                return df_base() + '\n\nauto out = df.cases(0);'
            return df_base() + '\n\nauto out = df.cases();'
        if n in {'iloc','iat','at','get','has_feature','feature_ref'}:
            args = {
                'iloc': '1' if 'start' not in sig else '0, 2',
                'iat': '0, 0', 'at': '0, "city"', 'get': '"sales"', 'has_feature': '"sales"',
                'feature_ref': '"sales"'
            }[n]
            return df_base() + f'\n\nauto out = df.{n}({args});'
        if n == 'features':
            if 'initializer_list' in sig:
                return df_base() + '\n\nauto out = df.features({"id", "sales"});'
            if 'string_view' in sig:
                return df_base() + '\n\nauto out = df.features("sales");'
            return df_base() + '\n\nauto out = df.features();'
        if n in {'series_features','note_features','timeline_features','with_cases','with_cases_','cast_series','cast_note','cast_timeline','cast_timeindex'}:
            if n == 'with_cases':
                return df_base() + '\n\nauto cases = pluss::table::Thread(pluss::make_note({"cs0", "cs1", "cs2"}));\nauto out = df.with_cases(cases);'
            if n == 'with_cases_':
                return df_base() + '\n\nauto cases = pluss::table::Thread(pluss::make_note({"cs0", "cs1", "cs2"}));\ndf.with_cases_(cases);'
            if n == 'cast_series':
                return 'auto df = pluss::table::DataFrame({{"sales", pluss::table::Thread(pluss::make_note({"100.0", "120.0", "90.0"}))}});\n\nauto out = df.cast_series({"sales"}, pluss::codem::ScalarType::Float64);'
            if n == 'cast_timeline':
                return 'auto df = pluss::table::DataFrame({{"ts", pluss::table::Thread(pluss::make_note({"2024-01-01T09:10:00Z", "2024-01-01T09:40:00Z"}))}});\n\nauto out = df.cast_timeline({"ts"});'
            return df_base() + f'\n\nauto out = df.{n}();' if n.endswith('_features') else df_base() + f'\n\nauto out = df.{n}({{"sales"}});'
        if n in {'astype','cast'}:
            if 'feature_dtypes' in sig:
                return 'auto df = pluss::table::DataFrame({{"sales", pluss::table::Thread(pluss::make_note({"100.0", "120.0", "90.0"}))}});\n\nauto out = df.astype({{"sales", pluss::codem::ScalarType::Float64}});'
            if 'CastSpec' in sig:
                return 'auto df = pluss::table::DataFrame({{"sales", pluss::table::Thread(pluss::make_note({"100.0", "120.0", "90.0"}))}});\n\nauto out = df.cast({{.feature = "sales", .target = pluss::table::ThreadCastTarget::Series, .series_dtype = pluss::codem::ScalarType::Float64}});'
            if n == 'astype':
                return 'auto df = pluss::table::DataFrame({{"sales", pluss::table::Thread(pluss::make_note({"100.0", "120.0", "90.0"}))}});\n\nauto out = df.astype({"sales"}, pluss::codem::ScalarType::Float64);'
        if n in {'insert','drop','reorder','add_prefix','add_suffix','rename','rename_','rename_features','relabel_features','set_axis','set_feature_axis'}:
            if n == 'insert':
                return df_base() + '\n\nauto out = df.insert(1, "profit", pluss::table::Thread(pluss::make_series({40.0, 35.0, 30.0})));'
            if n == 'drop':
                return df_base() + '\n\nauto out = df.drop({"city"});'
            if n == 'reorder':
                return df_base() + '\n\nauto out = df.reorder({"sales", "id", "city"});'
            if n == 'add_prefix':
                return df_base() + '\n\nauto out = df.add_prefix("feat_");'
            if n == 'add_suffix':
                return df_base() + '\n\nauto out = df.add_suffix("_2024");'
            if n == 'rename':
                return df_base() + '\n\nauto out = df.rename("sales", "revenue");'
            if n == 'rename_':
                return df_base() + '\n\ndf.rename_("sales", "revenue");'
            if n == 'rename_features':
                return df_base() + '\n\nauto out = df.rename_features({{"sales", "revenue"}, {"city", "location"}});'
            if n == 'relabel_features' or n == 'set_axis' or n == 'set_feature_axis':
                return df_base() + f'\n\nauto out = df.{n}({{"revenue", "location", "id_new"}});'
        if n in {'sort_values','sort_cases','top','bottom','repeated_cases','unique_cases','set_cases','reset_cases','align_cases'}:
            if n == 'sort_values':
                if 'ArrangeSpec' in sig:
                    return df_base() + '\n\nauto out = df.sort_values({{.feature = "sales", .ascending = false}});'
                return df_base() + '\n\nauto out = df.sort_values({"sales"}, false, true);'
            if n == 'sort_cases':
                return df_base() + '\n\nauto with_cases = df.with_cases(pluss::table::Thread(pluss::make_note({"cs2", "cs0", "cs1"})));\nauto out = with_cases.sort_cases(true, true);'
            if n == 'top':
                return df_base() + '\n\nauto out = df.top("sales", 2);'
            if n == 'bottom':
                return df_base() + '\n\nauto out = df.bottom("sales", 2);'
            if n == 'repeated_cases':
                return df_base() + '\n\nauto out = df.repeated_cases();'
            if n == 'unique_cases':
                return df_base() + '\n\nauto out = df.unique_cases();'
            if n == 'set_cases':
                return df_base() + '\n\nauto out = df.set_cases("city");'
            if n == 'reset_cases':
                return df_base() + '\n\nauto with_cases = df.set_cases("city", true);\nauto out = with_cases.reset_cases("case");'
            if n == 'align_cases':
                return df_base() + '\n\ndf = df.with_cases(pluss::table::Thread(pluss::make_note({"cs0", "cs1", "cs2"})));\nauto new_cases = pluss::table::Thread(pluss::make_note({"cs1", "cs0", "cs2"}));\nauto out = df.align_cases(new_cases);'
        if n in {'join','update','conform_to','reindex_like','reindex_cases','stitch_on','combine'}:
            if n == 'conform_to' or n == 'reindex_like':
                return df_two() + f'\n\nauto out = left.{n}(right);'
            if n == 'reindex_cases':
                return df_base() + '\n\nauto cases = pluss::table::Thread(pluss::make_note({"cs1", "cs0", "cs2"}));\nauto out = df.reindex_cases(cases);'
            if n == 'stitch_on':
                if 'initializer_list<std::string_view> left_features' in sig:
                    return df_two() + '\n\nauto out = left.stitch_on({"id"}, right, {"id"}, "_right");'
                return df_two() + '\n\nauto out = left.stitch_on("id", right, "id", "_right");'
            if n == 'combine' and 'CombineSpec' in sig:
                return df_two() + '\n\npluss::table::DataFrame::CombineSpec spec;\nspec.op = pluss::table::DataFrame::CombineOp::Update;\nauto out = left.combine(right, spec);'
            return df_two() + f'\n\nauto out = left.{n}(right);'
        if n == 'unify':
            if 'initializer_list<std::string_view> left_on' in sig:
                return (
                    'auto left = pluss::table::DataFrame({\n'
                    '    {"course", pluss::make_note({"STAT200", "STAT200", "STAT300"})},\n'
                    '    {"OrgDefinedId", pluss::make_note({"10", "20", "30"})},\n'
                    '    {"midterm", pluss::make_series({71, 82, 93})},\n'
                    '});\n'
                    'auto right = pluss::table::DataFrame({\n'
                    '    {"course_code", pluss::make_note({"STAT200", "STAT300"})},\n'
                    '    {"Username", pluss::make_note({"20", "30"})},\n'
                    '    {"Final Points Grade", pluss::make_series({88, 99})},\n'
                    '});\n\n'
                    'auto out = left.unify(\n'
                    '    {"course", "OrgDefinedId"},\n'
                    '    right,\n'
                    '    {"course_code", "Username"},\n'
                    '    "inner",\n'
                    '    "_right",\n'
                    '    "one_to_one"\n'
                    ');'
                )
            return (
                'auto left = pluss::table::DataFrame({\n'
                '    {"OrgDefinedId", pluss::make_note({"10", "20", "30"})},\n'
                '    {"midterm", pluss::make_series({71, 82, 93})},\n'
                '});\n'
                'auto right = pluss::table::DataFrame({\n'
                '    {"OrgDefinedId", pluss::make_note({"20", "30"})},\n'
                '    {"Final Points Grade", pluss::make_series({88, 99})},\n'
                '});\n\n'
                'auto out = left.unify(\n'
                '    "OrgDefinedId",\n'
                '    right,\n'
                '    "",\n'
                '    "left",\n'
                '    "_right",\n'
                '    "many_to_one"\n'
                ');'
            )
        if n in {'set_features','set_features_','set'}:
            if n == 'set_features':
                return df_base() + '\n\nauto out = df.set_features("profit", pluss::table::Thread(pluss::make_series({40.0, 35.0, 30.0})));'
            if n == 'set_features_':
                return df_base() + '\n\ndf.set_features_("profit", pluss::table::Thread(pluss::make_series({40.0, 35.0, 30.0})));'
            if n == 'set_features' or n == 'set_features_':
                if 'initializer_list<std::string_view> names' in sig:
                    return df_base() + (
                        '\n\nauto out = df.set_features(\n'
                        '    {"profit", "cost"},\n'
                        '    {\n'
                        '        pluss::table::Thread(pluss::make_series({40.0, 35.0, 30.0})),\n'
                        '        pluss::table::Thread(pluss::make_series({60.0, 85.0, 60.0})),\n'
                        '    });'
                        if not n.endswith('_') else
                        '\n\ndf.set_features_(\n'
                        '    {"profit", "cost"},\n'
                        '    {\n'
                        '        pluss::table::Thread(pluss::make_series({40.0, 35.0, 30.0})),\n'
                        '        pluss::table::Thread(pluss::make_series({60.0, 85.0, 60.0})),\n'
                        '    });'
                    )
                return df_base() + (f'\n\nauto out = df.{n}("profit", pluss::table::Thread(pluss::make_series({{40.0, 35.0, 30.0}})));' if not n.endswith('_') else '\n\ndf.set_features_("profit", pluss::table::Thread(pluss::make_series({40.0, 35.0, 30.0})));')
            if n == 'set':
                if 'initializer_list' in sig:
                    return df_base() + '\n\ndf.set({"profit", "cost"}) = {pluss::table::Thread(pluss::make_series({40.0, 35.0, 30.0})), pluss::table::Thread(pluss::make_series({60.0, 85.0, 60.0}))};'
                return df_base() + '\n\ndf.set("profit") = pluss::table::Thread(pluss::make_series({40.0, 35.0, 30.0}));'
        if n == 'plot':
            return df_base() + '\n\nauto fig = df.plot().line("sales");'
        if n == 'sql':
            return df_base() + '\n\nauto out = df.sql().execute("SELECT id, sales FROM self WHERE sales > 95");'
        if n == 'prepare_sql':
            return df_base() + '\n\nauto stmt = df.prepare_sql("SELECT id, sales FROM self WHERE sales > {min_sales}");'
        if n in {'head','tail','sample','slice','take','where','filter','query','assign','transform'}:
            if n == 'head':
                return df_base() + '\n\nauto out = df.head(2, false);'
            if n == 'tail':
                return df_base() + '\n\nauto out = df.tail(2, false);'
            if n == 'sample':
                return df_base() + '\n\nauto out = df.sample(2, 7);'
            if n == 'slice':
                return df_base() + '\n\nauto out = df.slice(0, 2);'
            if n == 'take':
                return df_base() + '\n\nstd::vector<std::int64_t> picks{0, 2};\nauto out = df.take(picks);'
            if n == 'where' or n == 'filter':
                return df_base() + '\n\nauto mask = pluss::make_series({true, false, true}, pluss::codem::ScalarType::Bool);\nauto out = df.' + n + '(mask);'
            if n == 'query':
                return df_base() + ('\n\nauto out = df.query("sales > 95");' if 'string_view' in sig else '\n\nauto out = df.query("sales > 95");')
            if n == 'assign':
                if 'pair<std::string, Thread>' in sig:
                    return df_base() + '\n\nauto out = df.assign({{"profit", pluss::table::Thread(pluss::make_series({40.0, 35.0, 30.0}))}});'
                return df_base() + '\n\nauto out = df.assign({{.feature = "profit", .expr = [](const pluss::table::DataFrame& frame) { return pluss::table::Thread(frame.features("sales").as_series().sub(60)); }}});'
            if n == 'transform':
                if 'std::string_view feature' in sig:
                    return df_base() + '\n\nauto out = df.transform("sales", [](const pluss::table::Thread& thread) { return thread.clip(0.0, 110.0); });'
                if 'TransformSpec' in sig:
                    return df_base() + '\n\nauto out = df.transform({{.feature = "sales", .expr = [](const pluss::table::Thread& thread) { return thread.clip(0.0, 110.0); }}});'
                return df_base() + '\n\nauto out = df.transform([](const pluss::table::Thread& thread) { return thread.kind() == pluss::table::ThreadKind::Series ? thread.clip(0.0, 110.0) : thread; });'
        if n in {'tz_convert','tz_localize','to_utc','shift_time','floor_time','ceil_time','round_time','resample'}:
            if n == 'resample':
                return df_time() + '\n\nauto out = df.resample("ts", {"load"}, "1h", "mean", 1, false);'
            if n == 'tz_convert':
                return df_time() + '\n\nauto out = df.tz_convert({"ts"}, "America/Edmonton");'
            if n == 'tz_localize':
                return df_time() + '\n\nauto out = df.tz_localize({"ts"}, "America/Edmonton", 0, "shift_forward");'
            if n == 'to_utc':
                return df_time() + '\n\nauto out = df.to_utc({"ts"});'
            if n == 'shift_time':
                return df_time() + '\n\nauto out = df.shift_time({"ts"}, 1, "hour");'
            if n == 'floor_time':
                return df_time() + '\n\nauto out = df.floor_time({"ts"}, "hour");'
            if n == 'ceil_time':
                return df_time() + '\n\nauto out = df.ceil_time({"ts"}, "hour");'
            if n == 'round_time':
                return df_time() + '\n\nauto out = df.round_time({"ts"}, "hour");'
        if n in {'aggregate','group_by','pivot','pivot_table','count_by','mean_by'}:
            if n == 'aggregate':
                return df_group() + '\n\nauto out = df.aggregate({{"sales_mean", "sales", pluss::table::DataFrame::SummaryOp::Mean}}, false);'
            if n == 'group_by':
                return df_group() + '\n\nauto out = df.group_by({"city"}, {{"sales_sum", "sales", pluss::table::DataFrame::GroupOp::Sum}}, true, false);'
            if n == 'pivot':
                return df_group() + '\n\nauto out = df.pivot("city", "product", "sales", true, false);'
            if n == 'pivot_table':
                return df_group() + '\n\nauto out = df.pivot_table("city", "product", "sales", pluss::table::DataFrame::GroupOp::Mean, true, false);'
            if n == 'count_by':
                return df_group() + '\n\nauto out = df.count_by("city", 3, true, false);'
            if n == 'mean_by':
                return df_group() + '\n\nauto out = df.mean_by("city", "sales", 3, 1, true, false);'
        if n == 'count_distinct':
            return df_base() + '\n\nauto out = df.count_distinct({"id", "sales", "city"}, true, false);'
        if n == 'corr':
            return df_base() + '\n\nauto out = df.corr({"id", "sales"}, pluss::table::DataFrame::CorrMethod::Pearson, false);'
        if n in {'add','sub','subtract','mul','multiply','div','divide','truediv','floordiv','mod','pow','eq','ne','lt','le','gt','ge','compare'}:
            if 'const DataFrame&' in sig:
                return df_two() + f'\n\nauto out = left.{n}(right);'
            return df_base() + f'\n\nauto out = df.{n}(10);'
        if n in {'radd','rsub','rmul','rdiv','rtruediv','rfloordiv','rmod','rpow'}:
            return df_base() + f'\n\nauto out = df.{n}(10);'
        if n in {'sum','mean','min','max','std','var','count','describe','transpose','tally','window','rolling'}:
            if n == 'window' or n == 'rolling':
                return df_base() + '\n\nauto out = df.' + n + '({{.output = "sales_roll", .feature = "sales", .op = pluss::table::ThreadWindowOp::RollingMean, .window = 2, .min_periods = 1}}, true);'
            if n == 'transpose':
                return df_base() + '\n\nauto out = df.transpose(false);'
            if n == 'tally':
                return df_group() + '\n\nauto out = df.tally("city", 3, true, false);'
            if n == 'count':
                return df_base() + '\n\nauto out = df.count(false);'
            return df_base() + f'\n\nauto out = df.{n}(false);'
        if n in {'all', 'any'}:
            return (
                'auto df = pluss::table::DataFrame({\n'
                '    {"passed_math", pluss::table::Thread(pluss::make_series({true, true, false}, pluss::codem::ScalarType::Bool))},\n'
                '    {"passed_lab", pluss::table::Thread(pluss::make_series({true, false, false}, pluss::codem::ScalarType::Bool))},\n'
                '});\n\n'
                f'auto out = df.{n}(1, {{"passed_math", "passed_lab"}}, true);'
            )
        if n in {'isna','isnull','notna','notnull'}:
            return df_base() + f'\n\nauto out = df.{n}({{"sales"}});'
        if n in {'fillna','fillna_','dropna','dropna_all','ffill','bfill','count_null_rows'}:
            if n == 'fillna' or n == 'fillna_':
                mut = n.endswith('_')
                if 'std::string_view' in sig or 'const char*' in sig:
                    code = 'auto df = pluss::table::DataFrame({{"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))}});'
                    call = ('df.fillna_("Unknown");' if mut else 'auto out = df.fillna("Unknown");')
                    return code + '\n\n' + call
                if 'Timeline' in sig:
                    return 'auto df = pluss::table::DataFrame({{"ts", pluss::table::Thread(pluss::make_timeline({"2024-01-01T09:10:00Z", "2024-01-01T09:40:00Z"}))}});\n\n' + ('df.fillna_(pluss::make_timeline("2024-01-01T00:00:00Z"));' if mut else 'auto out = df.fillna(pluss::make_timeline("2024-01-01T00:00:00Z"));')
                if 'TimeIndex' in sig:
                    return 'auto df = pluss::table::DataFrame({{"idx", pluss::table::Thread(pluss::make_timeindex({"2024-01-01T09:10:00Z", "2024-01-01T09:40:00Z"}))}});\n\n' + ('df.fillna_(pluss::make_timeindex("2024-01-01T00:00:00Z"));' if mut else 'auto out = df.fillna(pluss::make_timeindex("2024-01-01T00:00:00Z"));')
                return df_base() + ('\n\ndf.fillna_(0.0);' if mut else '\n\nauto out = df.fillna(0.0, {"sales"});')
            if n == 'dropna_all':
                return df_base() + '\n\nauto out = df.dropna_all({"sales"});'
            if n == 'count_null_rows':
                return df_base() + '\n\nauto out = df.count_null_rows();'
            if n == 'bfill':
                return (
                    'auto score = pluss::table::Thread(pluss::make_series({71.0, 0.0, 88.0, 0.0}))\n'
                    '                 .with_validity_mask(pluss::make_series({true, false, true, false}, pluss::codem::ScalarType::Bool));\n'
                    'auto df = pluss::table::DataFrame({\n'
                    '    {"score", score},\n'
                    '    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi", "Tamale"}))},\n'
                    '});\n'
                    '\n'
                    'auto out = df.bfill();'
                )
            if n == 'dropna':
                return (
                    'auto sales = pluss::table::Thread(pluss::make_series({100.0, 0.0, 90.0}))\n'
                    '                 .with_validity_mask(pluss::make_series({true, false, true}, pluss::codem::ScalarType::Bool));\n'
                    'auto df = pluss::table::DataFrame({\n'
                    '    {"id", pluss::make_series({1, 2, 3})},\n'
                    '    {"sales", sales},\n'
                    '    {"city", pluss::make_note({"Accra", "Tema", "Kumasi"})},\n'
                    '});\n\n'
                    'auto out = df.dropna();'
                )
            return df_base() + f'\n\nauto out = df.{n}();'
        if n in {'view','copy','equals','to_string','info_string','info','save_csv','save_excel','save_txt','save_json','save_parquet','save_as'}:
            if n == 'view':
                return df_base() + '\n\nauto out = df.view(0, 2, false);'
            if n == 'copy':
                return df_base() + '\n\nauto out = df.copy();'
            if n == 'equals':
                return df_two() + '\n\nauto out = left.equals(right);'
            if n == 'to_string':
                return df_base() + '\n\nauto out = df.to_string();'
            if n == 'info_string':
                return df_base() + '\n\nauto out = df.info_string();'
            if n == 'info':
                return df_base() + '\n\ndf.info();'
            if n.startswith('save_') or n == 'save_as':
                target = {
                    'save_csv': f'"{GENERIC_OUTPUT_CSV_PATH}"', 'save_excel': f'"{GENERIC_OUTPUT_XLSX_PATH}"', 'save_txt': f'"{GENERIC_OUTPUT_TXT_PATH}"',
                    'save_json': f'"{GENERIC_OUTPUT_JSON_PATH}"', 'save_parquet': f'"{GENERIC_OUTPUT_PARQUET_PATH}"', 'save_as': f'"json", "{GENERIC_OUTPUT_JSON_PATH}"'
                }[n]
                return df_base() + f'\n\ndf.{n}({target});'
        if n in {'defined','unique'}:
            return df_base() + f'\n\nauto out = df.{n}();'
        if n == 'unsqueeze':
            return df_base() + ('\n\nauto out = df.unsqueeze();' if 'depth_labels' not in sig else '\n\nauto labels = pluss::table::Thread(pluss::make_note({"train"}));\nauto out = df.unsqueeze(labels);')
        return df_base() + f'\n\nauto out = df.{n}();'

    if class_name == 'ColumnRef':
        base = feature_ref_base()
        if n in {'name', 'thread', 'mean', 'value', 'values'}:
            return base + f'\n\nauto out = ref.{n}();'
        if n == 'head':
            return base + '\n\nauto out = ref.head(2);'
        return base + f'\n\nauto out = ref.{n}();'

    if class_name == 'DataFramePlotAccessor':
        base = plot_accessor_base()
        if n == 'act':
            return (
                base + '\n\npluss::table::ActPlotOptions options;\n'
                'options.max_lag = 24;\n'
                'auto fig = plot.act("sales", options);'
            )
        if n == 'andrews_curves':
            return (
                'auto df = pluss::table::DataFrame({\n'
                '    {"x1", pluss::make_series({1.0, 2.0, 3.0})},\n'
                '    {"x2", pluss::make_series({4.0, 3.0, 2.0})},\n'
                '    {"x3", pluss::make_series({2.0, 5.0, 4.0})},\n'
                '    {"group", pluss::make_note({"A", "B", "A"})},\n'
                '});\n'
                'auto plot = df.plot();\n\n'
                'auto fig = plot.andrews_curves({"x1", "x2", "x3"}, "group");'
            )
        if n == 'line':
            if 'initializer_list<std::string_view>' in sig:
                return base + '\n\nauto fig = plot.line({"sales"}, std::nullopt);'
            return base + '\n\nauto fig = plot.line("sales");'
        if n == 'area':
            if 'initializer_list<std::string_view>' in sig or 'RangeView::Of<std::string_view> y_features' in sig:
                return (
                    'auto df = pluss::table::DataFrame({\n'
                    '    {"month", pluss::make_note({"Jan", "Feb", "Mar"})},\n'
                    '    {"sales", pluss::make_series({100.0, 120.0, 90.0})},\n'
                    '    {"cost", pluss::make_series({60.0, 80.0, 55.0})},\n'
                    '});\n'
                    'auto plot = df.plot();\n\n'
                    'auto fig = plot.area({"sales", "cost"}, "month");'
                )
            if 'AreaPlotOptions' in sig:
                return (
                    base + '\n\npluss::table::AreaPlotOptions options;\n'
                    'options.alpha = 0.6;\n'
                    'auto fig = plot.area("sales", "id", options);'
                )
            return base + '\n\nauto fig = plot.area("sales", "id");'
        if n == 'scatter':
            return base + '\n\nauto fig = plot.scatter("id", "sales");'
        if n == 'density':
            if ('initializer_list<std::string_view> features' in sig or 'RangeView::Of<std::string_view> features' in sig) and 'DensityPlotOptions' in sig:
                return (
                    'auto df = pluss::table::DataFrame({\n'
                    '    {"sales", pluss::make_series({100.0, 120.0, 90.0})},\n'
                    '    {"cost", pluss::make_series({60.0, 80.0, 55.0})},\n'
                    '});\n'
                    'auto plot = df.plot();\n\n'
                    'pluss::table::DensityPlotOptions options;\n'
                    'options.points = 256;\n'
                    'auto fig = plot.density({"sales", "cost"}, options);'
                )
            if 'DensityPlotOptions' in sig:
                return (
                    base + '\n\npluss::table::DensityPlotOptions options;\n'
                    'options.points = 256;\n'
                    'auto fig = plot.density("sales", options);'
                )
            if 'RangeView::Of<std::string_view> features' in sig or 'initializer_list<std::string_view> features' in sig:
                return (
                    'auto df = pluss::table::DataFrame({\n'
                    '    {"sales", pluss::make_series({100.0, 120.0, 90.0})},\n'
                    '    {"cost", pluss::make_series({60.0, 80.0, 55.0})},\n'
                    '});\n'
                    'auto plot = df.plot();\n\n'
                    'auto fig = plot.density({"sales", "cost"}, 128);'
                )
            return base + '\n\nauto fig = plot.density("sales", 128);'
        if n == 'bar':
            if 'initializer_list<std::string_view>' in sig or 'RangeView::Of<std::string_view> y_features' in sig:
                return (
                    'auto df = pluss::table::DataFrame({\n'
                    '    {"student", pluss::make_note({"Laurel", "Kojo", "Ama"})},\n'
                    '    {"midterm", pluss::make_series({71.0, 82.0, 93.0})},\n'
                    '    {"final", pluss::make_series({88.0, 79.0, 95.0})},\n'
                    '});\n'
                    'auto plot = df.plot();\n\n'
                    'auto fig = plot.bar({"midterm", "final"}, "student");'
                )
            if 'BarPlotOptions' in sig:
                return (
                    base + '\n\npluss::table::BarPlotOptions options;\n'
                    'options.horizontal = true;\n'
                    'auto fig = plot.bar("sales", "city", options);'
                )
            return base + '\n\nauto fig = plot.bar("sales", "city");'
        if n == 'bar_stacked':
            if 'BarStackedPlotOptions' in sig:
                return (
                    'auto df = pluss::table::DataFrame({\n'
                    '    {"student", pluss::make_note({"Laurel", "Kojo", "Ama"})},\n'
                    '    {"midterm", pluss::make_series({71.0, 82.0, 93.0})},\n'
                    '    {"final", pluss::make_series({88.0, 79.0, 95.0})},\n'
                    '});\n'
                    'auto plot = df.plot();\n\n'
                    'pluss::table::BarStackedPlotOptions options;\n'
                    'options.alpha = 0.85;\n'
                    'auto fig = plot.bar_stacked({"midterm", "final"}, "student", options);'
                )
            return (
                'auto df = pluss::table::DataFrame({\n'
                '    {"student", pluss::make_note({"Laurel", "Kojo", "Ama"})},\n'
                '    {"midterm", pluss::make_series({71.0, 82.0, 93.0})},\n'
                '    {"final", pluss::make_series({88.0, 79.0, 95.0})},\n'
                '});\n'
                'auto plot = df.plot();\n\n'
                'auto fig = plot.bar_stacked({"midterm", "final"}, "student");'
            )
        if n == 'box':
            if 'BoxPlotOptions' in sig:
                return (
                    'auto df = pluss::table::DataFrame({\n'
                    '    {"sales", pluss::make_series({100.0, 120.0, 90.0})},\n'
                    '    {"cost", pluss::make_series({60.0, 80.0, 55.0})},\n'
                    '});\n'
                    'auto plot = df.plot();\n\n'
                    'pluss::table::BoxPlotOptions options;\n'
                    'options.line_width = 1.6;\n'
                    'auto fig = plot.box({"sales", "cost"}, options);'
                )
            return (
                'auto df = pluss::table::DataFrame({\n'
                '    {"sales", pluss::make_series({100.0, 120.0, 90.0})},\n'
                '    {"cost", pluss::make_series({60.0, 80.0, 55.0})},\n'
                '});\n'
                'auto plot = df.plot();\n\n'
                'auto fig = plot.box({"sales", "cost"});'
            )
        if n == 'bootstrap':
            return (
                base + '\n\npluss::table::BootstrapPlotOptions options;\n'
                'options.samples = 1000;\n'
                'auto fig = plot.bootstrap("sales", options);'
            )
        if n == 'brief':
            if 'Studio& figure' in sig:
                return (
                    'auto df = pluss::table::DataFrame({\n'
                    '    {"sales", pluss::make_series({100.0, 120.0, 90.0})},\n'
                    '    {"cost", pluss::make_series({60.0, 80.0, 55.0})},\n'
                    '});\n'
                    'auto plot = df.plot();\n\n'
                    'auto fig = plot.line("sales");\n'
                    'pluss::table::BriefPlotOptions options;\n'
                    'options.max_cases = 6;\n'
                    'plot.brief(fig, {"sales"}, options);'
                )
            return (
                'auto df = pluss::table::DataFrame({\n'
                '    {"sales", pluss::make_series({100.0, 120.0, 90.0})},\n'
                '    {"cost", pluss::make_series({60.0, 80.0, 55.0})},\n'
                '});\n'
                'auto plot = df.plot();\n\n'
                'pluss::table::BriefPlotOptions options;\n'
                'options.max_cases = 6;\n'
                'auto fig = plot.brief({"sales", "cost"}, options);'
            )
        if n == 'corr_heatmap':
            if 'CorrHeatmapPlotOptions' in sig:
                return (
                    'auto df = pluss::table::DataFrame({\n'
                    '    {"x1", pluss::make_series({1.0, 2.0, 3.0})},\n'
                    '    {"x2", pluss::make_series({2.0, 4.0, 6.0})},\n'
                    '    {"x3", pluss::make_series({3.0, 1.0, 5.0})},\n'
                    '});\n'
                    'auto plot = df.plot();\n\n'
                    'pluss::table::CorrHeatmapPlotOptions options;\n'
                    'options.annotate_levels = true;\n'
                    'auto fig = plot.corr_heatmap({"x1", "x2", "x3"}, options);'
                )
            return (
                'auto df = pluss::table::DataFrame({\n'
                '    {"x1", pluss::make_series({1.0, 2.0, 3.0})},\n'
                '    {"x2", pluss::make_series({2.0, 4.0, 6.0})},\n'
                '    {"x3", pluss::make_series({3.0, 1.0, 5.0})},\n'
                '});\n'
                'auto plot = df.plot();\n\n'
                'auto fig = plot.corr_heatmap({"x1", "x2", "x3"});'
            )
        if n == 'auto_plot':
            return (
                'auto df = pluss::table::DataFrame({\n'
                '    {"student", pluss::make_note({"Laurel", "Kojo", "Ama"})},\n'
                '    {"midterm", pluss::make_series({71.0, 82.0, 93.0})},\n'
                '    {"final", pluss::make_series({88.0, 79.0, 95.0})},\n'
                '});\n'
                'auto plot = df.plot();\n\n'
                'auto fig = plot.auto_plot({"midterm", "final"});'
            )
        if n == 'hist':
            return base + '\n\nauto fig = plot.hist("sales", 12);'
        return base + f'\n\nauto out = plot.{n}();'

    if class_name == 'DataFrameSqlAccessor':
        base = sql_accessor_base()
        if n == 'execute':
            return base + '\n\nauto out = sql.execute("SELECT id, sales FROM self WHERE sales > 95");'
        if n == 'prepare':
            return base + '\n\nauto out = sql.prepare("SELECT id, sales FROM self WHERE sales > {min_sales}");'
        return base + f'\n\nauto out = sql.{n}();'

    if class_name == 'DataFrameSqlPrepared':
        base = sql_prepared_base()
        if n == 'execute':
            if 'identifier_params' in sig:
                return base + '\n\nauto out = stmt.execute({{"min_sales", 95.0}}, {{"table_name", "self"}});'
            if 'params)' in sig:
                return base + '\n\nauto out = stmt.execute({{"min_sales", 95.0}});'
            return base + '\n\nauto out = stmt.execute();'
        return base + f'\n\nauto out = stmt.{n}();'

    if class_name == 'Thread':
        if n in {'defined','unique','size','kind','dtype_name','value','values','has_validity_mask','validity_mask'}:
            base = thread_series()
            return base + f'\n\nauto out = thread.{n}();'
        if n == 'is_valid':
            return thread_series() + '\n\nauto out = thread.is_valid(0);'
        if n in {'with_validity_mask','with_validity_mask_','clear_validity_mask','clear_validity_mask_'}:
            mask = 'auto mask = pluss::make_series({true, false, true}, pluss::codem::ScalarType::Bool);\n' + thread_series()
            if n.endswith('_'):
                return mask + f'\n\nthread.{n}(mask);' if 'with_' in n else mask + '\n\nthread.clear_validity_mask_();'
            return mask + f'\n\nauto out = thread.{n}(mask);' if 'with_' in n else mask + '\n\nauto out = thread.clear_validity_mask();'
        if n in {'isnull','notnull','isnan','isfinite','abs','log','exp','dropna','ffill','bfill','sum','mean','min','max'}:
            if n == 'dropna':
                return (
                    'auto thread = pluss::table::Thread(pluss::make_series({10.0, 0.0, 30.0}));\n'
                    'thread = thread.with_validity_mask(pluss::make_series({true, false, true}, pluss::codem::ScalarType::Bool));\n\n'
                    'auto out = thread.dropna();'
                )
            if n == 'bfill':
                return (
                    'auto thread = pluss::table::Thread(pluss::make_series({10.0, 0.0, 30.0, 0.0}));\n'
                    'thread = thread.with_validity_mask(pluss::make_series({true, false, true, false}, pluss::codem::ScalarType::Bool));\n\n'
                    'auto out = thread.bfill();'
                )
            return thread_series() + f'\n\nauto out = thread.{n}();'
        if n in {'var','std'}:
            return thread_series() + f'\n\nauto out = thread.{n}(true);'
        if n in {'fillna','fillna_','clip'}:
            if 'std::string_view' in sig or 'const char*' in sig:
                base = thread_note()
                return base + ('\n\nthread.fillna_("Unknown");' if n.endswith('_') else '\n\nauto out = thread.fillna("Unknown");')
            if 'Timeline' in sig:
                base = thread_time()
                return base + ('\n\nthread.fillna_(pluss::make_timeline("2024-01-01T00:00:00Z"));' if n.endswith('_') else '\n\nauto out = thread.fillna(pluss::make_timeline("2024-01-01T00:00:00Z"));')
            if 'TimeIndex' in sig:
                base = 'auto thread = pluss::table::Thread(pluss::make_timeindex({"2024-01-01T09:10:00Z", "2024-02-01T11:00:00Z"}));'
                return base + ('\n\nthread.fillna_(pluss::make_timeindex("2024-01-01T00:00:00Z"));' if n.endswith('_') else '\n\nauto out = thread.fillna(pluss::make_timeindex("2024-01-01T00:00:00Z"));')
            if n == 'clip':
                return thread_series() + '\n\nauto out = thread.clip(0.0, 25.0);'
            return thread_series() + ('\n\nthread.fillna_(0.0);' if n.endswith('_') else '\n\nauto out = thread.fillna(0.0);')
        if n in {'series','note','timeline','timeindex','as_series','as_note','as_timeline','as_timeindex'}:
            base = thread_series() if 'series' in n else thread_note() if 'note' in n else thread_time()
            return base + f'\n\nauto out = thread.{n}();'
        if n in {'head','tail','slice','take','filter','window','cast','astype','bucketize','btw','contains','corr','count'}:
            if n == 'head': return thread_series() + '\n\nauto out = thread.head(2);'
            if n == 'tail': return thread_series() + '\n\nauto out = thread.tail(2);'
            if n == 'slice': return thread_series() + '\n\nauto out = thread.slice(0, 2);'
            if n == 'take': return thread_series() + '\n\nstd::vector<std::int64_t> picks{0, 2};\nauto out = thread.take(picks);'
            if n == 'filter': return thread_series() + '\n\nauto mask = pluss::make_series({true, false, true}, pluss::codem::ScalarType::Bool);\nauto out = thread.filter(mask);'
            if n == 'window': return thread_series() + '\n\nauto out = thread.window(2, pluss::table::ThreadWindowOp::RollingMean, 1);'
            if n == 'cast': return 'auto thread = pluss::table::Thread(pluss::make_note({"1.0", "2.0", "3.0"}));\n\nauto out = thread.cast(pluss::table::ThreadCastTarget::Series, pluss::codem::ScalarType::Float64);'
            if n == 'astype': return thread_series() + '\n\nauto out = thread.astype(pluss::codem::ScalarType::Float64);'
            if n == 'contains': return 'auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));\n\nauto out = thread.contains("Acc");'
            if n == 'corr': return 'auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));\nauto other = pluss::table::Thread(pluss::make_series({12.0, 18.0, 29.0}));\n\nauto out = thread.corr(other, pluss::table::ThreadCorrMethod::Pearson);'
            if n == 'count': return 'auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));\n\nauto out = thread.count(true);'
            if n == 'bucketize':
                return (
                    'auto thread = pluss::table::Thread(pluss::make_series({12.0, 18.0, 27.0, 35.0, 0.0}));\n'
                    'thread = thread.with_validity_mask(pluss::make_series({true, true, true, true, false}, pluss::codem::ScalarType::Bool));\n'
                    'std::vector<double> bins{0.0, 20.0, 30.0, 40.0};\n\n'
                    'auto out = thread.bucketize(bins, {}, true, true);'
                )
            if n == 'btw':
                if 'double a, double b' in sig:
                    return (
                        'auto thread = pluss::table::Thread(pluss::make_series({45.0, 65.0, 82.0, 101.0}));\n\n'
                        'auto out = thread.btw(50.0, 100.0, true);'
                    )
                if 'std::string_view lo, std::string_view hi' in sig:
                    return (
                        'auto thread = pluss::table::Thread(pluss::make_timeline({\n'
                        '    "2024-01-05T09:00:00Z",\n'
                        '    "2024-01-18T12:00:00Z",\n'
                        '    "2024-02-02T08:00:00Z",\n'
                        '}));\n\n'
                        'auto out = thread.btw("2024-01-01T00:00:00Z", "2024-01-31T23:59:59Z", true);'
                    )
                return (
                    'auto thread = pluss::table::Thread(pluss::make_series({45.0, 65.0, 82.0}));\n'
                    'auto lo = pluss::table::Thread(pluss::make_series({40.0, 60.0, 80.0}));\n'
                    'auto hi = pluss::table::Thread(pluss::make_series({50.0, 70.0, 81.0}));\n\n'
                    'auto out = thread.btw(lo, hi, true);'
                )
        if n in {'lower','replace','contains','strip','lstrip','rstrip','distinct','describe','display_labels','display_name'}:
            if n == 'lower': return thread_note() + '\n\nauto out = thread.lower();'
            if n == 'replace': return thread_note() + '\n\nauto out = thread.replace("Tema", "Tamale");'
            if n == 'distinct': return 'auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Accra"}));\n\nauto out = thread.distinct(true);'
            if n == 'describe': return thread_series() + '\n\nauto out = thread.describe(false);'
            if n == 'display_labels': return 'auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0})).with_display_labels(pluss::table::Thread(pluss::make_note({"a", "b", "c"})));\n\nauto out = thread.display_labels();'
            if n == 'display_name': return 'auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0})).with_display_name("sales");\n\nauto out = thread.display_name();'
            if n == 'strip':
                if 'std::string_view chars' in sig:
                    return thread_note() + '\n\nauto out = thread.strip("# ");'
                return thread_note() + '\n\nauto out = thread.strip();'
            if n == 'lstrip':
                if 'std::string_view chars' in sig:
                    return 'auto thread = pluss::table::Thread(pluss::make_note({"#203930", "##7788", "4455##"}));\n\nauto out = thread.lstrip("#");'
                return thread_note() + '\n\nauto out = thread.lstrip();'
            if n == 'rstrip':
                if 'std::string_view chars' in sig:
                    return 'auto thread = pluss::table::Thread(pluss::make_note({"#203930", "##7788", "4455##"}));\n\nauto out = thread.rstrip("#");'
                return thread_note() + '\n\nauto out = thread.rstrip();'
            return thread_note() + '\n\nauto out = thread.contains("Accra");'
        if n in {'year','month','day','hour','to_note_weekday'}:
            return thread_time() + f'\n\nauto out = thread.{n}();'
        if n in {'numeric_binary','compare','compare_rows'}:
            if n == 'numeric_binary':
                return thread_series() + '\n\nauto out = thread.numeric_binary(10.0, pluss::table::ThreadBinaryMathOp::Add, false);'
            if n == 'compare_rows':
                return thread_series() + '\n\nauto out = thread.compare_rows(0, 1, true);'
            if 'const Thread&' in sig:
                return thread_series() + '\n\nauto other = pluss::table::Thread(pluss::make_series({15.0, 10.0, 35.0}));\nauto out = thread.compare(other, pluss::table::ThreadBinaryCompareOp::Gt);'
            return thread_series() + '\n\nauto out = thread.compare(15.0, pluss::table::ThreadBinaryCompareOp::Gt);'
        if n in {'to_string','show'}:
            return thread_series() + ('\n\nauto out = thread.to_string();' if n == 'to_string' else '\n\nthread.show();')
        return thread_series() + f'\n\nauto out = thread.{n}();'

    if class_name == 'ThreadValue':
        base = thread_value_base()
        if n in {'defined', 'kind', 'thread', 'as_series', 'as_note', 'as_timeline', 'as_timeindex'}:
            if 'note' in n:
                base = thread_value_note_base()
            elif 'timeline' in n:
                base = thread_value_timeline_base()
            elif 'timeindex' in n:
                base = thread_value_timeindex_base()
            return base + f'\n\nauto out = value.{n}();'
        return base + f'\n\nauto out = value.{n}();'

    if class_name == 'Atlas':
        if n in {'defined','unique','empty','depth','nrows','ncols','column_names','axis0_labels','frames'}:
            return atlas_base() + f'\n\nauto out = atlas.{n}();'
        if n in {'frame','iloc','loc','dloc','plot','squeeze','head','tail','slice_frames','sort_depth','relabel_depth','align_depth','map_frames','aggregate_over_frames','tabelize','kindelize','to_string','show'}:
            if n == 'frame' or n == 'iloc': return atlas_base() + f'\n\nauto out = atlas.{n}(0);'
            if n == 'dloc': return atlas_base() + '\n\nauto out = atlas.dloc(1);'
            if n == 'loc': return atlas_base() + '\n\nauto labeled = pluss::table::Atlas({f0, f1}, pluss::table::Thread(pluss::make_note({"train", "test"})));\nauto out = labeled.loc("test");'
            if n == 'plot': return atlas_base() + '\n\nauto fig = atlas.plot(0).line("sales");'
            if n == 'squeeze': return atlas_base() + '\n\nauto out = atlas.squeeze();'
            if n == 'head': return atlas_base() + '\n\nauto out = atlas.head(1);'
            if n == 'tail': return atlas_base() + '\n\nauto out = atlas.tail(1);'
            if n == 'slice_frames': return atlas_base() + '\n\nauto out = atlas.slice_frames(0, 1);'
            if n == 'sort_depth': return atlas_base() + '\n\nauto labeled = pluss::table::Atlas({f0, f1}, pluss::table::Thread(pluss::make_note({"test", "train"})));\nauto out = labeled.sort_depth(true, true);'
            if n == 'relabel_depth' or n == 'align_depth': return atlas_base() + f'\n\nauto labels = pluss::table::Thread(pluss::make_note({{"baseline", "stress"}}));\nauto out = atlas.{n}(labels);'
            if n == 'map_frames': return atlas_base() + '\n\nauto out = atlas.map_frames([](const pluss::table::DataFrame& frame) { return frame.head(1, false); });'
            if n == 'aggregate_over_frames': return atlas_base() + '\n\nauto out = atlas.aggregate_over_frames([](const std::vector<pluss::table::DataFrame>& frames) { return frames.front(); });'
            if n == 'tabelize': return 'using namespace pluss::kindle::literals;\nauto c0 = pluss::kindle::Catalog({"sales"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0}))});\nauto c1 = pluss::kindle::Catalog({"sales"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0}))});\nauto archive = pluss::kindle::Archive({c0, c1});\nauto out = pluss::table::Atlas::tabelize(archive);'
            if n == 'kindelize': return atlas_base() + '\n\nauto out = atlas.kindelize();'
            if n == 'to_string': return atlas_base() + '\n\nauto out = atlas.to_string();'
            if n == 'show': return atlas_base() + '\n\natlas.show();'
        return atlas_base() + f'\n\nauto out = atlas.{n}();'

    if class_name == 'Stream':
        base = stream_series()
        if n in {'defined','unique','size','kind','dtype_name','has_validity_mask','validity_mask','has_cases','cases','to_string','to_json'}:
            return base + f'\n\nauto out = stream.{n}();'
        if n in {'isnull','notnull','abs','log','exp','clear_validity_mask','clear_validity_mask_','clear_cases','clear_cases_','head','tail','dropna','ffill','bfill','sum','mean','min','max'}:
            if n == 'bfill':
                return (
                    'auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 0.0, 30.0, 0.0}));\n'
                    'stream = stream.with_validity_mask(pluss::make_series({true, false, true, false}, pluss::codem::ScalarType::Bool));\n\n'
                    'auto out = stream.bfill();'
                )
            if n == 'dropna':
                return (
                    'auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 0.0, 30.0}));\n'
                    'stream = stream.with_validity_mask(pluss::make_series({true, false, true}, pluss::codem::ScalarType::Bool));\n\n'
                    'auto out = stream.dropna();'
                )
            if n.endswith('_'): return base + f'\n\nstream.{n}();'
            return base + f'\n\nauto out = stream.{n}();'
        if n in {'var','std'}:
            return base + f'\n\nauto out = stream.{n}(true);'
        if n in {'fillna','clip','with_validity_mask','with_validity_mask_','with_cases','with_cases_','with_meta','with_meta_','drop_meta','drop_meta_','meta','has_meta','take','filter','slice','cast','astype','to_thread'}:
            if n == 'fillna':
                if 'std::string_view' in sig or 'const char*' in sig:
                    return stream_note() + '\n\nauto out = stream.fillna("Unknown");'
                if 'Timeline' in sig:
                    return stream_time() + '\n\nauto out = stream.fillna(pluss::make_timeline("2024-01-01T00:00:00Z"));'
                if 'TimeIndex' in sig:
                    return 'auto stream = pluss::kindle::Stream(pluss::make_timeindex({"2024-01-01T09:10:00Z", "2024-01-01T09:40:00Z"}));\n\nauto out = stream.fillna(pluss::make_timeindex("2024-01-01T00:00:00Z"));'
                return base + '\n\nauto out = stream.fillna(0.0);'
            if n == 'clip': return base + '\n\nauto out = stream.clip(0.0, 25.0);'
            if n == 'with_validity_mask' or n == 'with_validity_mask_':
                snippet = 'auto mask = pluss::make_series({true, false, true}, pluss::codem::ScalarType::Bool);\n' + base
                return snippet + ('\n\nauto out = stream.with_validity_mask(mask);' if not n.endswith('_') else '\n\nstream.with_validity_mask_(mask);')
            if n == 'with_cases' or n == 'with_cases_':
                snippet = 'auto cases = pluss::table::Thread(pluss::make_note({"cs0", "cs1", "cs2"}));\n' + base
                return snippet + ('\n\nauto out = stream.with_cases(cases);' if not n.endswith('_') else '\n\nstream.with_cases_(cases);')
            if n in {'with_meta','with_meta_'}:
                return base + ('\n\nauto out = stream.with_meta("unit", "km");' if not n.endswith('_') else '\n\nstream.with_meta_("unit", "km");')
            if n in {'drop_meta','drop_meta_'}:
                snippet = base + '\nstream = stream.with_meta("unit", "km");'
                return snippet + ('\n\nauto out = stream.drop_meta("unit");' if not n.endswith('_') else '\n\nstream.drop_meta_("unit");')
            if n == 'meta': return base + '\n\nstream = stream.with_meta("unit", "km");\nauto out = stream.meta("unit");'
            if n == 'has_meta': return base + '\n\nstream = stream.with_meta("unit", "km");\nauto out = stream.has_meta("unit");'
            if n == 'take': return base + '\n\nstd::vector<std::int64_t> picks{0, 2};\nauto out = stream.take(picks);'
            if n == 'filter': return base + '\n\nauto mask = pluss::make_series({true, false, true}, pluss::codem::ScalarType::Bool);\nauto out = stream.filter(mask);'
            if n == 'slice': return base + '\n\nauto out = stream.slice(0, 2);'
            if n == 'cast': return 'auto stream = pluss::kindle::Stream(pluss::make_note({"1.0", "2.0", "3.0"}));\n\nauto out = stream.cast(pluss::table::ThreadCastTarget::Series, pluss::codem::ScalarType::Float64);'
            if n == 'astype': return base + '\n\nauto out = stream.astype(pluss::codem::ScalarType::Float64);'
            if n == 'to_thread':
                return base + ('\n\nauto out = stream.to_thread();' if 'start' not in sig else '\n\nauto out = stream.to_thread(0, 2);')
        if n in {'as_series','as_note','as_timeline','as_timeindex'}:
            if 'note' in n: return stream_note() + f'\n\nauto out = stream.{n}();'
            if 'timeline' in n: return stream_time() + f'\n\nauto out = stream.{n}();'
            if 'timeindex' in n: return 'auto stream = pluss::kindle::Stream(pluss::make_timeindex({"2024-01-01T09:10:00Z", "2024-01-01T09:40:00Z"}));\n\nauto out = stream.as_timeindex();'
            return base + f'\n\nauto out = stream.{n}();'
        if n == 'show':
            return base + '\n\nstream.show(std::cout);'
        if n == 'from_thread':
            return thread_series() + '\n\nauto out = pluss::kindle::Stream::from_thread(thread);'
        return base + f'\n\nauto out = stream.{n}();'

    if class_name == 'Catalog':
        base = catalog_base()
        if n.startswith('load') or n == 'from_json':
            if n == 'load':
                return f'auto out = pluss::kindle::Catalog::load("csv", "{GENERIC_CSV_PATH}");'
            if n == 'load_csv_with_key':
                return f'auto out = pluss::kindle::Catalog::load_csv_with_key("{GENERIC_CSV_PATH}", "id");'
            if n == 'from_json':
                return 'auto out = pluss::kindle::Catalog::from_json(R"([{"id":1,"city":"Accra"},{"id":2,"city":"Tema"}])");'
            if 'ReadCsvOptions' in sig:
                return f'pluss::loader::ReadCsvOptions options;\nauto out = pluss::kindle::Catalog::load_csv("{GENERIC_CSV_PATH}", options);'
            if 'ReadExcelOptions' in sig:
                return f'pluss::loader::ReadExcelOptions options;\nauto out = pluss::kindle::Catalog::load_excel("{GENERIC_XLSX_PATH}", options);'
            if 'Config' in sig:
                path = GENERIC_CSV_PATH
                if 'excel' in n:
                    path = GENERIC_XLSX_PATH
                elif 'json' in n:
                    path = GENERIC_JSON_PATH
                elif 'parquet' in n:
                    path = GENERIC_PARQUET_PATH
                elif 'txt' in n:
                    path = GENERIC_TXT_PATH
                return f'auto out = pluss::kindle::Catalog::{n}("{path}", {{"nrows", 16}});'
            path = GENERIC_CSV_PATH
            if 'excel' in n:
                path = GENERIC_XLSX_PATH
            elif 'json' in n:
                path = GENERIC_JSON_PATH
            elif 'parquet' in n:
                path = GENERIC_PARQUET_PATH
            elif 'txt' in n:
                path = GENERIC_TXT_PATH
            return f'auto out = pluss::kindle::Catalog::{n}("{path}");'
        if n == 'kindelize':
            return df_base() + '\n\nauto out = pluss::kindle::Catalog::kindelize(df);'
        if n in {'defined','unique','nstreams','nfields','empty','keys','streams','count','to_string','to_json'}:
            return base + f'\n\nauto out = catalog.{n}();'
        if n in {'has','has_stream','stream','at','get','get_stream'}:
            return base + f'\n\nauto out = catalog.{n}("metric");'
        if n in {'series','note','timeline','timeindex','head','tail','dropna','ffill','bfill','isnull','notnull','drop','display','info'}:
            if n == 'bfill':
                return (
                    'using namespace pluss::kindle::literals;\n'
                    'auto catalog = pluss::kindle::Catalog({\n'
                    '    "Final Points Grade"_k = pluss::kindle::Stream(pluss::make_series({71.0, 0.0, 88.0, 0.0})).with_validity_mask(pluss::make_series({true, false, true, false}, pluss::codem::ScalarType::Bool)),\n'
                    '    "Midterm 3"_k = pluss::kindle::Stream(pluss::make_series({60.0, 0.0, 75.0, 91.0})).with_validity_mask(pluss::make_series({true, false, true, true}, pluss::codem::ScalarType::Bool)),\n'
                    '    "Name"_k = pluss::kindle::Stream(pluss::make_note({"A", "B", "C", "D"})),\n'
                    '});\n\n'
                    'auto out = catalog.bfill({"Final Points Grade", "Midterm 3"});'
                )
            if n == 'dropna':
                return (
                    'using namespace pluss::kindle::literals;\n'
                    'auto metric = pluss::kindle::Stream(pluss::make_series({10.0, 0.0, 30.0}))\n'
                    '                 .with_validity_mask(pluss::make_series({true, false, true}, pluss::codem::ScalarType::Bool));\n'
                    'auto catalog = pluss::kindle::Catalog({\n'
                    '    "metric"_k = metric,\n'
                    '    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),\n'
                    '});\n\n'
                    'auto out = catalog.dropna({"metric"});'
                )
            if n == 'drop':
                if 'vector<std::string>' in sig:
                    return base + '\n\nstd::vector<std::string> names{"city"};\nauto out = catalog.drop(names);'
                if 'initializer_list' in sig:
                    return base + '\n\nauto out = catalog.drop({"city"});'
                return base + '\n\nauto out = catalog.drop("city");'
            if n == 'display' or n == 'info':
                return base + f'\n\ncatalog.{n}();'
            if n == 'head': return base + '\n\nauto out = catalog.head(2);'
            if n == 'tail': return base + '\n\nauto out = catalog.tail(2);'
            return base + f'\n\nauto out = catalog.{n}();'
        if n in {'slice','iloc','take','sample','filter','where','query','masked_select','select','rename','rename_','sort','sort_by','group_by','fillna','cast_series','cast_note','cast_timeline','cast_timeindex','astype','sum','mean','min','max','merge','apply','apply_','apply_all','apply_all_','transform','text_preprocess','extract_keywords','compute_similarity','filter_by_text','add_text_features','aggregate_text_stats','tfidf','set_stream','set_stream_','set','set_','set_streams','set_streams_','tabelize','to_table','save_csv','save_txt','save_excel','save_json','save_parquet','save_as','begin','end'}:
            if n == 'slice': return base + '\n\nauto out = catalog.slice(0, 2);'
            if n == 'iloc': return base + ('\n\nauto out = catalog.iloc(1);' if 'start' not in sig else '\n\nauto out = catalog.iloc(0, 2);')
            if n == 'take': return base + '\n\nstd::vector<std::int64_t> picks{0, 2};\nauto out = catalog.take(picks);'
            if n == 'sample': return base + '\n\nauto out = catalog.sample(2, 7);'
            if n == 'filter' or n == 'where' or n == 'masked_select': return base + f'\n\nauto mask = pluss::make_series({{true, false, true}}, pluss::codem::ScalarType::Bool);\nauto out = catalog.{n}(mask);'
            if n == 'query': return base + '\n\nauto out = catalog.query([](const pluss::kindle::Catalog& c) { return c.stream("metric").as_series().gt(15.0); });'
            if n == 'select':
                if 'vector<std::string>' in sig:
                    return base + '\n\nstd::vector<std::string> names{"id", "metric"};\nauto out = catalog.select(names);'
                return base + '\n\nauto out = catalog.select({"id", "metric"});'
            if n == 'rename' or n == 'rename_':
                if 'unordered_map' in sig:
                    return base + '\n\nstd::unordered_map<std::string, std::string> renames{{"metric", "distance_km"}};\nauto out = catalog.rename(renames);'
                if 'initializer_list' in sig:
                    return base + '\n\nauto out = catalog.rename({{"metric", "distance_km"}});'
                return base + ('\n\nauto out = catalog.rename("metric", "distance_km");' if not n.endswith('_') else '\n\ncatalog.rename_("metric", "distance_km");')
            if n == 'sort': return base + '\n\nauto out = catalog.sort("metric", false);'
            if n == 'sort_by': return base + '\n\nauto out = catalog.sort_by({"city", "id"}, {true, false});'
            if n == 'group_by': return base + '\n\nauto out = catalog.group_by("city");'
            if n == 'fillna':
                if 'std::string_view' in sig or 'const char*' in sig:
                    return 'using namespace pluss::kindle::literals;\nauto catalog = pluss::kindle::Catalog({"city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"}))});\n\nauto out = catalog.fillna("Unknown", {"city"});'
                if 'Timeline' in sig:
                    return 'using namespace pluss::kindle::literals;\nauto catalog = pluss::kindle::Catalog({"ts"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-01-01T09:10:00Z", "2024-01-01T09:40:00Z"}))});\n\nauto out = catalog.fillna(pluss::make_timeline("2024-01-01T00:00:00Z"), {"ts"});'
                if 'TimeIndex' in sig:
                    return 'using namespace pluss::kindle::literals;\nauto catalog = pluss::kindle::Catalog({"idx"_k = pluss::kindle::Stream(pluss::make_timeindex({"2024-01-01T09:10:00Z", "2024-01-01T09:40:00Z"}))});\n\nauto out = catalog.fillna(pluss::make_timeindex("2024-01-01T00:00:00Z"), {"idx"});'
                return base + '\n\nauto out = catalog.fillna(0.0, {"metric"});'
            if n.startswith('cast_') or n == 'astype':
                if n == 'cast_series':
                    return 'using namespace pluss::kindle::literals;\nauto catalog = pluss::kindle::Catalog({"metric"_k = pluss::kindle::Stream(pluss::make_note({"10.0", "20.0", "30.0"}))});\n\nauto out = catalog.cast_series({"metric"}, pluss::codem::ScalarType::Float64);'
                return base + f'\n\nauto out = catalog.{n}({{"metric"}}' + (', pluss::codem::ScalarType::Float64' if n == 'astype' else '') + ');'
            if n in {'sum','mean','min','max'}: return base + f'\n\nauto out = catalog.{n}("metric");'
            if n == 'count': return base + '\n\nauto out = catalog.count();'
            if n == 'merge':
                return base + '\n\nusing namespace pluss::kindle::literals;\nauto right = pluss::kindle::Catalog({"id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})), "score"_k = pluss::kindle::Stream(pluss::make_series({0.9, 0.7, 0.8}))});\nauto out = catalog.merge(right, "id", "inner");'
            if n in {'apply','apply_'}:
                return base + ('\n\nauto out = catalog.apply("metric", [](const pluss::kindle::Stream& s) { return s.clip(0.0, 25.0); });' if not n.endswith('_') else '\n\ncatalog.apply_("metric", [](const pluss::kindle::Stream& s) { return s.clip(0.0, 25.0); });')
            if n in {'apply_all','apply_all_','transform'}:
                if n == 'transform' and 'std::string_view field' in sig:
                    return base + '\n\nauto out = catalog.transform("metric", [](const pluss::kindle::Stream& s) { return s.clip(0.0, 25.0); });'
                if n == 'transform':
                    return base + '\n\nauto out = catalog.transform([](const pluss::kindle::Stream& s) { return s.kind() == pluss::kindle::StreamKind::Series ? s.clip(0.0, 25.0) : s; });'
                return base + (f'\n\nauto out = catalog.{n}([](const pluss::kindle::Stream& s) {{ return s.kind() == pluss::kindle::StreamKind::Series ? s.clip(0.0, 25.0) : s; }});' if not n.endswith('_') else f'\n\ncatalog.{n}([](const pluss::kindle::Stream& s) {{ return s.kind() == pluss::kindle::StreamKind::Series ? s.clip(0.0, 25.0) : s; }});')
            if n in {'text_preprocess','extract_keywords','compute_similarity','filter_by_text','add_text_features','aggregate_text_stats','tfidf'}:
                if n == 'compute_similarity': return catalog_text() + '\n\nauto out = catalog.compute_similarity("text", "text", "cosine");'
                if n == 'extract_keywords': return catalog_text() + '\n\nauto out = catalog.extract_keywords("text", 5);'
                if n == 'filter_by_text': return catalog_text() + '\n\nauto out = catalog.filter_by_text("text", "rain");'
                if n == 'aggregate_text_stats': return catalog_text() + '\n\nauto out = catalog.aggregate_text_stats("text", "mean");'
                return catalog_text() + f'\n\nauto out = catalog.{n}("text");'
            if n in {'set_stream','set_stream_','set','set_'}:
                return base + ('\n\nauto out = catalog.set_stream("flag", pluss::kindle::Stream(pluss::make_series({1, 0, 1})));' if not n.endswith('_') else '\n\ncatalog.set_stream_("flag", pluss::kindle::Stream(pluss::make_series({1, 0, 1})));')
            if n in {'set_streams','set_streams_'}:
                if 'NamedStream' in sig:
                    return base + (
                        '\n\nusing namespace pluss::kindle::literals;\n'
                        'auto out = catalog.set_streams({\n'
                        '    "flag"_k = pluss::kindle::Stream(pluss::make_series({1, 0, 1})),\n'
                        '});'
                        if not n.endswith('_') else
                        '\n\nusing namespace pluss::kindle::literals;\n'
                        'catalog.set_streams_({\n'
                        '    "flag"_k = pluss::kindle::Stream(pluss::make_series({1, 0, 1})),\n'
                        '});'
                    )
                return base + (
                    '\n\nauto out = catalog.set_streams({\n'
                    '    {"flag", pluss::kindle::Stream(pluss::make_series({1, 0, 1}))},\n'
                    '});'
                    if not n.endswith('_') else
                    '\n\ncatalog.set_streams_({\n'
                    '    {"flag", pluss::kindle::Stream(pluss::make_series({1, 0, 1}))},\n'
                    '});'
                )
            if n == 'tabelize' or n == 'to_table':
                return base + f'\n\npluss::kindle::TabelizePolicy policy;\npolicy.align = pluss::kindle::AlignPolicy::Inner;\nauto out = catalog.{n}(policy' + (', true' if n == 'to_table' else '') + ');'
            if n.startswith('save_') or n == 'save_as':
                target = {'save_csv':f'"{GENERIC_OUTPUT_CSV_PATH}"','save_txt':f'"{GENERIC_OUTPUT_TXT_PATH}"','save_excel':f'"{GENERIC_OUTPUT_XLSX_PATH}"','save_json':f'"{GENERIC_OUTPUT_JSON_PATH}"','save_parquet':f'"{GENERIC_OUTPUT_PARQUET_PATH}"','save_as':f'"json", "{GENERIC_OUTPUT_JSON_PATH}"'}[n]
                return base + f'\n\ncatalog.{n}({target});'
            if n in {'begin','end'}:
                return base + '\n\nfor (const auto& key : catalog) {\n  (void)key;\n}'
        return base + f'\n\nauto out = catalog.{n}();'

    if class_name == 'Archive':
        base = archive_base()
        if n in {'defined','unique','depth','empty','depth_labels','catalogs','to_string'}:
            return base + f'\n\nauto out = archive.{n}();'
        if n in {'catalog','iloc','loc','dloc','head','tail','slice_depth','sort_depth','relabel_depth','align_depth','map_catalogs','aggregate_over_catalogs','tabelize','to_table','show'}:
            if n == 'catalog' or n == 'iloc': return base + f'\n\nauto out = archive.{n}(0);'
            if n == 'dloc': return base + '\n\nauto out = archive.dloc(1);'
            if n == 'loc': return base + '\n\nauto out = archive.loc("test");'
            if n == 'head': return base + '\n\nauto out = archive.head(1);'
            if n == 'tail': return base + '\n\nauto out = archive.tail(1);'
            if n == 'slice_depth': return base + '\n\nauto out = archive.slice_depth(0, 1);'
            if n == 'sort_depth': return base + '\n\nauto out = archive.sort_depth(true, true);'
            if n == 'relabel_depth' or n == 'align_depth': return base + f'\n\nauto labels = pluss::kindle::Stream(pluss::make_note({{"baseline", "stress"}}));\nauto out = archive.{n}(labels);'
            if n == 'map_catalogs': return base + '\n\nauto out = archive.map_catalogs([](const pluss::kindle::Catalog& catalog) { return catalog.head(1); });'
            if n == 'aggregate_over_catalogs': return base + '\n\nauto out = archive.aggregate_over_catalogs([](const std::vector<pluss::kindle::Catalog>& catalogs) { return catalogs.front(); });'
            if n == 'tabelize' or n == 'to_table': return base + f'\n\npluss::kindle::TabelizePolicy policy;\npolicy.align = pluss::kindle::AlignPolicy::Inner;\nauto out = archive.{n}(policy' + (', true' if n == 'to_table' else '') + ');'
            if n == 'show': return base + '\n\narchive.show(std::cout);'
        if n == 'kindelize':
            return atlas_base() + '\n\nauto out = pluss::kindle::Archive::kindelize(atlas);'
        return base + f'\n\nauto out = archive.{n}();'

    if class_name == 'Slot':
        base = slot_base()
        if n in {'defined','unique','size','empty','values','to_string','to_json'}:
            return base + f'\n\nauto out = slot.{n}();'
        if n == 'at':
            return base + '\n\nauto out = slot.at(0);'
        if n in {'push','push_','head','tail','slice','take','filter','transform','concat','type_counts','to_kindle','as_kindle','kindelize','show'}:
            if n == 'push': return base + '\n\nauto out = slot.push(pluss::make_note("tail"));'
            if n == 'push_': return base + '\n\nslot.push_(pluss::make_note("tail"));'
            if n == 'head': return base + '\n\nauto out = slot.head(2);'
            if n == 'tail': return base + '\n\nauto out = slot.tail(2);'
            if n == 'slice': return base + '\n\nauto out = slot.slice(0, 2);'
            if n == 'take': return base + '\n\nstd::vector<std::int64_t> picks{0, 2};\nauto out = slot.take(picks);'
            if n == 'filter': return slot_mixed() + '\n\nauto out = slot.filter([](const pluss::bundle::Slot::Value& value) { return std::holds_alternative<pluss::Series>(value); });'
            if n == 'transform': return base + '\n\nauto out = slot.transform([](const pluss::bundle::Slot::Value& value) -> pluss::bundle::Slot::Value { if (std::holds_alternative<pluss::Series>(value)) return std::get<pluss::Series>(value).add(10); return value; });'
            if n == 'concat': return base + '\n\nauto other = pluss::bundle::Slot({pluss::make_series(4), pluss::make_series(5)});\nauto out = slot.concat(other);'
            if n == 'type_counts': return slot_mixed() + '\n\nauto out = slot.type_counts();'
            if n in {'to_kindle','as_kindle','kindelize'}: return base + f'\n\nauto out = slot.{n}();'
            if n == 'show': return base + '\n\nslot.show(std::cout);'
        if n == 'from_kindle':
            return stream_series() + '\n\nauto out = pluss::bundle::Slot::from_kindle(stream);'
        return base + f'\n\nauto out = slot.{n}();'

    if class_name == 'Registry':
        base = registry_base()
        if n.startswith('load'):
            if n == 'load': return f'auto out = pluss::bundle::Registry::load("csv", "{GENERIC_CSV_PATH}");'
            if 'ReadCsvOptions' in sig: return f'pluss::loader::ReadCsvOptions options;\nauto out = pluss::bundle::Registry::load_csv("{GENERIC_CSV_PATH}", options);'
            if 'ReadExcelOptions' in sig: return f'pluss::loader::ReadExcelOptions options;\nauto out = pluss::bundle::Registry::load_excel("{GENERIC_XLSX_PATH}", options);'
            if 'Config' in sig:
                path = GENERIC_CSV_PATH
                if 'excel' in n:
                    path = GENERIC_XLSX_PATH
                elif 'json' in n:
                    path = GENERIC_JSON_PATH
                elif 'parquet' in n:
                    path = GENERIC_PARQUET_PATH
                elif 'txt' in n:
                    path = GENERIC_TXT_PATH
                return f'auto out = pluss::bundle::Registry::{n}("{path}", {{"nrows", 16}});'
            path = GENERIC_CSV_PATH
            if 'excel' in n:
                path = GENERIC_XLSX_PATH
            elif 'json' in n:
                path = GENERIC_JSON_PATH
            elif 'parquet' in n:
                path = GENERIC_PARQUET_PATH
            elif 'txt' in n:
                path = GENERIC_TXT_PATH
            return f'auto out = pluss::bundle::Registry::{n}("{path}");'
        if n in {'defined','unique','nslots','nfields','empty','keys','field_names','slots','is_stream_compatible','slot_sizes','type_profiles','to_string','to_json'}:
            return base + f'\n\nauto out = registry.{n}();'
        if n in {'has','has_slot','slot','at','get','get_slot','select','drop','head','tail','slice','iloc','take','sample','rename','rename_','apply','apply_','apply_all','map','map_','combine','clear','clear_','drop_empty_slots','drop_empty_slots_','set_slot','set_slot_','set_slots','set_slots_','push','push_','push_all','push_all_','filter_values','filter_values_','transform_values','transform_values_','take_values','take_values_','concat_values','concat_values_','to_kindle','as_kindle','kindelize','save_csv','save_txt','save_excel','save_json','save_parquet','save_as','show','begin','end'}:
            if n in {'has','has_slot','slot','at','get','get_slot'}: return base + f'\n\nauto out = registry.{n}("ids");'
            if n == 'select':
                if 'std::string_view key' in sig: return base + '\n\nauto out = registry.select("ids");'
                if 'vector<std::string>' in sig: return base + '\n\nstd::vector<std::string> names{"ids"};\nauto out = registry.select(names);'
                return base + '\n\nauto out = registry.select({"ids"});'
            if n == 'drop':
                if 'std::string_view key' in sig: return base + '\n\nauto out = registry.drop("city");'
                if 'vector<std::string>' in sig: return base + '\n\nstd::vector<std::string> names{"city"};\nauto out = registry.drop(names);'
                return base + '\n\nauto out = registry.drop({"city"});'
            if n == 'head': return base + '\n\nauto out = registry.head(2);'
            if n == 'tail': return base + '\n\nauto out = registry.tail(2);'
            if n == 'slice': return base + '\n\nauto out = registry.slice(0, 2);'
            if n == 'iloc': return base + ('\n\nauto out = registry.iloc(1);' if 'start' not in sig else '\n\nauto out = registry.iloc(0, 2);')
            if n == 'take': return base + '\n\nstd::vector<std::int64_t> picks{0, 2};\nauto out = registry.take(picks);'
            if n == 'sample': return base + '\n\nauto out = registry.sample(2, 7);'
            if n == 'rename' or n == 'rename_':
                if 'unordered_map' in sig: return base + '\n\nstd::unordered_map<std::string, std::string> renames{{"ids", "ids_new"}};\nauto out = registry.rename(renames);'
                if 'initializer_list' in sig: return base + '\n\nauto out = registry.rename({{"ids", "ids_new"}});'
                return base + ('\n\nauto out = registry.rename("ids", "ids_new");' if not n.endswith('_') else '\n\nregistry.rename_("ids", "ids_new");')
            if n == 'apply' or n == 'apply_':
                if 'initializer_list<std::string_view> keys' in sig:
                    return base + (f'\n\nauto out = registry.{n}({{"ids"}}, [](const pluss::bundle::Slot& slot) {{ return slot; }});' if not n.endswith('_') else '\n\nregistry.apply_({"ids"}, [](const pluss::bundle::Slot& slot) { return slot; });')
                return base + (f'\n\nauto out = registry.{n}("ids", [](const pluss::bundle::Slot& slot) {{ return slot; }});' if not n.endswith('_') else '\n\nregistry.apply_("ids", [](const pluss::bundle::Slot& slot) { return slot; });')
            if n in {'apply_all','map'}: return base + f'\n\nauto out = registry.{n}([](const pluss::bundle::Slot& slot) {{ return slot; }});'
            if n in {'map_','apply_all_'}: return base + f'\n\nregistry.{n}([](const pluss::bundle::Slot& slot) {{ return slot; }});'
            if n == 'combine': return base + '\n\nauto other = pluss::bundle::Registry({{"flag", pluss::bundle::Slot({pluss::make_series(true, pluss::codem::ScalarType::Bool), pluss::make_series(false, pluss::codem::ScalarType::Bool), pluss::make_series(true, pluss::codem::ScalarType::Bool)})}});\nauto out = registry.combine(other, "_rhs");'
            if n == 'clear' or n == 'clear_': return base + ('\n\nauto out = registry.clear();' if not n.endswith('_') else '\n\nregistry.clear_();')
            if n == 'drop_empty_slots':
                return (
                    'auto registry = pluss::bundle::Registry({\n'
                    '    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},\n'
                    '    {"empty", pluss::bundle::Slot({})},\n'
                    '    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},\n'
                    '});\n\n'
                    'auto out = registry.drop_empty_slots();'
                )
            if n == 'drop_empty_slots_':
                return (
                    'auto registry = pluss::bundle::Registry({\n'
                    '    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},\n'
                    '    {"empty", pluss::bundle::Slot({})},\n'
                    '    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},\n'
                    '});\n\n'
                    'registry.drop_empty_slots_();'
                )
            if n in {'set_slot','set_slot_'}: return base + ('\n\nauto out = registry.set_slot("score", pluss::bundle::Slot({pluss::make_series(0.9), pluss::make_series(0.7), pluss::make_series(0.8)}));' if not n.endswith('_') else '\n\nregistry.set_slot_("score", pluss::bundle::Slot({pluss::make_series(0.9), pluss::make_series(0.7), pluss::make_series(0.8)}));')
            if n in {'set_slots','set_slots_'}:
                return base + (
                    '\n\nauto out = registry.set_slots({\n'
                    '    {"score", pluss::bundle::Slot({pluss::make_series(0.9), pluss::make_series(0.7), pluss::make_series(0.8)})},\n'
                    '});'
                    if not n.endswith('_') else
                    '\n\nregistry.set_slots_({\n'
                    '    {"score", pluss::bundle::Slot({pluss::make_series(0.9), pluss::make_series(0.7), pluss::make_series(0.8)})},\n'
                    '});'
                )
            if n in {'push','push_'}: return base + ('\n\nauto out = registry.push("ids", pluss::make_series(4));' if not n.endswith('_') else '\n\nregistry.push_("ids", pluss::make_series(4));')
            if n in {'push_all','push_all_'}: return base + ('\n\nauto out = registry.push_all(pluss::make_note("ok"));' if not n.endswith('_') else '\n\nregistry.push_all_(pluss::make_note("ok"));')
            if n in {'filter_values','filter_values_'}: return base + ('\n\nauto out = registry.filter_values("ids", [](const pluss::bundle::Slot::Value& value) { return std::holds_alternative<pluss::Series>(value); });' if not n.endswith('_') else '\n\nregistry.filter_values_("ids", [](const pluss::bundle::Slot::Value& value) { return std::holds_alternative<pluss::Series>(value); });')
            if n in {'transform_values','transform_values_'}: return base + ('\n\nauto out = registry.transform_values("ids", [](const pluss::bundle::Slot::Value& value) -> pluss::bundle::Slot::Value { if (std::holds_alternative<pluss::Series>(value)) return std::get<pluss::Series>(value).add(10); return value; });' if not n.endswith('_') else '\n\nregistry.transform_values_("ids", [](const pluss::bundle::Slot::Value& value) -> pluss::bundle::Slot::Value { if (std::holds_alternative<pluss::Series>(value)) return std::get<pluss::Series>(value).add(10); return value; });')
            if n in {'take_values','take_values_'}: return base + ('\n\nstd::vector<std::int64_t> picks{0, 2};\nauto out = registry.take_values("ids", picks);' if not n.endswith('_') else '\n\nstd::vector<std::int64_t> picks{0, 2};\nregistry.take_values_("ids", picks);')
            if n in {'concat_values','concat_values_'}: return base + ('\n\nauto extra = pluss::bundle::Slot({pluss::make_series(4), pluss::make_series(5)});\nauto out = registry.concat_values("ids", extra);' if not n.endswith('_') else '\n\nauto extra = pluss::bundle::Slot({pluss::make_series(4), pluss::make_series(5)});\nregistry.concat_values_("ids", extra);')
            if n in {'to_kindle','as_kindle','kindelize'}: return base + f'\n\nauto out = registry.{n}();'
            if n.startswith('save_') or n == 'save_as':
                target = {'save_csv':f'"{GENERIC_OUTPUT_CSV_PATH}"','save_txt':f'"{GENERIC_OUTPUT_TXT_PATH}"','save_excel':f'"{GENERIC_OUTPUT_XLSX_PATH}"','save_json':f'"{GENERIC_OUTPUT_JSON_PATH}"','save_parquet':f'"{GENERIC_OUTPUT_PARQUET_PATH}"','save_as':f'"json", "{GENERIC_OUTPUT_JSON_PATH}"'}[n]
                return base + f'\n\nregistry.{n}({target});'
            if n == 'show': return base + '\n\nregistry.show(std::cout);'
            if n in {'begin','end'}: return base + '\n\nfor (const auto& key : registry) {\n  (void)key;\n}'
        if n == 'ensure_stream_compatible':
            return base + '\n\nregistry.ensure_stream_compatible("demo");'
        if n == 'from_kindle':
            return catalog_base() + '\n\nauto out = pluss::bundle::Registry::from_kindle(catalog);'
        if n in {'has_all','has_any'}:
            return base + f'\n\nauto out = registry.{n}({{"ids", "city"}});'
        return base + f'\n\nauto out = registry.{n}();'

    if class_name == 'Aisle':
        base = aisle_base()
        if n in {'defined','unique','depth','empty','registries','labels','to_string','to_json'}:
            return base + f'\n\nauto out = aisle.{n}();'
        if n in {'registry','head','tail','slice','map','select','apply_slot','concat_depth','to_kindle','as_kindle','kindelize','show'}:
            if n == 'registry': return base + '\n\nauto out = aisle.registry(0);'
            if n == 'head': return base + '\n\nauto out = aisle.head(1);'
            if n == 'tail': return base + '\n\nauto out = aisle.tail(1);'
            if n == 'slice': return base + '\n\nauto out = aisle.slice(0, 1);'
            if n == 'map': return base + '\n\nauto out = aisle.map([](const pluss::bundle::Registry& registry) { return registry; });'
            if n == 'select': return base + '\n\nauto out = aisle.select({"ids"});'
            if n == 'apply_slot': return base + '\n\nauto out = aisle.apply_slot("ids", [](const pluss::bundle::Slot& slot) { return slot; });'
            if n == 'concat_depth': return base + '\n\nauto out = aisle.concat_depth(aisle);'
            if n in {'to_kindle','as_kindle','kindelize'}: return base + f'\n\nauto out = aisle.{n}();'
            if n == 'show': return base + '\n\naisle.show(std::cout);'
        if n == 'from_kindle':
            return archive_base() + '\n\nauto out = pluss::bundle::Aisle::from_kindle(archive);'
        return base + f'\n\nauto out = aisle.{n}();'

    raise RuntimeError(f'Unhandled class {class_name}')


def build_page(class_name: str, methods: list[Method]) -> str:
    def extra_sections_for_method(method: Method) -> list[str]:
        n = method.name
        sig = method.signature
        def bullets(title: str, items: list[str]) -> list[str]:
            return [f'#### {title}', '', *[f'- {item}' for item in items], '']
        if class_name == 'DataFrame' and n == 'bfill':
            return [
                '#### Practical Behavior',
                '',
                '- `bfill()` runs backward fill feature-by-feature across the full frame.',
                '- A null value is replaced by the next valid value below it in the same feature.',
                '- Trailing null runs stay null because there is no later value to pull from.',
                '- If you want to backward-fill only one feature, apply it at the `Thread` level and assign it back with `set(...)` or `set_features(...)`.',
                '',
                '#### Common Pattern',
                '',
                '```cpp',
                'gradebook.set("Final Points Grade") =',
                '    gradebook.features("Final Points Grade").bfill();',
                '```',
                '',
            ]
        if class_name == 'DataFrame' and n == 'bottom':
            return [
                '#### Practical Behavior',
                '',
                '- `bottom(feature, n)` sorts ascending by `feature`, keeps nulls last, then returns the first `n` rows.',
                '- Because nulls are sorted last, null rows are not chosen unless there are not enough non-null rows to satisfy `n`.',
                '- `n == 0` returns an empty frame with the same schema.',
                '- The output keeps the original row payloads and case labels of the selected rows.',
                '',
                '#### Argument Effects',
                '',
                '- Small `n` is useful for “lowest few rows” inspection.',
                '- Larger `n` gives you the full ascending slice without writing `sort_values(...).head(...)` yourself.',
                '',
                '#### Equivalent Expanded Form',
                '',
                '```cpp',
                'auto out = df.sort_values({pluss::table::DataFrame::ArrangeSpec{"sales", true, true}})',
                '             .head(5, false);',
                '```',
                '',
            ]
        if class_name == 'DataFrame' and n == 'aggregate':
            return bullets('Practical Behavior', [
                '`aggregate(...)` builds a compact summary frame instead of returning grouped rows.',
                'Each `SummarySpec` chooses an output name, a source feature, and a summary operation.',
                'Use this when you want one-row summary output such as totals, means, mins, maxes, or counts.',
            ])
        if class_name == 'DataFrame' and n == 'align_cases':
            return bullets('Practical Behavior', [
                '`align_cases(new_cases)` reorders rows by matching the existing case labels against `new_cases`.',
                'Every requested case label must already exist or the call throws.',
                'The output case axis is rewritten to `new_cases`, so both the row order and the labels now follow that target order.',
            ])
        if class_name == 'DataFrame' and n in {'all', 'any'}:
            return bullets('Axis Rules', [
                '`axis=0` reduces down the selected boolean features and returns one result per feature.',
                '`axis=1` reduces across features and returns one result per row.',
                'These methods expect boolean features; they are not general numeric reducers.',
                'Nullable boolean input requires `skipna=true`.',
            ])
        if class_name == 'DataFrame' and n == 'assign':
            return bullets('Practical Behavior', [
                '`assign(...)` adds new derived features or replaces existing ones without mutating the source frame.',
                'Each derived thread must match the frame case count.',
                'Use the pair form when you already have the computed thread, and the spec form when the value should be computed from the current frame inside the call.',
            ])
        if class_name == 'DataFrame' and n in {'astype', 'cast_series', 'cast_note', 'cast_timeline', 'cast_timeindex', 'cast'}:
            return bullets('Conversion Rules', [
                'These methods rewrite only the selected features; unselected features are passed through unchanged.',
                'Duplicate feature names in the request are ignored after the first occurrence.',
                'Unknown or empty feature names throw immediately.',
                '`astype(...)` is the direct numeric-series dtype conversion path, while `cast(...)` is the broader target-selection route.',
            ])
        if class_name == 'DataFrame' and n == 'at':
            return bullets('Practical Behavior', [
                '`at(row, feature)` returns one scalar value rendered as text.',
                'The row is positional here; it does not look up by case label.',
                'Use this for quick single-cell inspection, not for typed column work.',
            ])
        if class_name == 'DataFrame' and n == 'cases':
            return bullets('Overload Guide', [
                '`cases(row)` and `cases(start, stop)` return row slices.',
                '`cases(row, feature)` returns one scalar value from a row/feature intersection.',
                'Use `cases_series()` only when the case axis is actually Series-backed.',
            ])
        if class_name == 'DataFrame' and n == 'ceil_time':
            return bullets('Practical Behavior', [
                '`ceil_time(...)` rounds selected temporal features upward to the requested unit.',
                'Only timeline/timeindex features are valid targets.',
                'Use this when you want time-bucket labels that always round toward the next boundary.',
            ])
        if class_name == 'DataFrame' and n == 'cloc':
            return bullets('Practical Behavior', [
                '`cloc(...)` is the case-axis row slice helper.',
                'It is equivalent to slicing rows, not selecting by feature names.',
                'Use it when you want row-position access in a method that reads like case-location.',
            ])
        if class_name == 'DataFrame' and n == 'combine':
            return bullets('Practical Behavior', [
                '`combine(...)` is the lower-level composition route behind feature-wise join/update behaviors.',
                'Collision handling depends on the chosen `CombineSpec` and suffix settings.',
                'Use the dedicated helpers such as `join(...)`, `update(...)`, or `unify(...)` when they fit; use `combine(...)` when you need explicit combine policy control.',
            ])
        if class_name == 'DataFrame' and n == 'compare':
            return bullets('Practical Behavior', [
                '`compare(...)` is the “not equal” comparison alias on `DataFrame`.',
                'It returns a boolean-like frame you can inspect directly or reuse as a mask.',
                'Use `eq/ne/lt/le/gt/ge` when you want the comparison intent spelled out directly.',
            ])
        if class_name == 'DataFrame' and n == 'conform_to':
            return bullets('Practical Behavior', [
                '`conform_to(other)` first reorders features to match `other.feature_names()` and then aligns the case axis to `other.cases()`.',
                'Both frames must have the same feature count and the same feature name set.',
                'Use this before arithmetic or updates when structure compatibility matters more than preserving the current order.',
            ])
        if class_name == 'DataFrame' and n == 'copy':
            return bullets('Practical Behavior', [
                '`copy()` gives you a detached frame value for downstream edits.',
                'Use it when you want to make the independence of the result explicit in user code or examples.',
            ])
        if class_name == 'DataFrame' and n == 'corr':
            return bullets('Practical Behavior', [
                '`corr(...)` builds a correlation matrix across numeric features only.',
                'If you do not pass a feature list, the method auto-selects numeric features from the frame.',
                'Supported methods route through thread-level correlation kernels such as Pearson, Spearman, and Kendall Tau.',
            ])
        if class_name == 'DataFrame' and n == 'count':
            return bullets('Practical Behavior', [
                '`count()` returns per-feature non-null counts as a summary frame.',
                'This is not row count; use `nrows()` when you want total case count.',
            ])
        if class_name == 'DataFrame' and n == 'count_by':
            return bullets('Practical Behavior', [
                '`count_by(feature, ...)` groups by one feature, counts rows per distinct value, sorts by count descending, then trims to `top_n` when requested.',
                'Ties are broken by key name in ascending order.',
            ])
        if class_name == 'DataFrame' and n == 'count_distinct':
            return bullets('Practical Behavior', [
                '`count_distinct(...)` returns one summary row containing distinct-value counts for the selected features.',
                'With no feature list, every feature is included.',
                '`dropna` controls whether null should participate as its own distinct value.',
            ])
        if class_name == 'DataFrame' and n == 'count_null_rows':
            return bullets('Practical Behavior', [
                '`count_null_rows()` counts how many rows contain at least one null value anywhere in the frame.',
                'Use this as a quick completeness check before grouping, joining, or export.',
            ])
        if class_name == 'DataFrame' and n == 'describe':
            return bullets('Practical Behavior', [
                '`describe()` summarizes numeric series features only.',
                'Non-numeric and non-series features are skipped rather than coerced.',
                'Null values are removed before the statistics are computed.',
            ])
        if class_name == 'DataFrame' and n == 'drop':
            return bullets('Practical Behavior', [
                '`drop(...)` removes features, not rows.',
                'The remaining features keep their existing row alignment and case axis.',
            ])
        if class_name == 'DataFrame' and n == 'dropna':
            return bullets('Practical Behavior', [
                '`dropna()` removes rows that contain at least one null value anywhere in the frame.',
                'Use `dropna_all(features)` when the rule should apply only to selected features and only when all of them are null.',
            ])
        if class_name == 'DataFrame' and n == 'dropna_all':
            return bullets('Practical Behavior', [
                '`dropna_all(features)` removes rows only when all selected features are null in that row.',
                'Rows with at least one observed value among the selected features are kept.',
            ])
        if class_name == 'Thread' and n == 'bfill':
            return [
                '#### Practical Behavior',
                '',
                '- `bfill()` works within one column.',
                '- Each null entry pulls the next valid value below it.',
                '- Trailing null runs remain null because there is no later value to use.',
                '- Use this when only one feature should be filled instead of the whole frame.',
                '',
            ]
        if class_name == 'Thread' and n in {'as_note', 'as_series', 'as_timeline', 'as_timeindex', 'astype', 'cast'}:
            return bullets('Conversion Rules', [
                'These methods convert or reinterpret the current payload into the requested thread type.',
                'Use `as_*()` when you want access to the underlying pluss-type surface directly.',
                'Use `cast(...)` when you need explicit target selection with broader conversion control.',
                '`astype(...)` is the numeric-series dtype conversion path.',
            ])
        if class_name == 'Thread' and n == 'btw':
            if 'double a, double b' in sig:
                return [
                    '#### Boundary Rules',
                    '',
                    '- This overload is for numeric `Series` payloads only.',
                    '- If `a > b`, the implementation swaps the bounds for you.',
                    '- When bounds are swapped, the left/right inclusivity flags are swapped too, so the semantics stay consistent.',
                    '- Use `inclusive=true` when both ends should be closed, or the four-argument form when each side needs different behavior.',
                    '',
                    '#### Common Pattern',
                    '',
                    '```cpp',
                    'auto passing = scores.btw(50.0, 100.0, true);',
                    'auto strict_band = scores.btw(50.0, 100.0, false, true);',
                    '```',
                    '',
                ]
            if 'std::string_view lo, std::string_view hi' in sig:
                return [
                    '#### Boundary Rules',
                    '',
                    '- This overload is for `Timeline` and `TimeIndex` payloads.',
                    '- The string bounds are parsed as timeline values.',
                    '- Reversed bounds are accepted; the implementation swaps them before comparison.',
                    '- This is the range-filter form to use for date/time columns.',
                    '',
                    '#### Common Pattern',
                    '',
                    '```cpp',
                    'auto january = ts.btw("2024-01-01T00:00:00Z", "2024-01-31T23:59:59Z", true);',
                    '```',
                    '',
                ]
            if 'const Thread& lo, const Thread& hi' in sig:
                return [
                    '#### Boundary Rules',
                    '',
                    '- This overload is row-wise: each row is compared against its own lower and upper bound.',
                    '- `lo` and `hi` must be defined and must match the size of the current thread.',
                    '- Use this when the allowable range changes per row instead of using one scalar interval for the whole column.',
                    '',
                ]
        if class_name == 'Thread' and n == 'bucketize':
            return [
                '#### Practical Behavior',
                '',
                '- `bucketize(...)` is only valid for numeric `Series` payloads.',
                '- `bins` must contain at least two edges, must not contain `NaN`, and must be strictly increasing.',
                '- If `labels` is empty, the method uses `0, 1, 2, ...` for the bucket labels.',
                '- If `labels` is provided, its size must be exactly `bins.size() - 1`.',
                '- Values that do not land in any interval remain invalid in the output rather than being forced into a bucket.',
                '',
                '#### Boundary Rules',
                '',
                '- With `right=true`, intervals are right-closed by default: `(a, b]`.',
                '- With `right=false`, intervals are left-closed and right-open: `[a, b)`.',
                '- `include_lowest=true` makes the first bucket include its left edge even when `right=true`.',
                '',
            ]
        if class_name == 'Thread' and n == 'clear_validity_mask':
            return bullets('Practical Behavior', [
                '`clear_validity_mask()` keeps the payload values but drops null-tracking, so every entry becomes valid again.',
                'Use this when null semantics should be discarded rather than filled.',
            ])
        if class_name == 'Thread' and n == 'clip':
            return bullets('Practical Behavior', [
                '`clip(min, max)` clamps numeric series values into the closed interval `[min, max]`.',
                'The validity mask is preserved.',
            ])
        if class_name == 'Thread' and n == 'compare':
            return bullets('Comparison Rules', [
                'Scalar/native comparisons broadcast the right-hand value across the whole thread.',
                'Thread-vs-thread comparisons are row-wise and require compatible sizes.',
                'Text and temporal comparisons route through the payload-specific kernels rather than forcing everything through numeric conversion.',
            ])
        if class_name == 'Thread' and n == 'contains':
            return bullets('Practical Behavior', [
                '`contains(needle)` is only valid for note-backed threads.',
                'It returns a boolean series telling you which text entries contain the requested substring.',
            ])
        if class_name == 'Thread' and n == 'corr':
            return bullets('Practical Behavior', [
                '`corr(other, method)` requires both threads to be numeric series of the same size.',
                'Invalid, null, or non-finite pairs are removed before the statistic is computed.',
                'If too few valid paired values remain, the result is `NaN`.',
            ])
        if class_name == 'Thread' and n == 'count':
            return bullets('Practical Behavior', [
                'For numeric/text/temporal threads, `count(true)` returns the number of valid entries.',
                'For boolean series, the underlying series count kernel is used.',
                '`count(false)` is only meaningful on boolean series payloads.',
            ])
        if class_name == 'Thread' and n == 'day':
            return bullets('Practical Behavior', [
                '`day()` is valid only for timeline/timeindex payloads.',
                'The result preserves the validity mask while projecting the day component.',
            ])
        if class_name == 'Thread' and n == 'describe':
            return bullets('Practical Behavior', [
                'Numeric series are summarized through the table-style numeric describe path.',
                'Note payloads produce count/unique/top/freq style summaries.',
                'Other payload kinds are not supported by `describe()`.',
            ])
        if class_name == 'Thread' and n in {'display_labels', 'display_name'}:
            return bullets('Practical Behavior', [
                'These accessors expose display metadata attached to the thread.',
                'They do not change the payload values or validity mask.',
            ])
        if class_name == 'Thread' and n == 'distinct':
            return bullets('Practical Behavior', [
                '`distinct(dropna)` keeps the first occurrence of each distinct value in encounter order.',
                'With `dropna=false`, at most one null entry is retained.',
            ])
        if class_name == 'Thread' and n == 'dropna':
            return bullets('Practical Behavior', [
                '`dropna()` removes invalid entries from the thread and compacts the result.',
                'Use this when null rows should disappear rather than be filled.',
            ])
        if class_name == 'Thread' and n == 'dtype_name':
            return bullets('Practical Behavior', [
                '`dtype_name()` reports the scalar dtype used by a numeric series payload.',
                'Use it when debugging type-dependent behavior or conversion steps.',
            ])
        if class_name == 'Stream' and n == 'bfill':
            return [
                '#### Practical Behavior',
                '',
                '- `bfill()` applies thread-level backward fill while preserving the stream case labels.',
                '- Use this when you want the fill behavior of `Thread::bfill()` but you are already operating in Kindle/Stream workflows.',
                '',
            ]
        if class_name == 'Stream' and n in {'as_note', 'as_series', 'as_timeline', 'as_timeindex', 'astype', 'cast'}:
            return bullets('Conversion Rules', [
                'These methods convert the underlying payload while keeping stream-level structure such as cases and metadata in view.',
                '`astype(...)` is the direct numeric-series dtype path.',
            ])
        if class_name == 'Stream' and n in {'drop_meta', 'drop_meta_'}:
            return bullets('Practical Behavior', [
                'These methods remove one metadata key only.',
                'Payload values, cases, and validity remain unchanged.',
            ])
        if class_name == 'Stream' and n == 'dropna':
            return bullets('Practical Behavior', [
                '`dropna()` removes invalid entries from the stream while preserving stream semantics for the remaining entries.',
                'Use it when rows with null payload values should be removed entirely.',
            ])
        if class_name == 'Catalog' and n == 'at':
            return bullets('Practical Behavior', [
                '`at(key)` fetches one named stream from the catalog.',
                'Use it when you want stream access without routing through table conversion first.',
            ])
        if class_name == 'Catalog' and n in {'apply', 'apply_all'}:
            return bullets('Practical Behavior', [
                '`apply(...)` rewrites one named stream; `apply_all(...)` rewrites every stream.',
                'The transform must return a defined stream with the same length as the input stream.',
                'Use the underscore form when you want the mutation in place.',
            ])
        if class_name == 'Catalog' and n == 'astype':
            return bullets('Conversion Rules', [
                '`astype(keys, dtype)` is the catalog-level numeric-series conversion helper.',
                'It delegates to stream/thread conversion on the selected keys only.',
            ])
        if class_name == 'Catalog' and n == 'add_text_features':
            return bullets('Practical Behavior', [
                '`add_text_features(field)` adds `<field>_char_len` and `<field>_word_count` streams.',
                'The source field is coerced through note/text handling before the text statistics are computed.',
            ])
        if class_name == 'Catalog' and n == 'aggregate_text_stats':
            return bullets('Practical Behavior', [
                '`aggregate_text_stats(field, stat_type)` collapses one text field into a one-value summary stream.',
                'Supported statistics include common length-based summaries such as mean/min/max/count, along with vocabulary size.',
            ])
        if class_name == 'Catalog' and n == 'cast_series':
            return bullets('Conversion Rules', [
                'Catalog cast helpers rewrite only the selected keys.',
                'Unknown keys throw immediately.',
            ])
        if class_name == 'Catalog' and n in {'cast_note', 'cast_timeline', 'cast_timeindex'}:
            return bullets('Conversion Rules', [
                'These helpers rewrite only the selected keys and leave all other streams unchanged.',
                'Unknown keys throw immediately.',
            ])
        if class_name == 'Catalog' and n == 'cloc':
            return bullets('Practical Behavior', [
                '`cloc(...)` slices aligned row positions across every stream in the catalog.',
                'Use it when you want row-wise positional access while keeping stream alignment intact.',
            ])
        if class_name == 'Catalog' and n == 'compute_similarity':
            return bullets('Practical Behavior', [
                '`compute_similarity(field1, field2, "cosine")` computes row-wise text similarity and appends the result as a numeric stream.',
                'At the moment the supported similarity method is cosine.',
                'Both fields must have the same length.',
            ])
        if class_name == 'Catalog' and n == 'count':
            return bullets('Practical Behavior', [
                '`count()` returns the catalog row count after alignment through `tabelize()`.',
                'This is the quick “how many aligned rows do I have?” answer.',
            ])
        if class_name == 'Catalog' and n == 'display':
            return bullets('Practical Behavior', [
                '`display()` renders a human-readable catalog view for inspection.',
                'Use it for inspection and debugging rather than transformation.',
            ])
        if class_name == 'Catalog' and n == 'drop':
            return bullets('Practical Behavior', [
                '`drop(...)` removes named streams, not rows.',
                'The remaining streams keep their alignment.',
            ])
        if class_name == 'Catalog' and n == 'dropna':
            return bullets('Practical Behavior', [
                '`dropna(keys)` applies stream-level `dropna()` to the selected keys.',
                'With no keys supplied, every stream is processed.',
                'Unknown keys throw immediately.',
            ])
        if class_name == 'Catalog' and n == 'extract_keywords':
            return bullets('Practical Behavior', [
                '`extract_keywords(field, top_n)` tokenizes the text field, counts tokens, sorts by frequency descending then alphabetically, and returns a small keyword/count catalog.',
                'Use it for quick corpus-level keyword previews rather than row-wise text features.',
            ])
        if class_name == 'Atlas' and n == 'aggregate_over_frames':
            return bullets('Practical Behavior', [
                '`aggregate_over_frames(...)` hands the full frame vector to your reducer and expects one output object back.',
                'Use it when the aggregation depends on multiple frames together rather than frame-by-frame mapping.',
            ])
        if class_name == 'Atlas' and n == 'align_depth':
            return bullets('Practical Behavior', [
                '`align_depth(labels)` reorders frames to match the requested depth label order.',
                'Use it when two atlases should share the same depth layout before comparison or combination.',
            ])
        if class_name == 'Atlas' and n == 'dloc':
            return bullets('Practical Behavior', [
                '`dloc(i)` is the positional depth-access alias for `frame(i)`.',
                'Use it when you want the depth-axis wording to be explicit.',
            ])
        if class_name == 'Archive' and n == 'aggregate_over_catalogs':
            return bullets('Practical Behavior', [
                '`aggregate_over_catalogs(...)` hands the full catalog vector to your reducer and expects one output object back.',
                'Use it when the aggregation depends on multiple catalogs together rather than catalog-by-catalog mapping.',
            ])
        if class_name == 'Archive' and n == 'align_depth':
            return bullets('Practical Behavior', [
                '`align_depth(labels)` reorders catalogs to match the requested depth label order.',
                'Use it when archive depth order must match another archive or reporting layout.',
            ])
        if class_name == 'Archive' and n == 'dloc':
            return bullets('Practical Behavior', [
                '`dloc(i)` is the positional depth-access alias for `catalog(i)`.',
                'Use it when you want the depth-axis wording to be explicit.',
            ])
        if class_name == 'Registry' and n == 'at':
            return bullets('Practical Behavior', [
                '`at(key)` fetches one named slot from the registry.',
                'Use it when you want direct slot access before applying slot-level transforms.',
            ])
        if class_name == 'Registry' and n in {'apply', 'apply_all'}:
            return bullets('Practical Behavior', [
                '`apply(...)` rewrites one slot or a selected slot set; `apply_all(...)` rewrites every slot.',
                'The transform must return a defined slot.',
                'Use the underscore form when you want the mutation in place.',
            ])
        if class_name == 'Registry' and n == 'as_kindle':
            return bullets('Bridge Behavior', [
                '`as_kindle()` converts the registry into the Kindle/Catalog model.',
                'Use this when the next operation is stream-oriented rather than bundle-oriented.',
            ])
        if class_name == 'Registry' and n == 'clear':
            return bullets('Practical Behavior', [
                '`clear()` returns an empty registry with no slots.',
                'Use `clear_()` when you want the same effect in place.',
            ])
        if class_name == 'Registry' and n == 'cloc':
            return bullets('Practical Behavior', [
                '`cloc(...)` slices positional rows across every slot while keeping slot alignment intact.',
            ])
        if class_name == 'Registry' and n == 'combine':
            return bullets('Practical Behavior', [
                '`combine(other, suffix)` appends slots from `other` and renames collisions using the supplied suffix.',
                'If the first suffixed name still collides, numbered suffixes are added until the name is unique.',
            ])
        if class_name == 'Registry' and n == 'concat_values':
            return bullets('Practical Behavior', [
                '`concat_values(key, other)` appends the values from another slot onto one existing slot.',
                'The target key must exist and the appended slot must be defined.',
            ])
        if class_name == 'Registry' and n == 'drop':
            return bullets('Practical Behavior', [
                '`drop(...)` removes named slots, not individual values.',
                'Unknown keys throw immediately.',
            ])
        if class_name == 'Registry' and n in {'drop_empty_slots', 'drop_empty_slots_'}:
            return bullets('Practical Behavior', [
                'These methods remove slots whose payload is empty.',
                'Non-empty slots are passed through unchanged.',
            ])
        if class_name == 'Aisle' and n == 'apply_slot':
            return bullets('Practical Behavior', [
                '`apply_slot(key, fn)` walks every registry in the aisle and rewrites that slot only where it exists.',
                'Registries that do not have the requested key are left unchanged.',
            ])
        if class_name == 'Aisle' and n == 'as_kindle':
            return bullets('Bridge Behavior', [
                '`as_kindle()` converts the aisle into the Archive model.',
                'Use this when the next workflow is archive/catalog based rather than bundle based.',
            ])
        if class_name == 'Aisle' and n == 'concat_depth':
            return bullets('Practical Behavior', [
                '`concat_depth(other)` appends the other aisle’s registries onto the current depth axis.',
                'If both aisles have labels, the labels are concatenated too.',
            ])
        if class_name == 'Catalog' and n == 'bfill':
            return [
                '#### Practical Behavior',
                '',
                '- With no keys supplied, `bfill()` is applied to every stream in the catalog.',
                '- If you pass keys, only those streams are backward-filled; all other streams are left unchanged.',
                '- Unknown keys throw immediately instead of silently skipping.',
                '',
                '#### Common Pattern',
                '',
                '```cpp',
                'auto cleaned = catalog.bfill({"Final Points Grade", "Midterm 3"});',
                '```',
                '',
            ]
        if class_name == 'DataFramePlotAccessor' and n == 'act':
            return bullets('Practical Behavior', [
                '`act(feature, options)` requires one numeric feature.',
                'Use it when you want autocorrelation-style time-lag structure for one series.',
            ])
        if class_name == 'DataFramePlotAccessor' and n == 'andrews_curves':
            return bullets('Practical Behavior', [
                '`andrews_curves(...)` works on numeric features and can optionally color/group by a class feature.',
                'If you do not supply labels in the options, the feature names are used.',
            ])
        if class_name == 'DataFramePlotAccessor' and n == 'area':
            return bullets('Practical Behavior', [
                'One y-feature creates one filled area trace.',
                'Many y-features create one area trace per selected feature over the same x-axis.',
                'If `x_feature` is omitted, the case axis is used.',
            ])
        if class_name == 'DataFramePlotAccessor' and n == 'area_stacked':
            return bullets('Practical Behavior', [
                '`area_stacked(...)` requires at least one y-feature.',
                'Each later y-feature is stacked on top of the running baseline from the earlier ones.',
                'This is useful for contribution-over-total views across the same x positions.',
            ])
        if class_name == 'DataFramePlotAccessor' and n == 'bar':
            return [
                '#### Practical Behavior',
                '',
                '- One `y_feature` creates one bar series.',
                '- Many `y_features` create grouped bars over the same x-axis positions.',
                '- If `x_feature` is omitted, the frame case axis is used as the bar axis.',
                '- `BarPlotOptions::horizontal=true` swaps the plot orientation.',
                '- `BarPlotOptions::ci_enabled=true` aggregates values by axis position and adds confidence-interval whiskers.',
                '',
                '#### Common Patterns',
                '',
                '```cpp',
                'auto one_series = df.plot().bar("sales", "city");',
                'auto grouped = df.plot().bar({"midterm", "final"}, "student");',
                '',
                'pluss::table::BarPlotOptions options;',
                'options.horizontal = true;',
                'auto horizontal = df.plot().bar("sales", "city", options);',
                '```',
                '',
            ]
        if class_name == 'DataFramePlotAccessor' and n == 'bar_stacked':
            return [
                '#### Practical Behavior',
                '',
                '- `bar_stacked(...)` requires at least one y-feature.',
                '- Each later y-feature is stacked on top of the earlier ones at the same x-axis positions.',
                '- This is useful for composition views where the total height and each component both matter.',
                '- If `x_feature` is omitted, the case axis is used.',
                '',
            ]
        if class_name == 'DataFramePlotAccessor' and n == 'box':
            return [
                '#### Practical Behavior',
                '',
                '- `box(...)` accepts numeric features only.',
                '- With no explicit feature list, the accessor resolves the available numeric features for you.',
                '- Use this when you want a distribution summary instead of a per-row plot.',
                '',
            ]
        if class_name == 'DataFramePlotAccessor' and n == 'bootstrap':
            return [
                '#### Practical Behavior',
                '',
                '- `bootstrap(feature, options)` requires one numeric feature.',
                '- The plot is driven by resampling options such as `samples`, `sample_size`, `seed`, and the chosen statistic.',
                '- This is the method to use when you want sampling-based uncertainty or distribution shape around a summary statistic.',
                '',
            ]
        if class_name == 'DataFramePlotAccessor' and n == 'brief':
            if 'Studio& figure' in sig:
                return [
                    '#### Practical Behavior',
                    '',
                    '- This overload writes the summary into an existing figure instead of creating a new one.',
                    '- Use it when you want a compact numeric summary beside or under another plot.',
                    '',
                ]
            return [
                '#### Practical Behavior',
                '',
                '- `brief(...)` builds a compact summary figure for numeric features.',
                '- `BriefPlotOptions` controls table-style presentation details such as `precision`, `max_cases`, and whether headers are shown.',
                '- Use this when you want a readable figure-based summary rather than a chart trace per row.',
                '',
            ]
        if class_name == 'DataFramePlotAccessor' and n == 'corr_heatmap':
            return bullets('Practical Behavior', [
                '`corr_heatmap(...)` selects numeric features, computes their correlation matrix, and renders it as a heatmap.',
                'If you omit the feature list, the accessor resolves the numeric feature set for you.',
            ])
        if class_name == 'DataFramePlotAccessor' and n == 'auto_plot':
            return bullets('Selection Rules', [
                '`auto_plot(...)` chooses a plot family based on feature count, row count, case labels, and sign patterns.',
                'Examples include bar for one labeled feature, stacked bar/area for a few nonnegative features, scatter matrix for small numeric sets, and correlation heatmap for larger ones.',
                'Use it for quick inspection when you do not want to choose the plot form manually.',
            ])
        if class_name == 'DataFramePlotAccessor' and n == 'density':
            return bullets('Practical Behavior', [
                '`density(...)` builds kernel-density traces for one or more numeric features.',
                'The requested point count is clamped to a minimum of 16.',
                'With many features, one density trace is drawn per feature on the same pane.',
            ])
        return []

    def class_specific_intro_sections() -> list[str]:
        if class_name != 'DataFramePlotAccessor':
            return []
        return [
            '```{include} snippets/dataframe_plot_options_reference.md',
            '```',
            '',
        ]

    cfg = CLASS_CONFIG[class_name]
    grouped: dict[str, list[Method]] = {}
    for m in methods:
        grouped.setdefault(m.name, []).append(m)
    lines = [
        f'# {cfg["title"]}',
        '',
        f'# {cfg["subtitle"]}',
        '',
        '## API Reference Style',
        '',
        cfg['intro'],
        '',
    ]
    lines.extend(class_specific_intro_sections())
    for method in sorted(methods, key=lambda m: (m.name.lower(), user_signature(m, class_name).lower())):
        lines.append(f'### `{method.name}`')
        lines.append('')
        lines.append('#### Signature')
        lines.append('')
        lines.append('```cpp')
        lines.append(user_signature(method, class_name))
        lines.append('```')
        lines.append('')
        lines.append('#### Parameters')
        lines.append('')
        lines.append(params_doc(method.params))
        lines.append('')
        lines.append('#### Returns')
        lines.append('')
        lines.append(returns_doc(method.return_type))
        lines.append('')
        lines.append('#### Notes')
        lines.append('')
        lines.append(generic_note(class_name, method))
        if len(grouped[method.name]) > 1:
            sibs = [m for m in grouped[method.name] if m.signature != method.signature]
            if sibs:
                lines.append('')
                lines.append('#### See also')
                lines.append('')
                for sib in sibs:
                    lines.append(f'- `{user_signature(sib, class_name)}`')
        lines.append('')
        lines.append('#### Examples')
        lines.append('')
        lines.append('```cpp')
        lines.append(normalize_user_names(class_name, example_for(class_name, method)))
        lines.append('```')
        preview = normalize_user_names(class_name, output_for(class_name, method))
        if preview:
            lines.append('')
            lines.append('#### Output')
            lines.append('')
            lines.append('```text')
            lines.append(preview)
            lines.append('```')
        extra = extra_sections_for_method(method)
        if extra:
            lines.append('')
            lines.extend(extra)
        lines.append('')
    return '\n'.join(lines)


def main() -> None:
    for class_name, cfg in CLASS_CONFIG.items():
        methods = extract_methods(class_name, cfg['header'])
        methods = [m for m in methods if m.name not in {'begin', 'end'} or class_name in {'Catalog', 'Registry'}]
        cfg['output'].write_text(build_page(class_name, methods))
        print(f'{class_name}: wrote {len(methods)} methods -> {cfg["output"].name}')


if __name__ == '__main__':
    main()
