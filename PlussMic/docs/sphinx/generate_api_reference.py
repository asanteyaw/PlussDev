from __future__ import annotations

import argparse
import hashlib
import json
import pathlib
import re
import sys
import tomllib
from dataclasses import dataclass
from typing import Callable

ROOT = pathlib.Path('/Users/yawasante/software/cxx/PlussDev')
DOCS = ROOT / 'PlussMic/docs/sphinx'
API_CONTENT = DOCS / 'api_content'
PER_METHOD_CLASSES = {
    'DataFrame', 'Thread', 'Catalog', 'Stream', 'Archive', 'Atlas',
    'DataFrameNoteAccessor', 'DataFrameTimelineAccessor', 'DataFrameLabelAccessor',
    'DataFrameFeatureAxisAccessor', 'DataFramePlot3DAccessor', 'DataFrameTsAccessor',
    'DataFrameTsWindowAccessor', 'DataFrameTsResampler', 'DataFrameGroupBuilder',
    'TimeSeriesAccessor', 'DataFrameCastAccessor', 'ThreadPlotAccessor',
    'AtlasLabelAccessor', 'AtlasTimelineAccessor', 'AtlasTsAccessor',
    'AtlasTsResampler', 'AtlasPlotAccessor',
    'Slot', 'Registry', 'Axel',
    'SlotCastAccessor', 'SlotNoteAccessor', 'SlotTimelineAccessor',
    'RegistryFieldAxisAccessor', 'RegistryCastAccessor',
    'RegistryNoteAccessor', 'RegistryTimelineAccessor',
    'AxelFieldAxisAccessor', 'AxelCastAccessor',
    'AxelNoteAccessor', 'AxelTimelineAccessor',
}
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
API_NAV_JS = DOCS / '_static' / 'api_nav.js'
MANUAL_API_NAV = {
    'Nexus': {
        'title': 'Nexus',
        'slug': 'nexus',
        'landing': 'nexus_api_reference.html',
        'index': DOCS / 'nexus_api_reference.md',
    },
    'NexusQuery': {
        'title': 'NexusQuery',
        'slug': 'nexus-query',
        'landing': 'nexus_query_api_reference.html',
        'index': DOCS / 'nexus_query_api_reference.md',
    },
}

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
        'intro': 'This page documents the public prepared-statement helper returned by `DataFrame::prep_sql()` or `DataFrameSqlAccessor::prepare()`.',
    },
    'DataFrameNoteAccessor': {
        'header': ROOT / 'PlussMic/src/table/DataFrame.h',
        'output': DOCS / 'dataframe_note_accessor_api_reference.md',
        'title': 'DataFrameNoteAccessor API Reference',
        'subtitle': 'Practical DataFrameNoteAccessor API Reference',
        'intro': 'This page documents the public string-feature accessor returned by `DataFrame::nt()`.',
        'binder': 'nt()',
    },
    'DataFrameTimelineAccessor': {
        'header': ROOT / 'PlussMic/src/table/DataFrame.h',
        'output': DOCS / 'dataframe_timeline_accessor_api_reference.md',
        'title': 'DataFrameTimelineAccessor API Reference',
        'subtitle': 'Practical DataFrameTimelineAccessor API Reference',
        'intro': 'This page documents the public timestamp-feature accessor returned by `DataFrame::tl()`.',
        'binder': 'tl()',
    },
    'DataFrameLabelAccessor': {
        'header': ROOT / 'PlussMic/src/table/DataFrame.h',
        'output': DOCS / 'dataframe_label_accessor_api_reference.md',
        'title': 'DataFrameLabelAccessor API Reference',
        'subtitle': 'Practical DataFrameLabelAccessor API Reference',
        'intro': 'This page documents the public case-label accessor returned by `DataFrame::label()`.',
        'binder': 'label()',
    },
    'DataFrameFeatureAxisAccessor': {
        'header': ROOT / 'PlussMic/src/table/DataFrame.h',
        'output': DOCS / 'dataframe_feataxis_accessor_api_reference.md',
        'title': 'DataFrameFeatureAxisAccessor API Reference',
        'subtitle': 'Practical DataFrameFeatureAxisAccessor API Reference',
        'intro': 'This page documents the public feature-name-axis accessor returned by `DataFrame::feataxis()`.',
        'binder': 'feataxis()',
    },
    'DataFramePlot3DAccessor': {
        'header': ROOT / 'PlussMic/src/table/plot/DataFramePlot.h',
        'output': DOCS / 'dataframe_plot3d_accessor_api_reference.md',
        'title': 'DataFramePlot3DAccessor API Reference',
        'subtitle': 'Practical DataFramePlot3DAccessor API Reference',
        'intro': 'This page documents the public 3D plotting accessor returned by `DataFrame::plot3d()`.',
        'binder': 'plot3d()',
    },
    'DataFrameTsAccessor': {
        'header': ROOT / 'PlussMic/src/table/DataFrame.h',
        'output': DOCS / 'dataframe_ts_accessor_api_reference.md',
        'title': 'DataFrameTsAccessor API Reference',
        'subtitle': 'Practical DataFrameTsAccessor API Reference',
        'intro': 'This page documents the public time-series accessor returned by `DataFrame::ts()`.',
        'binder': 'ts()',
    },
    'DataFrameTsWindowAccessor': {
        'header': ROOT / 'PlussMic/src/table/DataFrame.h',
        'output': DOCS / 'dataframe_ts_window_accessor_api_reference.md',
        'title': 'DataFrameTsWindowAccessor API Reference',
        'subtitle': 'Practical DataFrameTsWindowAccessor API Reference',
        'intro': 'This page documents the public rolling-window aggregator returned by `DataFrame::ts().window(...)`.',
        'binder': 'ts().window(...)',
    },
    'DataFrameTsResampler': {
        'header': ROOT / 'PlussMic/src/table/DataFrame.h',
        'output': DOCS / 'dataframe_ts_resampler_api_reference.md',
        'title': 'DataFrameTsResampler API Reference',
        'subtitle': 'Practical DataFrameTsResampler API Reference',
        'intro': 'This page documents the public time-based resampling accessor returned by `DataFrame::ts().resample(...)`.',
        'binder': 'ts().resample(...)',
    },
    'DataFrameCastAccessor': {
        'header': ROOT / 'PlussMic/src/table/DataFrame.h',
        'output': DOCS / 'dataframe_cast_accessor_api_reference.md',
        'title': 'DataFrameCastAccessor API Reference',
        'subtitle': 'Practical DataFrameCastAccessor API Reference',
        'intro': 'This page documents the public casting accessor returned by `DataFrame::cast()`.',
        'binder': 'cast()',
    },
    'DataFrameGroupBuilder': {
        'header': ROOT / 'PlussMic/src/table/DataFrame.h',
        'output': DOCS / 'dataframe_group_accessor_api_reference.md',
        'title': 'DataFrameGroupBuilder API Reference',
        'subtitle': 'Practical DataFrameGroupBuilder API Reference',
        'intro': 'This page documents the public grouped-workflow surface entered via `DataFrame::group()` -- the `DataFrameGroupBuilder` returned by `group()` itself, and the `DataFrameGrouped` handle returned by its `.by(...)`.',
        'binder': 'group()',
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
    'TimeSeriesAccessor': {
        'header': ROOT / 'PlussMic/src/table/Thread.h',
        'output': DOCS / 'thread_ts_accessor_api_reference.md',
        'title': 'TimeSeriesAccessor API Reference',
        'subtitle': 'Practical TimeSeriesAccessor API Reference',
        'intro': 'This page documents the public time-series accessor returned by `Thread::ts()`.',
        'binder': 'ts()',
    },
    'ThreadPlotAccessor': {
        'header': ROOT / 'PlussMic/src/table/plot/DataFramePlot.h',
        'output': DOCS / 'thread_plot_api_reference.md',
        'title': 'ThreadPlotAccessor API Reference',
        'subtitle': 'Practical ThreadPlotAccessor API Reference',
        'intro': 'This page documents the public plotting accessor returned by `Thread::plot()`.',
        'binder': 'plot()',
    },
    'Atlas': {
        'header': ROOT / 'PlussMic/src/table/Atlas.h',
        'output': DOCS / 'atlas_api_reference.md',
        'title': 'Atlas API Reference',
        'subtitle': 'Practical Atlas API Reference',
        'intro': 'This page documents the full public `Atlas` surface for frame-stack workflows.',
    },
    'AtlasLabelAccessor': {
        'header': ROOT / 'PlussMic/src/table/Atlas.h',
        'output': DOCS / 'atlas_label_accessor_api_reference.md',
        'title': 'AtlasLabelAccessor API Reference',
        'subtitle': 'Practical AtlasLabelAccessor API Reference',
        'intro': 'This page documents the public depth-label accessor returned by `Atlas::label()`.',
        'binder': 'label()',
    },
    'AtlasTimelineAccessor': {
        'header': ROOT / 'PlussMic/src/table/Atlas.h',
        'output': DOCS / 'atlas_timeline_accessor_api_reference.md',
        'title': 'AtlasTimelineAccessor API Reference',
        'subtitle': 'Practical AtlasTimelineAccessor API Reference',
        'intro': 'This page documents the public timestamp-feature accessor returned by `Atlas::tl()`, applied frame-by-frame across the stack.',
        'binder': 'tl()',
    },
    'AtlasTsAccessor': {
        'header': ROOT / 'PlussMic/src/table/Atlas.h',
        'output': DOCS / 'atlas_ts_accessor_api_reference.md',
        'title': 'AtlasTsAccessor API Reference',
        'subtitle': 'Practical AtlasTsAccessor API Reference',
        'intro': 'This page documents the public time-series accessor returned by `Atlas::ts()`, applied frame-by-frame across the stack.',
        'binder': 'ts()',
    },
    'AtlasTsResampler': {
        'header': ROOT / 'PlussMic/src/table/Atlas.h',
        'output': DOCS / 'atlas_ts_resampler_api_reference.md',
        'title': 'AtlasTsResampler API Reference',
        'subtitle': 'Practical AtlasTsResampler API Reference',
        'intro': 'This page documents the public time-based resampling accessor returned by `Atlas::ts().resample(...)`, applied frame-by-frame across the stack.',
        'binder': 'ts().resample(...)',
    },
    'AtlasPlotAccessor': {
        'header': ROOT / 'PlussMic/src/table/plot/AtlasPlot.h',
        'output': DOCS / 'atlas_plot_accessor_api_reference.md',
        'title': 'AtlasPlotAccessor API Reference',
        'subtitle': 'Practical AtlasPlotAccessor API Reference',
        'intro': 'This page documents the public plotting accessor returned by `Atlas::plot()`.',
        'binder': 'plot()',
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
    'Axel': {
        'header': ROOT / 'PlussMic/src/bundle/Axel.h',
        'output': DOCS / 'axel_api_reference.md',
        'title': 'Axel API Reference',
        'subtitle': 'Practical Axel API Reference',
        'intro': 'This page documents the full public `Axel` surface for depth-stacked registries.',
    },
    'SlotCastAccessor': {
        'header': ROOT / 'PlussMic/src/bundle/Slot.h',
        'output': DOCS / 'slot_cast_accessor_api_reference.md',
        'title': 'SlotCastAccessor API Reference',
        'subtitle': 'Practical SlotCastAccessor API Reference',
        'intro': 'This page documents the public casting accessor returned by `Slot::cast()`.',
        'binder': 'cast()',
    },
    'SlotNoteAccessor': {
        'header': ROOT / 'PlussMic/src/bundle/Slot.h',
        'output': DOCS / 'slot_note_accessor_api_reference.md',
        'title': 'SlotNoteAccessor API Reference',
        'subtitle': 'Practical SlotNoteAccessor API Reference',
        'intro': 'This page documents the public text-processing accessor returned by `Slot::nt()`.',
        'binder': 'nt()',
    },
    'SlotTimelineAccessor': {
        'header': ROOT / 'PlussMic/src/bundle/Slot.h',
        'output': DOCS / 'slot_timeline_accessor_api_reference.md',
        'title': 'SlotTimelineAccessor API Reference',
        'subtitle': 'Practical SlotTimelineAccessor API Reference',
        'intro': 'This page documents the public timestamp-feature accessor returned by `Slot::tl()`.',
        'binder': 'tl()',
    },
    'RegistryFieldAxisAccessor': {
        'header': ROOT / 'PlussMic/src/bundle/registry.h',
        'output': DOCS / 'registry_fieldaxis_accessor_api_reference.md',
        'title': 'RegistryFieldAxisAccessor API Reference',
        'subtitle': 'Practical RegistryFieldAxisAccessor API Reference',
        'intro': 'This page documents the public field-name-matching accessor returned by `Registry::fieldaxis()`.',
        'binder': 'fieldaxis()',
    },
    'RegistryCastAccessor': {
        'header': ROOT / 'PlussMic/src/bundle/registry.h',
        'output': DOCS / 'registry_cast_accessor_api_reference.md',
        'title': 'RegistryCastAccessor API Reference',
        'subtitle': 'Practical RegistryCastAccessor API Reference',
        'intro': 'This page documents the public casting accessor returned by `Registry::fields(keys).cast()`.',
        'binder': 'fields(keys).cast()',
    },
    'RegistryNoteAccessor': {
        'header': ROOT / 'PlussMic/src/bundle/registry.h',
        'output': DOCS / 'registry_note_accessor_api_reference.md',
        'title': 'RegistryNoteAccessor API Reference',
        'subtitle': 'Practical RegistryNoteAccessor API Reference',
        'intro': 'This page documents the public text-processing accessor returned by `Registry::fields(keys).nt()`.',
        'binder': 'fields(keys).nt()',
    },
    'RegistryTimelineAccessor': {
        'header': ROOT / 'PlussMic/src/bundle/registry.h',
        'output': DOCS / 'registry_timeline_accessor_api_reference.md',
        'title': 'RegistryTimelineAccessor API Reference',
        'subtitle': 'Practical RegistryTimelineAccessor API Reference',
        'intro': 'This page documents the public timestamp-feature accessor returned by `Registry::fields(keys).tl()`.',
        'binder': 'fields(keys).tl()',
    },
    'AxelFieldAxisAccessor': {
        'header': ROOT / 'PlussMic/src/bundle/Axel.h',
        'output': DOCS / 'axel_fieldaxis_accessor_api_reference.md',
        'title': 'AxelFieldAxisAccessor API Reference',
        'subtitle': 'Practical AxelFieldAxisAccessor API Reference',
        'intro': 'This page documents the public field-name-matching accessor returned by `Axel::fieldaxis()`, applied across the depth stack.',
        'binder': 'fieldaxis()',
    },
    'AxelCastAccessor': {
        'header': ROOT / 'PlussMic/src/bundle/Axel.h',
        'output': DOCS / 'axel_cast_accessor_api_reference.md',
        'title': 'AxelCastAccessor API Reference',
        'subtitle': 'Practical AxelCastAccessor API Reference',
        'intro': 'This page documents the public casting accessor returned by `Axel::fields(keys).cast()`, applied across the depth stack.',
        'binder': 'fields(keys).cast()',
    },
    'AxelNoteAccessor': {
        'header': ROOT / 'PlussMic/src/bundle/Axel.h',
        'output': DOCS / 'axel_note_accessor_api_reference.md',
        'title': 'AxelNoteAccessor API Reference',
        'subtitle': 'Practical AxelNoteAccessor API Reference',
        'intro': 'This page documents the public text-processing accessor returned by `Axel::fields(keys).nt()`, applied across the depth stack.',
        'binder': 'fields(keys).nt()',
    },
    'AxelTimelineAccessor': {
        'header': ROOT / 'PlussMic/src/bundle/Axel.h',
        'output': DOCS / 'axel_timeline_accessor_api_reference.md',
        'title': 'AxelTimelineAccessor API Reference',
        'subtitle': 'Practical AxelTimelineAccessor API Reference',
        'intro': 'This page documents the public timestamp-feature accessor returned by `Axel::fields(keys).tl()`, applied across the depth stack.',
        'binder': 'fields(keys).tl()',
    },
}

METHOD_PAGE_DIR = {
    'DataFrame': 'dataframe',
    'Thread': 'thread',
    'Catalog': 'catalog',
    'Stream': 'stream',
    'Archive': 'archive',
    'Atlas': 'atlas',
    'AtlasLabelAccessor': 'atlas-label',
    'AtlasTimelineAccessor': 'atlas-timeline',
    'AtlasTsAccessor': 'atlas-ts',
    'AtlasTsResampler': 'atlas-ts-resample',
    'AtlasPlotAccessor': 'atlas-plot',
    'DataFrameNoteAccessor': 'dataframe-note',
    'DataFrameTimelineAccessor': 'dataframe-timeline',
    'DataFrameLabelAccessor': 'dataframe-label',
    'DataFrameFeatureAxisAccessor': 'dataframe-feataxis',
    'DataFramePlot3DAccessor': 'dataframe-plot3d',
    'DataFrameTsAccessor': 'dataframe-ts',
    'DataFrameTsWindowAccessor': 'dataframe-ts-window',
    'DataFrameTsResampler': 'dataframe-ts-resample',
    'DataFrameGroupBuilder': 'dataframe-group',
    'TimeSeriesAccessor': 'thread-ts',
    'DataFrameCastAccessor': 'dataframe-cast',
    'ThreadPlotAccessor': 'thread-plot',
    'Slot': 'slot',
    'Registry': 'registry',
    'Axel': 'axel',
    'SlotCastAccessor': 'slot-cast',
    'SlotNoteAccessor': 'slot-note',
    'SlotTimelineAccessor': 'slot-timeline',
    'RegistryFieldAxisAccessor': 'registry-fieldaxis',
    'RegistryCastAccessor': 'registry-cast',
    'RegistryNoteAccessor': 'registry-note',
    'RegistryTimelineAccessor': 'registry-timeline',
    'AxelFieldAxisAccessor': 'axel-fieldaxis',
    'AxelCastAccessor': 'axel-cast',
    'AxelNoteAccessor': 'axel-note',
    'AxelTimelineAccessor': 'axel-timeline',
}

# Maps a class to the accessor classes it exposes, so their landing pages can
# be cross-linked from the parent's auto-generated index page (see
# `build_method_index_page`). This lets `ts()`'s own landing page in turn
# link to `window()`/`resample()`'s pages -- the nesting is just one more
# entry in this same dict, not a special case in the generator.
ACCESSOR_CHILDREN: dict[str, list[str]] = {
    'DataFrame': [
        'DataFrameNoteAccessor',
        'DataFrameTimelineAccessor',
        'DataFrameLabelAccessor',
        'DataFrameFeatureAxisAccessor',
        'DataFramePlot3DAccessor',
        'DataFrameTsAccessor',
    ],
    'DataFrameTsAccessor': [
        'DataFrameTsWindowAccessor',
        'DataFrameTsResampler',
    ],
    'Thread': [
        'TimeSeriesAccessor',
        'ThreadPlotAccessor',
    ],
    'Atlas': [
        'AtlasLabelAccessor',
        'AtlasTimelineAccessor',
        'AtlasTsAccessor',
        'AtlasPlotAccessor',
    ],
    'AtlasTsAccessor': [
        'AtlasTsResampler',
    ],
    'Slot': [
        'SlotCastAccessor',
        'SlotNoteAccessor',
        'SlotTimelineAccessor',
    ],
    'Registry': [
        'RegistryFieldAxisAccessor',
        'RegistryCastAccessor',
        'RegistryNoteAccessor',
        'RegistryTimelineAccessor',
    ],
    'Axel': [
        'AxelFieldAxisAccessor',
        'AxelCastAccessor',
        'AxelNoteAccessor',
        'AxelTimelineAccessor',
    ],
}


def clean_space(s: str) -> str:
    return re.sub(r'\s+', ' ', s).strip()


def public_cpp_type(text: str) -> str:
    out = text
    replacements = (
        ('pluss::codem::ScalarType', 'pluss::ScalarType'),
        ('pluss::msharp::RangeView::Int', 'pluss::RangeView<std::int64_t>'),
        ('pluss::msharp::RangeView::Double', 'pluss::RangeView<double>'),
        ('pluss::msharp::RangeView::Bool', 'pluss::RangeView<bool>'),
        ('RangeView::Int', 'pluss::RangeView<std::int64_t>'),
        ('RangeView::Double', 'pluss::RangeView<double>'),
        ('RangeView::Bool', 'pluss::RangeView<bool>'),
        ('pluss::msharp::RangeView::Of<', 'pluss::RangeView<'),
        ('RangeView::Of<', 'pluss::RangeView<'),
    )
    for old, new in replacements:
        out = out.replace(old, new)
    return out


def split_top_level(text: str) -> list[str]:
    out = []
    cur = []
    depth_angle = depth_paren = depth_brace = depth_brack = 0
    in_single = False
    in_double = False
    escaped = False
    i = 0
    while i < len(text):
        ch = text[i]
        if escaped:
            cur.append(ch)
            escaped = False
            i += 1
            continue
        if ch == '\\':
            cur.append(ch)
            escaped = True
            i += 1
            continue
        if ch == "'" and not in_double:
            in_single = not in_single
        elif ch == '"' and not in_single:
            in_double = not in_double
        elif not in_single and not in_double and ch == '<':
            depth_angle += 1
        elif not in_single and not in_double and ch == '>':
            depth_angle = max(0, depth_angle - 1)
        elif not in_single and not in_double and ch == '(':
            depth_paren += 1
        elif not in_single and not in_double and ch == ')':
            depth_paren = max(0, depth_paren - 1)
        elif not in_single and not in_double and ch == '{':
            depth_brace += 1
        elif not in_single and not in_double and ch == '}':
            depth_brace = max(0, depth_brace - 1)
        elif not in_single and not in_double and ch == '[':
            depth_brack += 1
        elif not in_single and not in_double and ch == ']':
            depth_brack = max(0, depth_brack - 1)
        elif (not in_single and not in_double
              and ch == ',' and depth_angle == depth_paren == depth_brace == depth_brack == 0):
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


def strip_default_value(part: str) -> str:
    depth_angle = depth_paren = depth_brace = depth_brack = 0
    in_single = False
    in_double = False
    escaped = False
    out = []
    for ch in part:
        if escaped:
            out.append(ch)
            escaped = False
            continue
        if ch == '\\':
            out.append(ch)
            escaped = True
            continue
        if ch == "'" and not in_double:
            in_single = not in_single
        elif ch == '"' and not in_single:
            in_double = not in_double
        elif not in_single and not in_double:
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
            elif ch == '=' and depth_angle == depth_paren == depth_brace == depth_brack == 0:
                break
        out.append(ch)
    return ''.join(out).strip()


def parse_params(text: str) -> list[tuple[str, str]]:
    text = text.strip()
    if not text or text == 'void':
        return []
    params = []
    for part in split_top_level(text):
        part = clean_space(part)
        part = clean_space(strip_default_value(part))
        m = re.match(r'(.+?)\s+(\w+)$', part)
        if m:
            params.append((m.group(2), m.group(1)))
        else:
            params.append((f'arg{len(params)+1}', part))
    return params


def parse_signature(sig: str, class_name: str) -> Method | None:
    sig = clean_space(sig)
    sig = sig.rstrip(';').strip()
    sig = re.sub(r'\{\s*$', '', sig).strip()
    sig = sig.replace(' noexcept', '')
    if sig.startswith('template ') and ' requires(' in sig:
        req_idx = sig.find(' requires(')
        if req_idx != -1:
            depth = 0
            end_idx = -1
            for i in range(req_idx + 1, len(sig)):
                ch = sig[i]
                if ch == '(':
                    depth += 1
                elif ch == ')':
                    depth -= 1
                    if depth == 0:
                        end_idx = i
                        break
            if end_idx != -1:
                sig = sig[end_idx + 1:].strip()
    if not sig or '(' not in sig:
        return None
    before, rest = sig.split('(', 1)
    params_text = rest.rsplit(')', 1)[0]
    before = clean_space(before)
    name = before.split()[-1]
    if name == class_name or name == f'~{class_name}':
        return None
    if name.startswith('operator') or name == 'requires':
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
    decl_paren = 0
    decl_angle = 0
    decl_brack = 0
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
                    decl_paren = decl_angle = decl_brack = 0
                    break
            else:
                structural_brace = ch == '{' and decl_paren == 0 and decl_angle == 0 and decl_brack == 0
                structural_close = ch == '}' and decl_paren == 0 and decl_angle == 0 and decl_brack == 0

                if ch == '(':
                    decl_paren += 1
                elif ch == ')':
                    decl_paren = max(0, decl_paren - 1)
                elif ch == '<':
                    decl_angle += 1
                elif ch == '>':
                    decl_angle = max(0, decl_angle - 1)
                elif ch == '[':
                    decl_brack += 1
                elif ch == ']':
                    decl_brack = max(0, decl_brack - 1)

                if structural_brace:
                    depth += 1
                elif structural_close:
                    depth = max(0, depth - 1)
                decl.append(ch)
                if ch == ';' and visibility == 'public':
                    statement = clean_space(''.join(decl))
                    decl = []
                    decl_paren = decl_angle = decl_brack = 0
                    if any(statement.startswith(x) for x in ('enum class ', 'struct ', 'class ', 'using ', 'friend ')):
                        pass
                    else:
                        method = parse_signature(statement, class_name)
                        if method:
                            methods.append(method)
                elif ch == ';':
                    decl = []
                    decl_paren = decl_angle = decl_brack = 0
                elif structural_brace:
                    statement = clean_space(''.join(decl))
                    if visibility == 'public':
                        method = parse_signature(statement, class_name)
                        if method:
                            methods.append(method)
                    decl = []
                    decl_paren = decl_angle = decl_brack = 0
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
    declaration = clean_space(method.signature).rstrip(';')
    params_text = declaration.split('(', 1)[1].rsplit(')', 1)[0]
    rendered_params: list[str] = []
    for index, raw_part in enumerate(split_top_level(params_text), start=1):
        part = clean_space(raw_part)
        without_default = clean_space(strip_default_value(part))
        default = part[len(without_default):].strip()
        match = re.match(r'(.+?)\s+(\w+)$', without_default)
        if match:
            typ = public_cpp_type(match.group(1))
            name = match.group(2)
        else:
            typ = public_cpp_type(without_default)
            name = f'arg{index}'
        rendered_params.append(
            f'{typ} {name}' + (f' {default}' if default else '')
        )
    ret = public_cpp_type(method.return_type)
    return f'{class_name}::{method.name}({", ".join(rendered_params)}) -> {ret}'


def returns_doc(ret: str) -> str:
    base = public_cpp_type(ret.strip())
    base = re.sub(r'^template <.*>\s+requires\s+.*\s+([A-Za-z_:][A-Za-z0-9_:<>]*)$', r'\1', base)
    descriptions = {
        'bool': 'Boolean status or membership result.',
        'void': 'Performs the side effect and returns no value.',
        'std::int64_t': 'Integer count, size, or positional result.',
        'std::string': 'Rendered string result.',
        'MatrixValues': 'Grouped matrix blocks split by payload family (`Series`, `Note`, `Timeline`, and `TimeIndex`).',
        'DataFrame::MatrixValues': 'Grouped matrix blocks split by payload family (`Series`, `Note`, `Timeline`, and `TimeIndex`).',
        'FrameMatch': 'Pair containing matched `left` and `right` frames with identical case and feature axes.',
        'pluss::loader::LoadResult': 'Multi-sheet Excel load bundle containing one frame per requested sheet.',
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


def method_content_path(class_name: str, method_name: str) -> pathlib.Path:
    namespace = {
        'DataFrame': 'table/DataFrame',
        'Thread': 'table/Thread',
        'Atlas': 'table/Atlas',
        'ColumnRef': 'table/ColumnRef',
        'DataFramePlotAccessor': 'table/DataFramePlotAccessor',
        'DataFrameSqlAccessor': 'table/DataFrameSqlAccessor',
        'DataFrameSqlPrepared': 'table/DataFrameSqlPrepared',
        'ThreadValue': 'table/ThreadValue',
        'Stream': 'kindle/Stream',
        'Catalog': 'kindle/Catalog',
        'Archive': 'kindle/Archive',
        'Slot': 'bundle/Slot',
        'Registry': 'bundle/Registry',
        'Axel': 'bundle/Axel',
    }.get(class_name, class_name)
    return API_CONTENT / namespace / f'{method_name}.toml'


def load_method_content(class_name: str, method_name: str) -> dict | None:
    path = method_content_path(class_name, method_name)
    if not path.exists():
        return None
    with path.open('rb') as fh:
        return tomllib.load(fh)


def render_named_markdown_blocks(items: list[dict], section_name: str) -> list[str]:
    lines: list[str] = [f'#### {section_name}', '']
    for item in items:
        name = item['name']
        body = item.get('body', '').strip()
        lines.append(f'**{name}**')
        lines.append('')
        if body:
            lines.extend(body.splitlines())
        else:
            lines.append('No additional details yet.')
        lines.append('')
    return lines


def render_string_list_section(title: str, items: list[str]) -> list[str]:
    if not items:
        return []
    lines: list[str] = [f'#### {title}', '']
    for item in items:
        lines.append(f'- {item}')
    lines.append('')
    return lines


def render_admonitions(kind: str, items: list[dict]) -> list[str]:
    lines: list[str] = []
    for item in items:
        title = item.get('title', kind.title())
        body = item.get('body', '').strip()
        lines.append(f'```{{{kind}}} {title}')
        if body:
            lines.append('')
            lines.extend(body.splitlines())
        lines.append('```')
        lines.append('')
    return lines


def render_structured_method_page(
    class_name: str,
    name: str,
    overloads: list[Method],
    content: dict,
) -> list[str]:
    lines: list[str] = [f'### `{name}`', '']
    lines.append('#### Signatures' if len(overloads) > 1 else '#### Signature')
    lines.append('')
    lines.append('```cpp')
    for idx, overload in enumerate(overloads):
        if idx:
            lines.append('')
        lines.append(user_signature(overload, class_name))
    lines.append('```')
    lines.append('')

    summary = str(content.get('summary', '')).strip()
    if summary:
        lines.append(summary)
        lines.append('')

    warnings = content.get('warnings', [])
    if isinstance(warnings, list):
        lines.extend(render_admonitions('warning', warnings))

    parameters = content.get('parameters', [])
    if isinstance(parameters, list) and parameters:
        lines.extend(render_named_markdown_blocks(parameters, 'Parameters'))
    else:
        lines.append('#### Parameters')
        lines.append('')
        lines.append('`None`')
        lines.append('')

    returns_text = str(content.get('returns', '')).strip()
    if returns_text:
        lines.append('#### Returns')
        lines.append('')
        lines.extend(returns_text.splitlines())
        lines.append('')

    see_also = content.get('see_also', [])
    if isinstance(see_also, list):
        lines.extend(render_string_list_section('See Also', [str(item) for item in see_also]))

    notes = content.get('notes', [])
    if isinstance(notes, list) and notes:
        lines.append('#### Notes')
        lines.append('')
        for note in notes:
            note_text = str(note).strip()
            if not note_text:
                continue
            lines.extend(note_text.splitlines())
            lines.append('')

    examples = content.get('examples', [])
    if isinstance(examples, list) and examples:
        lines.append('#### Examples')
        lines.append('')
        for index, example in enumerate(examples, start=1):
            title = str(example.get('title', f'Example {index}')).strip()
            code = str(example.get('code', '')).strip()
            output = str(example.get('output', '')).strip()
            lines.append(f'Example {index}. {title}')
            lines.append('')
            if code:
                lines.append('```cpp')
                lines.extend(code.splitlines())
                lines.append('```')
                lines.append('')
            lines.append('##### Output')
            lines.append('')
            lines.append('```text')
            lines.extend(output.splitlines() if output else ['Output not captured yet.'])
            lines.append('```')
            lines.append('')
    return lines


def render_method_prev_next(prev_name: str | None, next_name: str | None) -> list[str]:
    if not prev_name and not next_name:
        return []
    lines = ['<div class="pluss-inline-prev-next">']
    if prev_name:
        lines.extend([
            f'  <a class="prev-link" href="#{prev_name}">',
            '    <span class="label">Previous</span>',
            f'    <span class="title">{prev_name}</span>',
            '  </a>',
        ])
    if next_name:
        lines.extend([
            f'  <a class="next-link" href="#{next_name}">',
            '    <span class="label">Next</span>',
            f'    <span class="title">{next_name}</span>',
            '  </a>',
        ])
    lines.append('</div>')
    lines.append('')
    return lines


def ordered_method_names(methods: list[Method]) -> list[str]:
    grouped_names = sorted({m.name for m in methods}, key=str.lower)
    by_letter: dict[str, list[str]] = {}
    for name in grouped_names:
        by_letter.setdefault(name[0].upper(), []).append(name)
    return [name for letter in sorted(by_letter.keys()) for name in by_letter[letter]]


def parse_hidden_toctree_entries(index_path: pathlib.Path) -> list[str]:
    text = index_path.read_text(encoding='utf-8')
    match = re.search(r'```{toctree}(.*?)```', text, re.S)
    if not match:
        raise RuntimeError(f'No toctree found in {index_path}')
    entries: list[str] = []
    for raw_line in match.group(1).splitlines():
        line = raw_line.strip()
        if not line or line.startswith(':'):
            continue
        entries.append(line)
    return entries


def manual_nav_methods(index_path: pathlib.Path) -> list[str]:
    return [pathlib.Path(entry).name for entry in parse_hidden_toctree_entries(index_path)]


def method_page_root(class_name: str) -> pathlib.Path:
    return DOCS / 'api' / METHOD_PAGE_DIR[class_name]


def method_page_doc_path(class_name: str, method_name: str) -> pathlib.Path:
    return method_page_root(class_name) / f'{method_name}.md'


def method_page_rel_from_index(class_name: str, method_name: str) -> str:
    return f'api/{METHOD_PAGE_DIR[class_name]}/{method_name}'


def method_page_rel_from_method(current_class: str, target_class: str, method_name: str) -> str:
    if current_class == target_class:
        return f'{method_name}'
    return f'../{METHOD_PAGE_DIR[target_class]}/{method_name}'


def method_page_html_rel_from_method(current_class: str, target_class: str, method_name: str) -> str:
    return f'{method_page_rel_from_method(current_class, target_class, method_name)}.html'


def first_paragraph(text: str) -> str:
    stripped = text.strip()
    if not stripped:
        return ''
    parts = re.split(r'\n\s*\n', stripped, maxsplit=1)
    return parts[0].strip()


def method_summary(class_name: str, overloads: list[Method]) -> str:
    structured = load_method_content(class_name, overloads[0].name)
    if structured is not None:
        return first_paragraph(str(structured.get('summary', '')))
    return generic_note(class_name, overloads[0]).strip()


def strip_inline_prev_next(text: str) -> str:
    return re.sub(r'\n?<div class="pluss-inline-prev-next">.*?</div>\n?', '\n', text, flags=re.S)


def rewrite_method_links(text: str,
                         current_class: str,
                         known_methods: dict[str, set[str]]) -> str:
    def replace_href(match: re.Match[str]) -> str:
        href = match.group(1)
        new_href = href
        if href.startswith('#'):
            target = href[1:]
            if target in known_methods.get(current_class, set()):
                new_href = method_page_html_rel_from_method(current_class, current_class, target)
        else:
            same_class_target = re.fullmatch(r'([A-Za-z_][A-Za-z0-9_]*)\.md', href)
            if same_class_target:
                target = same_class_target.group(1)
                if target in known_methods.get(current_class, set()):
                    new_href = method_page_html_rel_from_method(current_class, current_class, target)
            target_match = re.fullmatch(r'(?:\.\./)*(dataframe|thread)_api_reference\.(?:md|html)#([A-Za-z_][A-Za-z0-9_]*)', href)
            if target_match:
                target_class = 'DataFrame' if target_match.group(1) == 'dataframe' else 'Thread'
                target = target_match.group(2)
                if target in known_methods.get(target_class, set()):
                    new_href = method_page_html_rel_from_method(current_class, target_class, target)
        return f'href="{new_href}"'

    text = re.sub(r'href="([^"]+)"', replace_href, text)

    def replace_md_link(match: re.Match[str]) -> str:
        doc = match.group(1)
        target = match.group(2)
        target_class = 'DataFrame' if doc == 'dataframe' else 'Thread'
        if target in known_methods.get(target_class, set()):
            return f'({method_page_rel_from_method(current_class, target_class, target)})'
        return match.group(0)

    text = re.sub(r'\((?:\.\./)*(dataframe|thread)_api_reference\.(?:md|html)#([A-Za-z_][A-Za-z0-9_]*)\)', replace_md_link, text)
    return text


def fallback_param_note(name: str, label_type: str) -> str:
    if label_type == 'bool':
        return 'Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.'
    if label_type in {'std::ostream&', 'std::ostream'}:
        return 'Destination output stream used instead of the default display or print target.'
    if 'std::function<' in label_type:
        return 'Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.'
    if 'initializer_list' in label_type:
        return 'Inline collection passed directly at the call site.'
    if 'pluss::RangeView<std::string_view>' in label_type or 'RangeView<std::string_view>' in label_type:
        return 'Feature or field names supplied as a lightweight view over several strings.'
    if label_type in {'std::string_view', 'std::string'}:
        string_name_notes = {
            'name': 'Name used to identify the target object, field, feature, or slot for this overload.',
            'filename': 'Filename or path-like string used by this overload.',
            'field_name': 'Field name used to select the target text, stream, or grouping column.',
            'field1': 'First named field used by this overload.',
            'field2': 'Second named field used by this overload.',
            'key_field': 'Field name that acts as the key for matching, grouping, or lookup.',
            'by_field': 'Field name that controls the grouping or sorting basis of the operation.',
            'on': 'Field name used as the match or join key for this overload.',
            'where': 'Filter expression or selector string interpreted by this overload.',
            'method': 'Method selector string understood by this overload.',
            'pattern': 'Pattern string used for matching or extraction.',
            'needle': 'Substring or token to search for.',
            'prefix': 'Text prefix inserted ahead of the existing label or value.',
            'suffix': 'Text suffix appended after the existing label or value.',
            'stat_type': 'Statistic selector string understood by this overload.',
            'target_type': 'Format or target selector string used by this overload.',
        }
        if name in string_name_notes:
            return string_name_notes[name]
    if label_type in {'std::int64_t', 'std::size_t', 'int'}:
        integer_name_notes = {
            'row': 'Zero-based row position used by this overload.',
            'i': 'Zero-based positional index used by this overload.',
            'index': 'Zero-based positional index used by this overload.',
            'feature_idx': 'Zero-based feature position used by this overload.',
            'lhs_row': 'Zero-based left-hand row position used in a row-wise comparison.',
            'rhs_row': 'Zero-based right-hand row position used in a row-wise comparison.',
            'label': 'Integer label or bucket code used by this overload.',
        }
        if name in integer_name_notes:
            return integer_name_notes[name]
    if name == 'os':
        return 'Destination output stream used instead of the default display or print target.'
    if name in {'fn', 'func', 'expr'}:
        return 'Callable used to transform, map, or derive values in this overload.'
    if name == 'predicate':
        return 'Callable that must return `true` for entries you want to keep.'
    if name in {'mask', 'condition'}:
        return 'Boolean mask used by this overload to decide which rows, values, or items participate.'
    if name in {'conditions', 'choices'}:
        return 'Ordered collection used by a select-style overload. Each condition or choice is matched positionally with the others.'
    if name == 'values':
        return 'Value collection supplied to this overload. The ordering must match the corresponding names or positions expected by the method.'
    if name == 'items':
        return 'Items supplied in one batch to this overload.'
    if name == 'mapping':
        return 'Name-to-name mapping used by this overload.'
    if name == 'indices':
        return 'Integer positions used to select, reorder, or address entries.'
    if name == 'row_span':
        return 'Row span given as a `(start, stop)` pair.'
    if name in {'from', 'to'}:
        return 'Boundary or replacement value used by this overload. Its type determines which payload family participates.'
    if name in {'a', 'b', 'lo', 'hi', 'min_value', 'max_value'}:
        return 'Lower/upper boundary used by this overload.'
    if name == 'default_value':
        return 'Fallback value written anywhere the input is missing or unmatched.'
    if name == 'dtype':
        return 'Target scalar dtype used by this conversion overload.'
    if name == 'series_dtype':
        return 'Optional scalar dtype used when the cast target is a numeric `Series`.'
    if name == 'target':
        return 'Target payload family chosen by this cast overload.'
    if name == 'thread':
        return 'Thread payload supplied to this overload.'
    if name == 'stream':
        return 'Stream payload supplied to this overload.'
    if name == 'frame':
        return 'Frame payload supplied to this overload.'
    if name == 'catalog':
        return 'Catalog payload supplied to this overload.'
    if name == 'archive':
        return 'Archive payload supplied to this overload.'
    if name == 'policy':
        return 'Policy object that changes the structural behavior of this overload.'
    if name == 'ascending':
        return 'Sort-order switch. `true` means ascending order and `false` means descending order.'
    if name == 'nulls_last':
        return 'Null-order switch. `true` places nulls after observed values and `false` places them first.'
    if name in {'inclusive', 'include_left', 'include_right'}:
        return 'Boundary-inclusion switch controlling whether the interval endpoints are treated as closed or open.'
    if name == 'unbiased':
        return 'When `true`, use the sample-statistic form. When `false`, use the population-statistic form.'
    if name == 'chars':
        return 'Characters to trim from the edge of each text value.'
    if name == 'field_names':
        return 'Field names supplied together for this overload.'
    if name in {'label', 'labels'}:
        return 'Label or labels used by this overload.'
    return 'Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.'


def enum_note(intro: str, items: list[tuple[str, str]], heading: str = 'Supported values') -> str:
    lines: list[str] = []
    if intro:
        lines.append(intro)
        lines.append('')
    lines.append(f'{heading}:')
    for label, desc in items:
        lines.append(f'- `{label}`: {desc}')
    return '\n'.join(lines)


def indent_note(note: str) -> str:
    return '\n'.join(('    ' + line) if line else '' for line in note.splitlines())


def params_doc(params: list[tuple[str, str]], class_name: str | None = None, method_name: str | None = None) -> str:
    if not params:
        return '`None`'
    param_notes = {
        'filepath': 'File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.',
        'source_type': 'Storage format selector such as `"csv"`, `"xlsx"`, `"json"`, or `"parquet"`.',
        'config': 'Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.',
        'config_entries': 'Inline configuration entries used to build a lightweight loader configuration without naming a separate config object.',
        'options': 'Typed loader, renderer, SQL, plotting, or policy options for this overload.',
        'engine': 'Backend selector. Use `"auto"` to let PlussMic choose, or pass an explicit engine name when you need predictable routing.',
        'sheet_name': 'Worksheet name to read from or write to inside an Excel workbook.',
        'sheet_names': 'Worksheet names to load together from one Excel workbook. This overload returns a multi-sheet load result instead of one frame.',
        'sheet_index': 'Zero-based worksheet position inside an Excel workbook.',
        'sheet_indices': 'Zero-based worksheet positions to load together from one Excel workbook.',
        'header': 'Controls whether feature names are written to, or expected from, the file header row.',
        'add_cases': 'When `true`, the current case axis is exported as its own column so row labels can be reconstructed when the file is loaded again.',
        'case_label': 'Optional column label to use for the exported case axis when `add_cases=true`.',
        'case_col': 'Optional source column that should become the DataFrame case axis while loading.',
        'case_feature': 'Name to use for the restored case-axis feature when cases are materialized back into ordinary columns.',
        'null_rep': 'String written anywhere a null or missing value must be serialized.',
        'float_format': 'Optional numeric formatting string used when floating-point values are written to text output.',
        'sep': 'Column separator used for delimited text files such as CSV.',
        'ncases': 'Optional row limit applied while loading. Leave unset to read the whole source.',
        'usefeats': 'Feature names to load from the source. Leave empty to load every available feature.',
        'quotechar': 'Quote character used when writing delimited text output.',
        'doublequote': 'When `true`, embedded quote characters are escaped by doubling them in CSV-style output.',
        'line_terminator': 'Line ending written between serialized rows in text-based export.',
        'decimal': 'Decimal marker used when formatting floating-point output.',
        'encoding': 'Text encoding used when reading or writing text-based files.',
        'on_bad_lines': 'Policy for malformed delimited rows during load. Common choices are to error, warn, or skip.',
        'new_cases': 'Replacement case labels in the exact order you want the rows to follow.',
        'new_label': 'Replacement case labels in the exact order you want the rows to follow.',
        'cases': 'Case-label thread to attach to the current object.',
        'rows': enum_note(
            'Selector describing which rows `case_study(...)` should return.',
            [
                ('`pluss::table::pos(row)`', 'One row by zero-based position. Negative positions count from the end.'),
                ('`pluss::table::pos(start, stop)`', 'A half-open positional row slice `[start, stop)`.'),
                ('`pluss::table::pos({r1, r2, ...})`', 'An explicit row-position list in the exact output order you want.'),
                ('`pluss::table::label(value)`', 'Every row whose case label matches `value`.'),
                ('`pluss::table::labels({...})`', 'Several case labels in requested order. If one label appears multiple times, every matching row is returned.'),
                ('`pluss::table::label_range(start, stop)`', 'An inclusive case-label window from the first `start` match through the last `stop` match.'),
                ('`pluss::table::mask(bool_thread)`', 'A boolean row mask, equivalent to filtering first and then projecting columns.'),
            ],
        ),
        'rows_selector': 'Alias for the row selector used by `case_study(...)`.',
        'features': enum_note(
            'Selector describing which feature columns `case_study(...)` should keep after resolving the row selection.',
            [
                ('`pluss::table::all_features()`', 'Keep every feature in the same left-to-right order as the source frame.'),
                ('`pluss::table::feature("name")`', 'Keep one named feature.'),
                ('`pluss::table::feature(position)`', 'Keep one feature by zero-based position.'),
                ('`pluss::table::features({"a", "b"})`', 'Keep several named features in the requested order.'),
                ('`pluss::table::features({0, 2})`', 'Keep several feature positions in the requested order.'),
                ('`pluss::table::features(start, stop)`', 'A half-open positional feature slice `[start, stop)`.'),
                ('`pluss::table::feature_range("start", "stop")`', 'An inclusive feature-name window from `start` through `stop`.'),
            ],
        ),
        'drop_feature': 'When `true`, the feature promoted to cases is removed from the ordinary feature columns.',
        'feature': 'Name of the source feature used by the operation.',
        'feature_name': 'Name of the target feature to read, rewrite, sort by, or promote to cases.',
        'feature_names': 'Feature name(s) supplied explicitly for subset, ordering, or axis-replacement operations, depending on the method.',
        'feature_index': 'Zero-based output column position used by structural methods such as `insert(...)`.',
        'feature_feature': 'Feature whose distinct values become new output columns in pivot-style reshaping.',
        'x_feature': 'Feature used on the horizontal axis of a plot. Leave it empty to let the plot use the case axis or the accessor default when that overload allows it.',
        'y_feature': 'Single numeric feature to draw on the vertical axis or as the primary plotted measure.',
        'y_features': 'Several numeric features plotted together on the same figure. Each selected feature usually becomes its own trace.',
        'hue_feature': 'Optional feature whose distinct values split one plot into color-coded groups.',
        'class_feature': 'Optional grouping or class label feature used to color, partition, or annotate multivariate plots.',
        'label_feature': 'Optional feature that supplies slice labels or category labels in label-aware plots such as pie charts.',
        'value_feature': 'Feature whose values populate pivot cells or grouped summaries.',
        'group_feature': 'Feature whose distinct values define the groups for grouped summaries such as `mean_by(...)`.',
        'time_feature': 'Name of the Timeline or TimeIndex feature that defines the resampling, alignment, or calendar axis.',
        'value_features': 'Feature name(s) to aggregate inside each resampling bin. Leave empty when you want PlussMic to infer compatible value features.',
        'bins': 'Strictly increasing interval edges used to define bucket boundaries.',
        'labels': 'Optional integer labels assigned to the output buckets. Leave empty to let PlussMic use `0, 1, 2, ...`.',
        'right': 'When `true`, intervals are right-closed. When `false`, they are left-closed and right-open.',
        'include_lowest': 'When `true`, the first bucket also includes its left edge.',
        'bool_mask': 'Boolean row mask. `true` keeps or rewrites the corresponding row, while `false` leaves it out or leaves the original value unchanged depending on the method.',
        'value': 'Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillnull(...)`, or `replace(...)`.',
        'spec': 'Structured reorder description. It lets you move a subset of features to the front, back, or relative to one anchor feature without listing the entire schema.',
        'specs': 'Collection of structured operation specs. Each element describes one requested summary, derive, transform, group, or placement rule depending on the method.',
        'anchor': 'Anchor feature used by partial reordering when placement is `Before` or `After`.',
        'anchor_feature': 'Reference feature used to place a newly inserted or moved feature before or after an existing column.',
        'after': 'When `false`, placement happens before the anchor. When `true`, placement happens after the anchor.',
        'left_on': 'Key feature name(s) on the left frame used to match rows during `unify(...)`.',
        'right_on': 'Key feature name(s) on the right frame used to match rows during `unify(...)`. Leave empty to reuse the same names as `left_on`.',
        'how': enum_note('Join mode for `DataFrame::unify(...)`.', [
            ('common', 'Keep only rows whose key values match on both sides.'),
            ('keep_left', 'Preserve left keys and attach right-hand matches where they exist.'),
            ('keep_right', 'Preserve right keys and attach left-hand matches where they exist.'),
            ('unite', 'Keep the stable union of keys from both frames.'),
            ('cross', 'Create the cartesian product; key feature arguments must be empty.'),
            ('left_only', 'Keep left rows whose keys do not occur on the right.'),
            ('right_only', 'Keep right rows whose keys do not occur on the left.'),
        ]),
        'right_suffix': 'Suffix added to right-hand feature names only when a non-key right feature would otherwise collide with an existing left feature name.',
        'validate': enum_note('Optional key-cardinality check for `DataFrame::unify(...)`.', [
            ('many_to_many', 'Allow duplicate key values on both sides. This is the default.'),
            ('one_to_one', 'Require each key to appear at most once on both sides.'),
            ('one_to_many', 'Require unique keys on the left and allow duplicates on the right.'),
            ('many_to_one', 'Allow duplicates on the left and require unique keys on the right.'),
        ]),
        'collision_suffix': 'Suffix added when a second frame contributes a feature name that already exists on the left-hand side.',
        'overwrite': 'When `true`, matching values from the right-hand side overwrite existing left-hand values during update-style operations.',
        'field': 'Name of the target stream in the catalog.',
        'key': 'Name of the target slot or stream.',
        'keys': 'Collection of slot or stream names to keep, drop, or rewrite together.',
        'names': 'Collection of feature, slot, or stream names supplied in positional order.',
        'features': 'Feature name(s) that select the target DataFrame columns for the operation.',
        'other': 'Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.',
        'n': 'Number of rows, entries, or depth items to keep in the preview.',
        'axis': 'Axis selector. On `DataFrame`, `0` usually means rows/cases and `1` means features/columns.',
        'skipnull': 'When `true`, missing values are skipped during the reduction. When `false`, missing state can affect the result.',
        'random_state': 'Optional seed used to make sampling or shuffling reproducible.',
        'freq': 'Bin or calendar frequency such as `"1h"`, `"D"`, `"W"`, or `"M"`.',
        'op': 'Operation key or enum that chooses the aggregation, comparison, or fill behavior for the call.',
        'step': 'Positive frequency multiplier. For example, `6` with hourly frequency means six-hour bins.',
        'render': 'When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.',
        'dropnull_keys': 'When `true`, rows with null pivot/group keys are excluded before reshaping.',
        'dropnull': 'When `true`, null grouping keys or null values are excluded where the method supports that policy. When `false`, null participates instead of being removed.',
        'top_n': 'Optional cap on how many grouped rows to keep after sorting. Use `0` to keep every group.',
        'min_count': 'Minimum non-null observations required before a grouped or rolling summary is considered valid.',
        'ddof': 'Degrees of freedom subtracted from the observation count in variance-based denominators. Use `1` for sample statistics and `0` for population statistics.',
        'ties': 'Tie policy for equal values: `Average`, `Lowest`, `Highest`, `First`, or `Dense`.',
        'nulls': 'Null ranking policy: `Keep` leaves null ranks missing, `Top` ranks them first, and `Bottom` ranks them last.',
        'percentage': 'When `true`, divide each rank by the number of ranked observations.',
        'drop_nulls': 'When `true`, missing values do not participate while the most-frequent value is selected.',
        'skip_nulls': 'When `true`, a null output remains null only at that case and later cumulative values resume; when `false`, every output after the first null is null.',
        'window': 'Positive fixed case count in each moving window.',
        'centered': 'When `true`, place each fixed-width result at the center case rather than the trailing case.',
        'alpha': 'Exponential smoothing factor in `(0, 1]`; larger values give more weight to recent observations.',
        'adjusted': 'When `true`, normalize by the finite geometric weight sum; when `false`, use the recursive form.',
        'bias_corrected': 'When `true`, apply finite-sample bias correction to exponentially weighted variance.',
        'duration_nanos': 'Positive time-window width expressed in nanoseconds. The interval is open on the left and closed on the current observation.',
        'periods': 'Number of row or time steps to shift forward or backward. Positive values usually move data down or later; negative values move it up or earlier.',
        'statement': 'SQL statement text to prepare against the current frame.',
        'figure': 'Existing figure object to add summary content into instead of creating a new figure.',
        'triangle': 'Pair-plot triangle selection controlling whether the scatter matrix renders the upper, lower, or corner-only half.',
        'points': 'Requested evaluation-point count for density-style plots. Higher values make smoother curves at higher compute cost.',
        'arg2': 'Pass `nullptr` here to request a multi-sheet Excel load result that includes every sheet.',
        'start': 'Inclusive starting position of the slice.',
        'stop': 'Exclusive ending position of the slice.',
    }
    method_param_notes = {
        ('DataFrame', 'aggregate', 'specs'):
            enum_note('Aggregation specs. Each `SummarySpec` has `output`, `feature`, and `op`.', [
                ('Count', 'Count observed values in the selected feature.'),
                ('Ndistinct', 'Count distinct observed values.'),
                ('Sum', 'Compute the numeric sum.'),
                ('Mean', 'Compute the arithmetic mean.'),
                ('Median', 'Compute the median.'),
                ('Min', 'Compute the minimum.'),
                ('Max', 'Compute the maximum.'),
                ('Std', 'Compute the standard deviation.'),
                ('Var', 'Compute the variance.'),
                ('Prod', 'Compute the numeric product.'),
            ], heading='`op` supports'),
        ('DataFrameGrouped', 'agg', 'specs'):
            enum_note('Grouped aggregation specs. Each `GroupSpec` has `output`, `feature`, and `op`.', [
                ('Count', 'Count observed values inside each group.'),
                ('Ndistinct', 'Count distinct observed values inside each group.'),
                ('Sum', 'Compute the numeric sum inside each group.'),
                ('Mean', 'Compute the arithmetic mean inside each group.'),
                ('Min', 'Compute the minimum inside each group.'),
                ('Max', 'Compute the maximum inside each group.'),
                ('Std', 'Compute the sample standard deviation inside each group.'),
                ('Var', 'Compute the sample variance inside each group.'),
            ], heading='`op` supports'),
        ('DataFrameGrouped', 'transform', 'specs'):
            enum_note('Grouped transform specs. Each `GroupSpec` has `output`, `feature`, and `op`.', [
                ('Count', 'Broadcast each group row count back to the original row space.'),
                ('Ndistinct', 'Broadcast each group distinct count back to the original row space.'),
                ('Sum', 'Broadcast each group sum back to the original row space.'),
                ('Mean', 'Broadcast each group mean back to the original row space.'),
                ('Min', 'Broadcast each group minimum back to the original row space.'),
                ('Max', 'Broadcast each group maximum back to the original row space.'),
                ('Std', 'Broadcast each group sample standard deviation back to the original row space.'),
                ('Var', 'Broadcast each group sample variance back to the original row space.'),
            ], heading='`op` supports'),
        ('DataFrame', 'corr', 'method'):
            enum_note('Correlation method.', [
                ('Pearson', 'Ordinary linear correlation.'),
                ('Spearman', 'Rank correlation.'),
                ('KendallTau', 'Rank-concordance correlation.'),
            ]),
        ('DataFrame', 'corr_with', 'other'):
            'Right-hand frame to compare against. Only common numeric features participate, and rows are aligned by the shared case axis before correlation is computed.',
        ('DataFrame', 'corr_with', 'others'):
            'Comparison frames to correlate against the current frame in one call. Each comparison frame becomes one output feature in the returned result frame.',
        ('DataFrame', 'corr_with', 'method'):
            enum_note('Correlation method used for each participating numeric feature pair.', [
                ('Pearson', 'Ordinary linear correlation on observed numeric values.'),
                ('Spearman', 'Rank correlation after converting each numeric feature to ranks.'),
                ('KendallTau', 'Rank-concordance correlation for monotonic association.'),
            ]),
        ('DataFrame', 'corr_with', 'min_count'):
            'Minimum number of pairwise-complete observations required before a correlation value is reported for a feature.',
        ('DataFrame', 'covariance', 'options'):
            '`StatisticOptions` controls `ddof` (denominator correction) and `min_count` (minimum pairwise-complete cases).',
        ('DataFrame', 'mean_error', 'options'):
            '`StatisticOptions` controls `ddof` and the minimum valid case count.',
        ('DataFrame', 'growing_variance', 'options'):
            '`StatisticOptions` controls `ddof` and the minimum valid case count.',
        ('DataFrame', 'growing_deviation', 'options'):
            '`StatisticOptions` controls `ddof` and the minimum valid case count.',
        ('DataFrame', 'moving_sum', 'options'):
            '`WindowOptions` controls `min_count`, `ddof`, and centered result placement.',
        ('DataFrame', 'moving_mean', 'options'):
            '`WindowOptions` controls `min_count`, `ddof`, and centered result placement.',
        ('DataFrame', 'moving_minimum', 'options'):
            '`WindowOptions` controls `min_count`, `ddof`, and centered result placement.',
        ('DataFrame', 'moving_maximum', 'options'):
            '`WindowOptions` controls `min_count`, `ddof`, and centered result placement.',
        ('DataFrame', 'moving_variance', 'options'):
            '`WindowOptions` controls `min_count`, `ddof`, and centered result placement.',
        ('DataFrame', 'moving_deviation', 'options'):
            '`WindowOptions` controls `min_count`, `ddof`, and centered result placement.',
        ('DataFrame', 'case_study', 'rows'):
            enum_note('Selector describing which rows `case_study(...)` should return.', [
                ('`pluss::table::pos(row)`', 'One row by zero-based position. Negative positions count from the end.'),
                ('`pluss::table::pos(start, stop)`', 'A half-open positional row slice `[start, stop)`.'),
                ('`pluss::table::pos({r1, r2, ...})`', 'An explicit row-position list in the exact output order you want.'),
                ('`pluss::table::label(value)`', 'Every row whose case label matches `value`.'),
                ('`pluss::table::labels({...})`', 'Several case labels in requested order. If one label appears multiple times, every matching row is returned.'),
                ('`pluss::table::label_range(start, stop)`', 'An inclusive case-label window from the first `start` match through the last `stop` match.'),
                ('`pluss::table::mask(bool_thread)`', 'A boolean row mask, equivalent to filtering first and then projecting columns.'),
            ]),
        ('DataFrame', 'case_study', 'features'):
            enum_note('Selector describing which feature columns `case_study(...)` should keep after resolving the row selection.', [
                ('`pluss::table::all_features()`', 'Keep every feature in the same left-to-right order as the source frame.'),
                ('`pluss::table::feature("name")`', 'Keep one named feature.'),
                ('`pluss::table::feature(position)`', 'Keep one feature by zero-based position.'),
                ('`pluss::table::features({"a", "b"})`', 'Keep several named features in the requested order.'),
                ('`pluss::table::features({0, 2})`', 'Keep several feature positions in the requested order.'),
                ('`pluss::table::features(start, stop)`', 'A half-open positional feature slice `[start, stop)`.'),
                ('`pluss::table::feature_range("start", "stop")`', 'An inclusive feature-name window from `start` through `stop`.'),
            ]),
        ('DataFrame', 'caselbl', 'name'):
            'Structured case-label name to extract from a multi-label case axis. The returned thread has one value per row in the current frame order.',
        ('DataFrame', 'caselbl', 'position'):
            'Zero-based structured case-label position to extract from a multi-label case axis. Negative positions count backward from the end.',
        ('DataFrame', 'observe', 'row'):
            'Zero-based row position to preview as a one-row `DataFrameView`.',
        ('DataFrame', 'observe', 'start'):
            'Inclusive zero-based starting row position of the preview window.',
        ('DataFrame', 'observe', 'stop'):
            'Exclusive zero-based ending row position of the preview window.',
        ('DataFrame', 'observe', 'render'):
            'When `true`, the preview may auto-render in temporary terminal contexts. Pass `false` when you want to call `.show()` yourself.',
        ('DataFrame', 'reorder', 'spec'):
            'One partial-order move. Use this when one move instruction is enough.',
        ('DataFrame', 'reorder', 'specs'):
            'Several partial-order moves applied in the exact order you pass them. Each move sees the column order produced by the previous move.',
        ('DataFrame', 'reorder', 'names'):
            'Column names to move together to the front while preserving the relative order of every column not listed here.',
        ('DataFrame', 'combine', 'spec'):
            'Combine policy object. Use it when you want explicit control over feature-wise append/update behavior instead of the default combine route.',
        ('DataFrame', 'match', 'other'):
            'Frame whose case labels and feature names should be matched with this frame.',
        ('DataFrame', 'match', 'options'):
            'Matching policy for cases, features, and repeated case labels. Defaults to a stable union on both axes and occurrence-order pairing for repeats.',
        ('DataFrame', 'extend', 'other'):
            'Frame whose cases should be appended beneath this frame.',
        ('DataFrame', 'extend', 'features'):
            'Feature matching rule for the non-extension axis. `Unite` keeps the stable union; `Common` keeps shared features.',
        ('DataFrame', 'extend', 'fresh_cases'):
            'When `true`, replace the appended case labels with contiguous integer cases starting at zero.',
        ('DataFrame', 'extend', 'reject_repeated_cases'):
            'When `true`, fail if preserving the two case axes would produce repeated case labels.',
        ('DataFrame', 'lengthen', 'id_features'):
            'Features repeated unchanged for each selected value feature.',
        ('DataFrame', 'lengthen', 'value_features'):
            'Homogeneous features stacked into the output value feature. Leave empty to use every non-identifier feature.',
        ('DataFrame', 'lengthen', 'feature_name'):
            'Output name for the Note-backed feature that identifies each source value feature.',
        ('DataFrame', 'lengthen', 'value_name'):
            'Output name for the stacked typed values.',
        ('DataFrame', 'lengthen', 'fresh_cases'):
            'When `true`, create contiguous integer cases; when `false`, repeat the original case axis for every value feature.',
        ('DataFrame', 'catalogue', 'feature'):
            'Feature whose distinct observed values should become compact levels.',
        ('DataFrame', 'catalogue', 'ordered'):
            'Records whether level order is semantically meaningful.',
        ('DataFrame', 'catalogue', 'sort_levels'):
            'When `true`, sort levels before assigning integer codes; otherwise preserve first-seen order.',
        ('DataFrame', 'sample', 'n'):
            'Number of rows or features to draw from the selected axis.',
        ('DataFrame', 'sample', 'frac'):
            'Fraction of the selected axis to draw. Values above `1.0` require `replace=true`.',
        ('DataFrame', 'sample', 'random_state'):
            'Deterministic seed for reproducible sampling and shuffling.',
        ('DataFrame', 'sample', 'replace'):
            'When `true`, the same row may be drawn more than once. Feature sampling currently requires `replace=false`.',
        ('DataFrame', 'sample', 'axis'):
            'Sampling axis. Use `0` for rows and `1` for features.',
        ('DataFrame', 'shuffle', 'random_state'):
            'Deterministic seed for reproducible row shuffles.',
        ('DataFrame', 'reshuffle', 'axis'):
            'Shuffle axis. Use `0` to permute rows and `1` to permute feature order.',
        ('DataFrame', 'reshuffle', 'random_state'):
            'Deterministic seed for reproducible row or feature shuffles.',
        ('DataFrame', 'load', 'on_bad_lines'):
            enum_note('Malformed-line policy for delimited input.', [
                ('Error', 'Stop immediately when a malformed row is encountered.'),
                ('Warn', 'Continue while reporting the malformed row.'),
                ('Skip', 'Ignore malformed rows silently.'),
            ]),
        ('DataFrame', 'load_csv', 'header'):
            enum_note('CSV header mode.', [
                ('infer', 'Read the first row as feature names.'),
                ('none', 'Treat the file as headerless and auto-generate feature names such as `feat_0`, `feat_1`, and so on.'),
            ], heading='Common values'),
        ('DataFrame', 'load_csv', 'on_bad_lines'):
            enum_note('Malformed-line policy for delimited input.', [
                ('Error', 'Stop immediately when a malformed row is encountered.'),
                ('Warn', 'Continue while reporting the malformed row.'),
                ('Skip', 'Ignore malformed rows silently.'),
            ]),
        ('DataFrame', 'pivot', 'case_feature'):
            'Feature whose distinct values become the output row groups in the pivoted result.',
        ('DataFrame', 'pivot_table', 'case_feature'):
            'Feature whose distinct values become the output row groups in the pivoted summary result.',
        ('DataFrame', 'place', 'name'):
            'Feature name to insert or move.',
        ('DataFrame', 'place', 'feature'):
            'Full thread payload to insert at the target position. Its length must match the frame case count.',
        ('DataFrame', 'put', 'feature'):
            'Feature name to rewrite.',
        ('DataFrame', 'put_', 'feature'):
            'Feature name to rewrite in place.',
        ('DataFrame', 'put', 'value'):
            'Replacement scalar, string, null, or thread written only where the mask is `true`.',
        ('DataFrame', 'put_', 'value'):
            'Replacement scalar, string, null, or thread written in place only where the mask is `true`.',
        ('DataFrame', 'query', 'expression'):
            'Boolean filter expression written against feature names, such as `"sales > 95"` or `"city == \\"Accra\\""`. Rows where the expression evaluates true are kept.',
        ('DataFrame', 'rename', 'old_name'):
            'Existing feature name to rename.',
        ('DataFrame', 'rename', 'new_name'):
            'Replacement feature name.',
        ('DataFrame', 'rename_', 'old_name'):
            'Existing feature name to rename in place.',
        ('DataFrame', 'rename_', 'new_name'):
            'Replacement feature name written back in place.',
        ('DataFrame', 'rename', 'renames'):
            'Collection of `(old_name, new_name)` pairs applied in one rename pass.',
        ('DataFrame', 'rename', 'feature_names'):
            'Full replacement feature-label list in final left-to-right order. The count must match the current number of features.',
        ('DataFrame', 'set_axis', 'feature_names'):
            'Full replacement feature-label list in final left-to-right order. The count must match the current number of features.',
        ('DataFrame', 'set_axis', 'feature_names'):
            'Full replacement feature-label list in final left-to-right order. The count must match the current number of features.',
        ('DataFrame', 'replace', 'from'):
            'Value to search for inside the selected compatible features. Its type decides which feature kinds participate.',
        ('DataFrame', 'replace', 'to'):
            'Replacement value written anywhere `from` matches. Use `pluss::null` when you want matched cells to become missing.',
        ('DataFrame', 'replace', 'features'):
            'Optional feature subset. Leave empty to apply the replacement across every compatible feature of the relevant kind.',
        ('DataFrame', 'rolling', 'specs'):
            enum_note('Rolling feature specs. Each `WindowSpec` has these fields.', [
                ('`output`', 'Name of the derived feature to create.'),
                ('`feature`', 'Source numeric feature to read from.'),
                ('`op`', 'Rolling statistic such as `RollingMean`, `RollingSum`, `RollingMin`, or `RollingMax`.'),
                ('`window`', 'Fixed trailing window width in cases.'),
                ('`min_periods`', 'Minimum observed values required before the output becomes valid.'),
            ]),
        ('DataFrame', 'rolling', 'keep_input'):
            'When `true`, the original features stay in the output and rolling features are appended. When `false`, only the derived rolling outputs are returned.',
        ('DataFrame', 'scan', 'statistic'):
            enum_note('Cumulative statistic to compute down each numeric feature.', [
                ('Sum', 'Running sum from the first row through the current row.'),
                ('Product', 'Running product from the first row through the current row.'),
                ('Minimum', 'Running minimum seen so far.'),
                ('Maximum', 'Running maximum seen so far.'),
            ]),
        ('DataFrame', 'select_like', 'needle'):
            'Substring token used to match feature names.',
        ('DataFrame', 'select_like', 'case_sensitive'):
            'When `true`, substring matching respects letter case. When `false`, feature names are matched case-insensitively.',
        ('DataFrame', 'sort_values', 'ascending'):
            'When you use the simple feature-list overload, this one switch applies to every sort key. `true` sorts ascending and `false` sorts descending.',
        ('DataFrame', 'sort_values', 'nulls_last'):
            'When `true`, null values are placed after observed values for every sort key in the simple overload. When `false`, nulls are placed first.',
        ('DataFrame', 'prod', 'axis'):
            'Reduction axis. `Axis::Feature` computes one product per numeric feature, while `Axis::Case` computes one product per row across numeric features.',
        ('DataFrame', 'prod', 'render'):
            'When `true`, the summary view can auto-render in temporary terminal contexts. Pass `false` when you want the summary object without implicit display.',
        ('DataFrame', 'sql', 'options'):
            'SQL execution options such as engine or binding policy for the returned SQL accessor.',
        ('DataFrame', 'sort_values', 'features'):
            'Feature names used as sort keys when you want the same ascending/null policy for every key.',
        ('DataFrame', 'label().sort', 'ascending'):
            'When `true`, case labels are sorted ascending. When `false`, they are sorted descending.',
        ('DataFrame', 'label().sort', 'nulls_last'):
            'When `true`, null case labels are placed after observed labels. When `false`, they are placed first.',
        ('DataFrame', 'tail', 'n'):
            'Number of trailing rows to keep from the bottom of the frame.',
        ('DataFrame', 'title', 'title'):
            'Human-readable title stamp to attach to the returned frame.',
        ('DataFrame', 'title_', 'title'):
            'Human-readable title stamp written back onto the current frame.',
        ('DataFrame', 'shift', 'periods'):
            'Number of rows to shift selected feature values. Positive values move values downward and introduce missing entries at the top; negative values move upward.',
        ('DataFrame', 'transform', 'fn'):
            'Thread-to-thread or frame-to-thread transform function, depending on the overload. The returned thread must match the frame case count.',
        ('DataFrame', 'transform', 'feature'):
            'Target feature name to rewrite, or output feature name when you use the frame-aware derive overload.',
        ('DataFrame', 'truediv', 'other'):
            'Scalar divisor or right-hand frame used for elementwise true division.',
        ('DataFrame', 'ts', 'feature'):
            'Feature name to bind into the returned time-series accessor.',
        ('DataFrame', 'ts', 'features'):
            'Feature names to bind into the returned time-series accessor.',
        ('DataFrame', 'round', 'unit'):
            'Temporal rounding unit such as `"hour"`, `"day"`, or `"month"`.',
        ('DataFrame', 'unstack', 'caselbl'):
            'Structured case label whose distinct values should become new output columns in the reshaped frame.',
        ('DataFrame', 'unstack', 'options'):
            enum_note('Unstack behavior bundle.', [
                ('`caselbl`', 'Structured case label whose distinct values are pivoted into output columns.'),
                ('`drop`', 'When `true`, the remaining case labels stay as case labels instead of also staying as ordinary columns.'),
                ('`sort`', 'When `true`, the pivoted column labels are sorted before the new columns are built.'),
                ('`separator`', 'Separator inserted between the source value-feature name and the pivoted case-label value when several value features are present.'),
            ]),
        ('DataFrame', 'unify', 'right'):
            'Right-hand frame whose rows are matched against the left frame by the requested key features.',
        ('DataFrame', 'unique_cases', 'features'):
            'Optional feature subset used to decide which rows count as duplicates. Leave empty to compare the full row payload.',
        ('DataFrame', 'unsqueeze', 'depth_labels'):
            'Single depth label to attach to the one-frame atlas produced by `unsqueeze(...)`.',
        ('DataFrame', 'view', 'start'):
            'Slice start position. Negative values count backward from the end of the frame.',
        ('DataFrame', 'view', 'stop'):
            'Slice stop position. Negative values count backward from the end, and `-1` includes the last row.',
        ('DataFrame', 'where', 'bool_mask'):
            'Boolean row mask. Rows where the mask is true are kept; rows where it is false are dropped.',
        ('DataFrame', 'window', 'keep_input'):
            'When `true`, the original input features are preserved and engineered window outputs are appended. When `false`, only the engineered outputs are returned.',
        ('DataFrame', 'window', 'specs'):
            enum_note('Window feature specs. Each `WindowSpec` has these fields.', [
                ('`output`', 'Name of the derived feature to create.'),
                ('`feature`', 'Source numeric feature to read from.'),
                ('`op`', 'Window statistic such as `RollingMean`, `RollingSum`, `RollingMin`, or `RollingMax`.'),
                ('`window`', 'Fixed trailing window width in cases.'),
                ('`min_periods`', 'Minimum observed values required before the output becomes valid.'),
            ]),
        ('ColumnRef', 'name', 'None'):
            '',
        ('ColumnRef', 'head', 'n'):
            'Number of leading values to preview from the referenced feature.',
        ('DataFramePlotAccessor', 'line', 'y_feature'):
            'One numeric feature to draw as a single line trace.',
        ('DataFramePlotAccessor', 'line', 'y_features'):
            'Numeric features to overlay as separate line traces on the same axes.',
        ('DataFramePlotAccessor', 'step', 'y_feature'):
            'One numeric feature to draw as a step trace.',
        ('DataFramePlotAccessor', 'step', 'y_features'):
            'Numeric features to overlay as separate step traces on the same axes.',
        ('DataFramePlotAccessor', 'area', 'y_feature'):
            'One numeric feature to draw as a filled area.',
        ('DataFramePlotAccessor', 'area', 'y_features'):
            'Numeric features to draw as separate filled areas over the same axis positions.',
        ('DataFramePlotAccessor', 'area_stacked', 'y_features'):
            'Numeric features whose values should accumulate vertically at shared x positions.',
        ('DataFramePlotAccessor', 'scatter', 'x_feature'):
            'Numeric or ordered feature placed on the horizontal axis.',
        ('DataFramePlotAccessor', 'scatter', 'y_feature'):
            'Numeric feature placed on the vertical axis.',
        ('DataFramePlotAccessor', 'bar', 'y_feature'):
            'Single feature whose values become one bar series.',
        ('DataFramePlotAccessor', 'bar', 'y_features'):
            'Several features whose values become grouped or layered bar series, depending on the method variant.',
        ('DataFramePlotAccessor', 'bar_stacked', 'y_features'):
            'Several features whose values stack on top of each other at each x position.',
        ('DataFramePlotAccessor', 'box', 'features'):
            'Numeric features to summarize as box plots. Leave empty to let the accessor infer the numeric feature set.',
        ('DataFramePlotAccessor', 'violin', 'features'):
            'Numeric features to summarize as violin plots. Leave empty to let the accessor infer the numeric feature set.',
        ('DataFramePlotAccessor', 'violin', 'value_feature'):
            'Numeric feature whose distribution is shown inside each violin.',
        ('DataFramePlotAccessor', 'violin', 'group_feature'):
            'Categorical feature that splits the violin plot into separate groups.',
        ('DataFramePlotAccessor', 'hist', 'bins'):
            'Histogram bin count. Larger values create narrower bins; smaller values create coarser buckets.',
        ('DataFramePlotAccessor', 'hist', 'feature'):
            'One numeric feature to histogram.',
        ('DataFramePlotAccessor', 'hist', 'features'):
            'Several numeric features to histogram in the same figure.',
        ('DataFramePlotAccessor', 'density', 'feature'):
            'One numeric feature whose kernel density estimate should be drawn.',
        ('DataFramePlotAccessor', 'density', 'features'):
            'Several numeric features whose kernel density estimates should be drawn together.',
        ('DataFramePlotAccessor', 'pie', 'value_feature'):
            'Feature whose values determine each slice magnitude.',
        ('DataFramePlotAccessor', 'pie', 'label_feature'):
            'Optional feature that supplies the text label for each slice.',
        ('DataFramePlotAccessor', 'heatmap', 'features'):
            'Numeric features to render directly as a value heatmap. Each feature becomes one column in the heatmap matrix.',
        ('DataFramePlotAccessor', 'heatmap', 'case_feature'):
            'Feature whose distinct values define the output heatmap rows in grouped heatmap mode.',
        ('DataFramePlotAccessor', 'heatmap', 'feature_feature'):
            'Feature whose distinct values define the output heatmap columns in grouped heatmap mode.',
        ('DataFramePlotAccessor', 'heatmap', 'value_feature'):
            'Feature whose values are aggregated into each grouped heatmap cell.',
        ('DataFramePlotAccessor', 'corr_heatmap', 'features'):
            'Numeric features to correlate. Leave empty to let the accessor infer the numeric feature set.',
        ('DataFramePlotAccessor', 'scatter_matrix', 'features'):
            'Numeric features to place on the pairwise matrix axes. Leave empty to let the accessor infer them.',
        ('DataFramePlotAccessor', 'parallel', 'features'):
            'Numeric features to place on the parallel-coordinate axes. Leave empty to let the accessor infer them.',
        ('DataFramePlotAccessor', 'andrews_curves', 'features'):
            'Numeric features used to build each observation curve.',
        ('DataFramePlotAccessor', 'radviz', 'features'):
            'Numeric features used as anchors in the radial visualization.',
        ('DataFramePlotAccessor', 'lag', 'feature'):
            'Numeric feature whose lag relationship should be plotted against itself.',
        ('DataFramePlotAccessor', 'act', 'feature'):
            'Numeric feature whose autocorrelation-style lag structure should be plotted.',
        ('DataFramePlotAccessor', 'bootstrap', 'feature'):
            'Numeric feature to resample repeatedly for the bootstrap figure.',
        ('DataFramePlotAccessor', 'brief', 'features'):
            'Feature subset to summarize inside the compact figure. Leave empty to let the accessor choose a broad compatible set.',
        ('DataFramePlotAccessor', 'auto_plot', 'features'):
            'Feature subset to consider while the accessor chooses the plot family automatically.',
        ('DataFrameSqlAccessor', 'execute', 'statement'):
            'SQL query text run against the current frame. The frame is exposed to the statement as `self`.',
        ('DataFrameSqlAccessor', 'prepare', 'statement'):
            'Parameterized SQL statement template. Use placeholders such as `{min_sales}` when you plan to bind values later.',
        ('DataFrameSqlPrepared', 'execute', 'params'):
            'Named scalar bindings supplied to placeholders such as `{min_sales}` inside the prepared statement template.',
        ('DataFrameSqlPrepared', 'execute', 'identifier_params'):
            'Named identifier bindings used for placeholder substitution when a statement needs a dynamic SQL identifier such as a table alias.',
    }
    type_notes = {
        'pluss::loader::BadLinePolicy':
            enum_note('', [
                ('Error', 'Stop on malformed lines.'),
                ('Warn', 'Keep going while reporting the issue.'),
                ('Skip', 'Silently ignore malformed lines.'),
            ]),
        'CorrMethod':
            enum_note('', [
                ('Pearson', 'Ordinary linear correlation.'),
                ('Spearman', 'Rank correlation.'),
                ('KendallTau', 'Rank-concordance correlation.'),
            ]),
        'ScanStatistic':
            enum_note('', [
                ('Sum', 'Compute a cumulative sum down the feature.'),
                ('Product', 'Compute a cumulative product down the feature.'),
                ('Minimum', 'Compute the running minimum down the feature.'),
                ('Maximum', 'Compute the running maximum down the feature.'),
            ]),
        'SummaryOp':
            enum_note('', [
                ('Count', 'Count observed values.'),
                ('Ndistinct', 'Count distinct observed values.'),
                ('Sum', 'Compute the numeric sum.'),
                ('Mean', 'Compute the arithmetic mean.'),
                ('Median', 'Compute the median.'),
                ('Min', 'Compute the minimum.'),
                ('Max', 'Compute the maximum.'),
                ('Std', 'Compute the standard deviation.'),
                ('Var', 'Compute the variance.'),
                ('Prod', 'Compute the numeric product.'),
            ]),
        'GroupOp':
            enum_note('', [
                ('Count', 'Count observed values in each group.'),
                ('Ndistinct', 'Count distinct observed values in each group.'),
                ('Sum', 'Compute the numeric sum in each group.'),
                ('Mean', 'Compute the arithmetic mean in each group.'),
                ('Min', 'Compute the minimum in each group.'),
                ('Max', 'Compute the maximum in each group.'),
                ('Std', 'Compute the sample standard deviation in each group.'),
                ('Var', 'Compute the sample variance in each group.'),
            ]),
        'MissingOp':
            enum_note('', [
                ('Fill', 'Replace missing values with a supplied fill value.'),
                ('ForwardFill', 'Propagate the last observed value forward.'),
                ('BackwardFill', 'Propagate the next observed value backward.'),
                ('DropAny', 'Drop rows containing any missing value in the targeted features.'),
                ('DropAll', 'Drop rows only when all targeted features are missing.'),
                ('IsNull', 'Return a missingness mask.'),
                ('NotNull', 'Return a non-missingness mask.'),
            ]),
        'CombineOp':
            enum_note('', [
                ('Features', 'Append compatible right-hand features onto the left frame.'),
                ('Update', 'Overwrite matching left-hand values with right-hand values.'),
            ]),
        'ReorderPlacement':
            enum_note('', [
                ('Front', 'Move the selected features to the front.'),
                ('Back', 'Move the selected features to the back.'),
                ('Before', 'Move the selected features immediately before the anchor feature.'),
                ('After', 'Move the selected features immediately after the anchor feature.'),
            ]),
        'ThreadWindowOp':
            enum_note('', [
                ('RollingSum', 'Compute a rolling sum.'),
                ('RollingMean', 'Compute a rolling mean.'),
                ('RollingMin', 'Compute a rolling minimum.'),
                ('RollingMax', 'Compute a rolling maximum.'),
            ]),
        'ScatterMatrixTriangle':
            enum_note('Selects which half of the pair plot to render.', [
                ('Full', 'Render the full pairwise matrix.'),
                ('Lower', 'Render only the lower triangle.'),
                ('Upper', 'Render only the upper triangle.'),
            ]),
        'pluss::msharp::RangeView::Of<SummarySpec>':
            'Collection of `SummarySpec` items. Each item provides `output`, `feature`, and `op`.',
        'pluss::msharp::RangeView::Of<GroupSpec>':
            'Collection of `GroupSpec` items. Each item provides `output`, `feature`, and `op`.',
        'pluss::msharp::RangeView::Of<DeriveSpec>':
            'Collection of `DeriveSpec` items. Each item provides the output feature name and a frame-aware expression that returns the derived thread.',
        'pluss::msharp::RangeView::Of<CastSpec>':
            'Collection of `CastSpec` items. Each item selects a feature, the target payload family, and any target Series dtype details needed for the cast.',
        'pluss::msharp::RangeView::Of<TransformSpec>':
            'Collection of `TransformSpec` items. Each item identifies one target feature and the thread-transform expression to apply to it.',
        'pluss::msharp::RangeView::Of<ArrangeSpec>':
            'Collection of `ArrangeSpec` items. Each item provides `feature`, `ascending`, and `nulls_last`.',
        'pluss::RangeView<DataFrame>':
            'Reusable view over several comparison frames passed together in one call.',
        'pluss::RangeView<ReorderSpec>':
            'Collection of `ReorderSpec` items applied sequentially from left to right. Use this overload when you want to move several column groups in one call while preserving every untouched column.',
        'pluss::RangeView<WindowSpec>':
            'Collection of `WindowSpec` items. Each item provides `output`, `feature`, `op`, `window`, and `min_periods` for one derived window feature.',
        'pluss::msharp::RangeView::Of<std::pair<std::string_view, pluss::codem::ScalarType>>':
            'Collection of `(feature_name, dtype)` pairs used to cast several existing features to explicit scalar dtypes in one call.',
        'pluss::msharp::RangeView::Of<std::pair<std::string, Thread>>':
            'Collection of `(feature_name, thread)` pairs used to add or replace several features in one call.',
        'const CombineSpec&':
            'Structured combine policy object. It controls how the right-hand frame should be merged, appended, or used to update the left-hand frame.',
        'const NameRange&':
            'Reusable feature-name range object. Use it when the same projected column subset will be passed to several calls.',
        'const ReorderSpec&':
            'Structured partial-order instruction that moves only selected features and preserves the relative order of every untouched feature.',
        'const UnstackOptions&':
            'Structured reshape policy that selects the case label to pivot, whether remaining labels stay as row labels, whether pivoted columns are sorted, and how multi-value output names are separated.',
    }
    lines = []
    for name, typ in params:
        label_type = public_cpp_type(typ).replace('double', 'scalar')
        note = method_param_notes.get((class_name, method_name, name),
                                      param_notes.get(
                                          name,
                                          type_notes.get(typ, type_notes.get(label_type, fallback_param_note(name, label_type))),
                                      ))
        lines.append(f'**{name}**\n{indent_note(note)}')
    return '\n\n'.join(lines)


def generic_note(class_name: str, method: Method) -> str:
    name = method.name
    method_notes = {
        'DataFrame': {
            'aggregate': 'Builds a compact summary frame from one or more aggregation specs, so you can inspect computed metrics instead of the full row set.',
            'bottom': 'Returns the rows with the smallest values in the selected feature.',
            'bucketize': 'Buckets numeric `Series` features into interval labels while preserving the rest of the frame unchanged.',
            'bounded': 'Computes variable-width window statistics where each row chooses its own bounds through a callback.',
            'case_study': 'Selects cases and features from a `DataFrame` using explicit positional, label-based, or boolean-mask selector objects. It is the single case/feature indexing surface for table selection.',
            'catalogue': 'Builds a compact level-and-code encoding for a low-cardinality feature while preserving missingness.',
            'caselbl': 'Returns the current case axis itself. Use this when you want the row labels as data rather than selecting rows from the frame.',
            'combine': 'Combines this frame with another frame according to the supplied combine policy, such as feature-wise update or join-like alignment.',
            'corr': 'Computes a correlation matrix across numeric features, optionally for a selected subset and a selected correlation method.',
            'corr_with': 'Computes aligned feature-wise correlations between this frame and one or more other frames.',
            'covar': 'Computes the pairwise covariance matrix across numeric features.',
            'covariance': 'Computes the pairwise covariance matrix for numeric features using pairwise-complete observations.',
            'percentiles': 'Computes requested numeric distribution points for every numeric feature.',
            'csmax': 'Computes one row-wise maximum across the selected numeric features.',
            'csmin': 'Computes one row-wise minimum across the selected numeric features.',
            'drop_duplicates': 'Removes duplicate rows, optionally considering only a selected feature subset.',
            'label().drop': 'Removes one or more named case labels from a multi-label case axis.',
            'dropnull': 'Drops rows according to an explicit missing-data policy object.',
            'dtypes': 'Returns the public dtype summary for each current feature.',
            'duplicated': 'Builds a boolean mask marking which rows are duplicates of earlier or later rows under the selected keep policy.',
            'most_frequent': 'Returns the most frequently occurring value in every supported feature.',
            'factor': 'Factorizes selected features into compact integer codes together with the corresponding observed levels.',
            'flow': 'Computes a cumulative directional summary across numeric features for each row.',
            'fold': 'Reduces rows into one or more summary values using a folding-style aggregation route.',
            'freq': 'Counts the most frequent values or value-combinations across the selected features.',
            'ranks': 'Assigns numeric ranks while controlling tie, null, direction, and percentage behavior.',
            'asymmetry': 'Computes bias-corrected sample skewness across cases or features.',
            'tail_weight': 'Computes bias-corrected Fisher excess kurtosis across cases or features.',
            'mean_error': 'Computes the standard error of the mean with configurable degrees of freedom and minimum observations.',
            'running_sum': 'Computes a cumulative sum down each numeric feature.',
            'running_product': 'Computes a cumulative product down each numeric feature.',
            'running_minimum': 'Computes the minimum observed so far down each numeric feature.',
            'running_maximum': 'Computes the maximum observed so far down each numeric feature.',
            'correlate_with': 'Computes one aligned correlation value for every numeric feature shared with another frame.',
            'matrix_product': 'Multiplies the numeric matrix represented by this frame with the numeric matrix represented by another frame.',
            'moving_sum': 'Computes fixed-width trailing sums over every numeric feature.',
            'moving_mean': 'Computes fixed-width trailing means over every numeric feature.',
            'moving_minimum': 'Computes fixed-width trailing minima over every numeric feature.',
            'moving_maximum': 'Computes fixed-width trailing maxima over every numeric feature.',
            'moving_variance': 'Computes fixed-width trailing variance with configurable degrees of freedom.',
            'moving_deviation': 'Computes fixed-width trailing standard deviation with configurable degrees of freedom.',
            'moving_mean_error': 'Computes fixed-width trailing standard error with configurable degrees of freedom.',
            'moving_asymmetry': 'Computes fixed-width bias-corrected skewness.',
            'moving_tail_weight': 'Computes fixed-width bias-corrected Fisher excess kurtosis.',
            'growing_sum': 'Computes sums over windows that grow from the first case through the current case.',
            'growing_mean': 'Computes means over windows that grow from the first case through the current case.',
            'growing_minimum': 'Computes minima over windows that grow from the first case through the current case.',
            'growing_maximum': 'Computes maxima over windows that grow from the first case through the current case.',
            'growing_variance': 'Computes variance over windows that grow from the first case through the current case.',
            'growing_deviation': 'Computes standard deviation over windows that grow from the first case through the current case.',
            'growing_mean_error': 'Computes standard error over windows that grow from the first case through the current case.',
            'growing_asymmetry': 'Computes skewness over windows that grow from the first case through the current case.',
            'growing_tail_weight': 'Computes excess kurtosis over windows that grow from the first case through the current case.',
            'exponential_mean': 'Computes an exponentially weighted mean over each numeric feature.',
            'exponential_variance': 'Computes exponentially weighted variance over each numeric feature.',
            'exponential_deviation': 'Computes exponentially weighted standard deviation over each numeric feature.',
            'grouped_moving_mean': 'Computes moving means independently within each group while preserving original case order.',
            'grouped_moving': 'Computes a selected moving statistic independently within each group while preserving original case order.',
            'time_moving_mean': 'Computes moving means over a nanosecond-duration window on a monotonic temporal feature.',
            'time_moving': 'Computes a selected statistic over a nanosecond-duration window on a monotonic temporal feature.',
            'copy': 'Returns a detached copy of the frame so later edits to the result do not affect the source object.',
            'count_by': 'Groups by one feature and returns per-group row counts.',
            'describe': 'Returns descriptive summary statistics for the selected numeric features.',
            'drop': 'Removes one or more features and returns the remaining frame.',
            'extend': 'Appends another frame beneath this frame after matching the feature schemas and null-filling one-sided features.',
            'equals': 'Checks whether two frames have the same structure, labels, and values.',
            'feature_ref': 'Returns a lightweight reference object for one feature so you can inspect metadata or switch into column-oriented operations without materializing a new frame.',
            'group': 'Opens the grouped workspace builder so you can choose keys and then aggregate, transform, or filter by group.',
            'has_feature': 'Checks whether a feature name exists in the frame.',
            'has_feature': 'Checks whether a feature name exists in the frame.',
            'head': 'Returns the first `n` rows of the frame.',
            'height': 'Returns the row count of the current frame.',
            'iat': 'Returns one cell as text using positional row and positional feature coordinates.',
            'interp': 'Interpolates missing numeric values across selected features using the requested interpolation strategy.',
            'with_cases': 'Replaces the current case axis with the supplied thread. The row values stay in the same order, but later case-aware operations now use the new labels.',
            'with_cases_': 'Replaces the current case axis in place. The row values stay in the same order, but later case-aware operations now use the new labels.',
            'insert': 'Inserts a new feature at the requested position and shifts the later features to the right.',
            'join': 'Adds compatible features from another frame, aligning rows by the shared case axis.',
            'kurtosis': 'Computes row-wise or column-wise sample kurtosis for numeric data.',
            'lengthen': 'Reshapes a wide frame into long form by tiling identifier features and stacking homogeneous value features.',
            'match': 'Returns a pair of frames whose cases and features have been matched according to explicit Pluss-native matching rules.',
            'matmul': 'Treats the numeric part of the frame as a matrix and returns the matrix product against another numeric frame.',
            'mask': 'Keeps original values where a boolean mask is false and substitutes new values where the mask is true.',
            'median': 'Computes per-feature medians for the selected numeric features.',
            'ncases': 'Returns the number of rows in the current frame.',
            'ndistinct': 'Counts distinct observed values or distinct value-combinations across the selected features.',
            'nfeatures': 'Returns the number of features in the current frame.',
            'nt': 'Opens the note/text surface for the current selected DataFrame. Every selected feature must be Note-backed; select text feature(s) first with `df.features({...}).nt()`.',
            'observe': 'Returns a `DataFrameView` over a positional row window so you can preview rows from the middle of a frame without materializing a new working copy.',
            'label().align': 'Reorders rows so their case labels match the supplied `new_label` order. Every requested case must already exist, and the output case axis is rewritten to that order.',
            'label().sort': 'Sorts rows by the current case-label axis rather than by a data feature.',
            'add_prefix': 'Prepends the supplied text to every feature name. Row values, row order, and case labels stay unchanged.',
            'add_suffix': 'Appends the supplied text to every feature name. Row values, row order, and case labels stay unchanged.',
            'max': 'Computes per-feature maxima for the selected numeric features.',
            'mean': 'Computes per-feature means for the selected numeric features.',
            'mean_by': 'Groups by one feature and returns the mean of another feature within each group.',
            'min': 'Computes per-feature minima for the selected numeric features.',
            'note_features': 'Returns only the note-backed features.',
            'pivot': 'Reshapes long-form rows into a wider table indexed by one feature and expanded by another.',
            'pivot_table': 'Builds a pivoted summary table using an aggregation function when multiple rows map to the same cell.',
            'place': 'Moves one existing feature relative to an anchor feature, or inserts a brand-new feature directly before or after that anchor.',
            'plot3d': 'Opens the 3D plotting accessor for the current frame.',
            'prod': 'Computes per-feature or per-row products across numeric data.',
            'put': 'Writes a replacement value only where a boolean mask is true and returns the updated frame.',
            'put_': 'Writes a replacement value only where a boolean mask is true and mutates the frame in place.',
            'query': 'Filters rows using an expression written against feature names.',
            'rank': 'Assigns numeric ranks to each compatible feature while controlling tie, null, order, and percentage policy.',
            'matrix': 'Packs same-kind features into matrix-shaped pluss blocks so lower-level numeric, text, or temporal kernels can consume them efficiently.',
            'mode': 'Returns the most frequently observed value for each compatible feature.',
            'rename': 'Renames one feature without changing its values.',
            'rename_': 'Renames one feature in place without changing its values.',
            'rename': 'Renames one feature, several features, or the full feature axis without changing row values.',
            'label().rename': 'Renames one or more structured case labels without changing row order or row values.',
            'reorder': 'Moves selected features into a new relative position while preserving row data and the untouched feature order.',
            'label().order': 'Reorders the current structured case labels and recomputes the composite row labels from that new order.',
            'label().repeated': 'Returns a boolean mask whose true values mark duplicate case labels.',
            'replace': 'Replaces matched scalar or pluss-typed values across selected compatible features and returns the updated frame.',
            'rolling': 'Computes rolling-window features and returns them as a derived frame.',
            'reshuffle': 'Randomly permutes rows or features, depending on the selected axis.',
            'se': 'Computes row-wise or column-wise standard error summaries for numeric data.',
            'sample': 'Returns a random sample of rows.',
            'scan': 'Computes cumulative numeric statistics such as running sums or running maxima down each feature.',
            'save_csv': 'Writes the frame to CSV. Set `add_cases=true` when you want the current case axis exported as a regular column so row labels survive the round trip.',
            'save_excel': 'Writes the frame to an Excel sheet. Set `add_cases=true` when you want the current case axis exported as a regular column.',
            'save_json': 'Writes the frame to JSON using the table export route for later reload or interchange.',
            'save_parquet': 'Writes the frame to Parquet for compact typed storage.',
            'save_txt': 'Writes the frame to a plain-text table preview suitable for inspection or lightweight logging.',
            'save_as': 'Writes the frame using the explicitly supplied output format selector rather than a dedicated saver overload.',
            'select_like': 'Selects only the features whose names contain the requested substring.',
            'select': 'Projects a selected feature subset into a new frame while preserving row order and case alignment.',
            'series_features': 'Returns only the numeric series-backed features.',
            'assign': 'Adds or replaces derived features computed from the current frame. Use this when the new column depends on one or more existing columns.',
            'set': 'Opens an assignment helper so you can write `df.set(...) = ...` and replace or create features declaratively.',
            'set_axis': 'Replaces the full feature label set with the supplied names in positional order.',
            'set_axis': 'Replaces the full feature label set with the supplied names in positional order.',
            'set_features': 'Adds or replaces one or more features and returns the updated frame.',
            'set_features_': 'Adds or replaces one or more features in place.',
            'slice': 'Returns a contiguous row slice `[start, stop)` from the frame.',
            'shuffle': 'Randomly permutes row order while keeping the feature axis unchanged.',
            'sort_values': 'Reorders rows using one or more feature values as sort keys.',
            'skew': 'Computes row-wise or column-wise skewness for numeric data.',
            'stack': 'Pivots the current feature axis into long form by creating feature/value columns.',
            'std': 'Computes per-feature standard deviations for the selected numeric features.',
            'sum': 'Computes per-feature sums for the selected numeric features.',
            'tail': 'Returns the last `n` rows of the frame.',
            'take': 'Returns the rows at the requested integer positions.',
            'tally': 'Counts how many rows fall into each distinct value of the grouping feature.',
            'tabelize': 'Converts a Kindle `Catalog` into a tabular `DataFrame`, preserving stream alignment as rows.',
            'title': 'Gets the current frame title or returns a titled copy, depending on the overload.',
            'title_': 'Overwrites the current frame title in place.',
            'top': 'Returns the rows with the largest values in the selected feature.',
            'transform': 'Rewrites one feature, several features, or all features by applying a user function to each targeted thread.',
            'transpose': 'Swaps rows and features so former feature names become row labels.',
            'truediv': 'Applies elementwise true division to compatible numeric features.',
            'ts': 'Returns the time-series accessor bound to one or more selected features.',
            'unique_cases': 'Keeps only the first occurrence of each case label and drops later duplicates.',
            'unify': 'Combines two frames by key values. Use `right_on=""` when both frames use the same key name, Pluss-native modes such as `"common"` or `"unite"` to choose retained keys, and `validate` for duplicate-key checks.',
            'unstack': 'Pivots one structured case label out of a multi-label case axis so its distinct values become new output columns.',
            'unsqueeze': 'Promotes the frame into a depth-1 `Atlas` so it can participate in multi-frame workflows.',
            'query': 'Keeps only the rows that satisfy the supplied filter expression or mask-producing query.',
            'where': 'Keeps only the rows where the supplied boolean mask is true.',
            'filter': 'Keeps only the rows where the supplied boolean mask is true.',
            'conform': 'Reorders and aligns this frame so it is compatible with another frame for downstream joins or arithmetic.',
            'update': 'Overlays matching features from another frame onto this frame using aligned cases.',
            'var': 'Computes per-feature variances for the selected numeric features.',
            'view': 'Returns a lightweight row view of the requested slice.',
            'weigh': 'Computes weighted trailing window statistics over numeric features using an explicit weight vector.',
            'window': 'Computes one or more window-based features such as rolling means over selected columns.',
            'set_features': 'Returns a new frame with one added or replaced feature.',
            'set_features_': 'Adds or replaces one feature in place.',
        },
        'Catalog': {
            'apply': 'Applies a function to one named stream and returns the updated catalog.',
            'apply_': 'Applies a function to one named stream in place.',
            'filter': 'Keeps only the rows where the supplied mask stream is true.',
            'from_json': 'Builds a catalog from JSON input using the PlussMic JSON loader route.',
            'has': 'Checks whether a named stream exists.',
            'head': 'Returns the first `n` rows across every stream in the catalog.',
            'map': 'Applies the same function to every stream and returns the updated catalog.',
            'map_': 'Applies the same function to every stream in place.',
            'max': 'Computes the maximum value of one named numeric stream.',
            'mean': 'Computes the mean value of one named numeric stream.',
            'min': 'Computes the minimum value of one named numeric stream.',
            'query': 'Keeps only the rows selected by the supplied row-wise query function or expression.',
            'rename_': 'Renames one or more streams in place without changing their values.',
            'sample': 'Returns a random sample of rows across all streams.',
            'set': 'Adds or replaces one named stream and returns the updated catalog.',
            'set_': 'Adds or replaces one named stream in place.',
            'slice': 'Returns a contiguous row slice `[start, stop)` across every stream.',
            'sum': 'Computes the sum of one named numeric stream.',
            'tail': 'Returns the last `n` rows across every stream in the catalog.',
            'take': 'Returns the rows at the requested integer positions across every stream.',
            'transform': 'Rewrites one stream, several streams, or all streams by applying a user function.',
            'select': 'Keeps only the named streams and preserves their shared row alignment.',
            'drop': 'Removes the named streams and keeps the remaining catalog structure intact.',
            'rename': 'Renames one or more streams without changing their values.',
            'sort': 'Reorders rows using one or more sort-key streams, while keeping all other streams aligned to the same row order.',
            'group': 'Returns the grouping accessor for keyed catalog splits.',
            'merge': 'Combines two catalogs by matching rows on a key stream.',
            'save_csv': 'Writes the catalog to CSV after aligning its streams into a table-like representation.',
            'save_excel': 'Writes the catalog to an Excel workbook after aligning its streams into a table-like representation.',
            'save_json': 'Writes the catalog to JSON for interchange or later reload.',
            'save_parquet': 'Writes the catalog to Parquet for compact typed storage.',
            'save_txt': 'Writes a text rendering of the catalog for inspection or logging.',
            'save_as': 'Writes the catalog using the explicitly supplied output format selector.',
            'where': 'Keeps only the rows where the supplied mask stream is true.',
            'clean': 'Cleans one named text stream and returns the updated catalog.',
            'keywords': 'Extracts high-frequency keywords from one text stream.',
            'similarity': 'Computes row-wise text similarity between two named text streams.',
            'grep': 'Keeps only the rows where one named text stream matches the pattern.',
            'featurize': 'Adds derived text features from one named text stream.',
            'stats': 'Computes one text summary statistic for a named text stream.',
            'load_labeled': 'Loads a CSV catalog and uses one field as stream case labels.',
            'nrows': 'Returns the aligned row count when all streams share the same length.',
            'minrows': 'Returns the smallest stream length in the catalog.',
            'maxrows': 'Returns the largest stream length in the catalog.',
        },
        'Registry': {
            'apply': 'Applies a function to one or more named slots and returns the updated registry.',
            'apply_': 'Applies a function to one or more named slots in place.',
            'clear': 'Removes all slots and returns an empty registry.',
            'clear_': 'Removes all slots in place.',
            'concat': 'Appends extra values onto one named slot and returns the updated registry.',
            'concat_': 'Appends extra values onto one named slot in place.',
            'has': 'Checks whether a named slot exists.',
            'has_all': 'Checks whether every requested slot name exists.',
            'has_any': 'Checks whether at least one requested slot name exists.',
            'head': 'Returns the first `n` values from every slot in the registry.',
            'keep': 'Keeps only the values in one named slot that satisfy the supplied predicate.',
            'keep_': 'Keeps only the values in one named slot in place.',
            'map': 'Transforms every slot with the same function and returns the updated registry.',
            'map_': 'Transforms every slot in place with the same function.',
            'morph': 'Rewrites the values inside one named slot with a user function and returns the updated registry.',
            'morph_': 'Rewrites the values inside one named slot in place.',
            'profiles': 'Summarizes the value types stored inside each slot.',
            'prune': 'Removes slots whose payload is empty and returns the reduced registry.',
            'prune_': 'Removes empty slots in place.',
            'push': 'Appends one new value to a named slot and returns the updated registry.',
            'push_': 'Appends one new value to a named slot in place.',
            'rename_': 'Renames one or more slots in place without changing their stored values.',
            'sample': 'Returns a random sample of positions across every slot.',
            'slice': 'Returns a contiguous positional slice `[start, stop)` across every slot.',
            'take': 'Returns the values at the requested integer positions across every slot.',
            'take_': 'Selects values from one named slot by integer position in place.',
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
            'bounded': 'Computes variable-width window statistics where each entry chooses its own bounds through a callback.',
            'bucketize': 'Places numeric values into interval bins and returns the assigned bin labels as a new thread.',
            'btw': 'Builds a boolean mask telling you whether each row falls between lower and upper bounds.',
            'casefold': 'Applies Unicode-aware case folding to note-backed text values.',
            'clip': 'Clamps numeric values to the supplied lower and upper bounds.',
            'corr': 'Computes the correlation between two numeric threads.',
            'corr_with': 'Computes one correlation value between this numeric thread and another numeric thread.',
            'covar': 'Computes the covariance between two numeric threads.',
            'distinct': 'Returns the distinct observed values from the current thread in stable first-seen order.',
            'dtype': 'Returns the scalar dtype name used by the current numeric payload.',
            'dropnull': 'Removes entries marked missing from the thread.',
            'duplicated': 'Builds a boolean mask marking duplicated entries in the thread.',
            'encode': 'Encodes note-backed text values into numeric codes or related compact representations.',
            'ends': 'Checks whether each note-backed text value ends with the requested suffix.',
            'exp': 'Applies the elementwise exponential to a numeric thread.',
            'extract': 'Extracts text submatches or text-derived pieces from note-backed values.',
            'filter': 'Keeps only the entries where the supplied boolean mask is true.',
            'flow': 'Computes a cumulative directional summary across the thread values.',
            'fold': 'Reduces the thread through a folding-style aggregation route.',
            'freq': 'Counts the most frequent observed values in the thread.',
            'fullmatch': 'Checks whether each note-backed text value fully matches the requested regular expression.',
            'has_display_labels': 'Reports whether the thread currently carries explicit display labels.',
            'has_display_name': 'Reports whether the thread currently carries an explicit display name.',
            'head': 'Returns the first `n` entries of the thread.',
            'indicate': 'Returns a boolean indicator thread showing which entries satisfy the requested predicate.',
            'interp': 'Interpolates missing numeric values inside the thread.',
            'kind': 'Returns which payload kind this thread currently stores, such as Series, Note, Timeline, or TimeIndex.',
            'kurtosis': 'Computes sample kurtosis of a numeric thread.',
            'length': 'Returns the element count of the current thread.',
            'log': 'Applies the elementwise natural logarithm to a numeric thread.',
            'max': 'Returns the maximum value of a numeric thread.',
            'matches': 'Checks whether each note-backed text value matches the requested regular expression.',
            'matmul': 'Computes the dot-product-style matrix multiplication between two numeric threads.',
            'mean': 'Returns the mean value of a numeric thread.',
            'median': 'Returns the median of a numeric thread.',
            'min': 'Returns the minimum value of a numeric thread.',
            'mode': 'Returns the most frequently observed value in the thread.',
            'ndistinct': 'Counts distinct observed values in the thread.',
            'note': 'Returns the current payload as a note/text object.',
            'normalize': 'Normalizes note-backed text values into the requested Unicode normalization form.',
            'pad': 'Pads note-backed text values to a target width.',
            'partition': 'Splits note-backed text values around a separator or boundary rule.',
            'percentiles': 'Computes requested percentile points for a numeric thread.',
            'put': 'Writes replacement values into positions selected by a boolean mask.',
            'rank': 'Assigns numeric ranks to thread values while controlling ties and null handling.',
            'strip': 'Trims note-backed text values. Use `strip()` for whitespace or `strip(chars)` for an explicit trim set.',
            'lstrip': 'Trims only the left edge of note-backed text values. This is useful for prefixes such as leading `#` in IDs.',
            'rstrip': 'Trims only the right edge of note-backed text values. Use it when trailing markers should be removed without touching the left side.',
            'series': 'Returns the current payload as a numeric series object.',
            'scan': 'Computes a cumulative statistic such as running sum, product, min, or max down the thread.',
            'se': 'Returns the standard error summary for a numeric thread.',
            'slice': 'Returns a contiguous slice `[start, stop)` of the thread.',
            'skew': 'Returns sample skewness of a numeric thread.',
            'arrange': 'Reorders thread entries by their values while keeping the payload type unchanged.',
            'split': 'Splits note-backed text values into pieces using the requested delimiter or rule.',
            'std': 'Returns the standard deviation of a numeric thread.',
            'starts': 'Checks whether each note-backed text value starts with the requested prefix.',
            'sum': 'Returns the sum of a numeric thread.',
            'tail': 'Returns the last `n` entries of the thread.',
            'take': 'Returns the entries at the requested integer positions.',
            'timeindex': 'Returns the current payload as a time-index object.',
            'timeline': 'Returns the current payload as a timeline object.',
            'titlecase': 'Converts note-backed text values into title case.',
            'upper': 'Converts note-backed text values to uppercase.',
            'var': 'Returns the variance of a numeric thread.',
            'weigh': 'Computes a weighted trailing window statistic over the thread using an explicit weight vector.',
            'window': 'Computes a window-based transformation such as a rolling mean over the thread values.',
            'with_display_labels': 'Attaches display labels to the thread without changing the underlying values.',
            'with_display_name': 'Attaches a display name to the thread without changing the underlying values.',
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
            'clip': 'Clamps numeric stream values to the supplied lower and upper bounds.',
            'drop_meta': 'Returns a stream with one metadata key removed.',
            'drop_meta_': 'Removes one metadata key in place.',
            'dtype': 'Returns the scalar dtype name used by the current numeric payload.',
            'exp': 'Applies the elementwise exponential to a numeric stream.',
            'filter': 'Keeps only the entries where the supplied boolean mask is true.',
            'from_thread': 'Builds a Kindle stream from a table thread.',
            'has_cases': 'Reports whether the stream currently carries explicit case labels.',
            'has_meta': 'Reports whether a metadata key exists on the stream.',
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
            'var': 'Returns the variance of a numeric stream.',
            'with_cases': 'Returns a new stream with the supplied case labels attached.',
            'with_cases_': 'Attaches the supplied case labels in place.',
            'with_meta': 'Returns a new stream with one metadata key/value attached.',
            'with_meta_': 'Adds or updates one metadata key/value in place.',
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
        'Axel': {
            'apply_slot': 'Applies a slot transform to one named key inside every registry and returns the updated axel.',
            'concat_depth': 'Appends the registries from another axel onto the current depth axis.',
            'head': 'Returns the first `n` registries from the axel depth axis.',
            'map': 'Applies the same function to each registry and returns the transformed axel.',
            'select': 'Keeps only the named keys inside every registry in the axel.',
            'slice': 'Returns a contiguous depth slice `[start, stop)` from the axel.',
            'tail': 'Returns the last `n` registries from the axel depth axis.',
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
            'value': 'Returns one scalar-like wrapper for the first referenced value, which is useful when you need typed single-cell access.',
            'values': 'Materializes the full referenced column as a standalone `Thread` so you can reuse it outside the parent frame.',
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
            'act': 'Builds an autocorrelation-style lag plot for one numeric feature.',
            'andrews_curves': 'Builds Andrews curves from several numeric features, optionally colored by a class feature.',
            'area': 'Builds a filled area plot from one or more numeric features.',
            'area_stacked': 'Builds a stacked area plot where several numeric features accumulate at shared x positions.',
            'auto_plot': 'Chooses a plot family automatically from the selected feature set and frame shape.',
            'bar': 'Builds a bar chart from one feature, optionally using another feature as the x-axis.',
            'bar_stacked': 'Builds stacked bars so multiple y-features accumulate on the same axis positions.',
            'bootstrap': 'Builds a bootstrap summary plot for one numeric feature using the supplied resampling options.',
            'box': 'Builds a box plot for one or more numeric features.',
            'brief': 'Builds a compact summary figure for one or more numeric features.',
            'corr_heatmap': 'Computes a correlation matrix across numeric features and renders it as a heatmap.',
            'density': 'Builds one or more kernel-density curves from numeric features.',
            'heatmap': 'Builds a heatmap either directly from numeric feature values or from grouped feature/value aggregations.',
            'hist': 'Builds a histogram for one feature using the requested number of bins.',
            'lag': 'Builds a lag plot that compares one numeric feature against a shifted copy of itself.',
            'line': 'Builds a line chart for one or more y-features, optionally against a chosen x-feature.',
            'parallel': 'Builds a parallel-coordinates plot from several numeric features, optionally grouped by a class feature.',
            'pie': 'Builds a pie chart from one value feature, optionally using another feature for labels.',
            'radviz': 'Builds a radial visualization from several numeric features, optionally grouped by a class feature.',
            'scatter': 'Builds a scatter plot using one feature for x and another for y.',
            'scatter_matrix': 'Builds a scatter-matrix figure for several numeric features.',
            'step': 'Builds a step chart for one or more y-features, optionally against a chosen x-feature.',
            'violin': 'Builds violin plots for one or more numeric features or grouped distributions.',
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
    if name in {'all', 'any'}:
        return 'Reduces boolean-like values across the current object and returns whether every value or at least one value satisfies the requested truth test.'
    if name.startswith('as_'):
        return 'Converts or reinterprets the current payload as another public Pluss type so downstream operations route through the right text, numeric, or temporal surface.'
    if name in {'abs', 'clip', 'log', 'exp', 'lower', 'strip', 'lstrip', 'rstrip'}:
        return 'Applies an elementwise cleanup or transformation while preserving the surrounding alignment, labels, and payload family whenever possible.'
    if name == 'chars':
        return 'Extracts positional character slices from each note-backed text value in the current object.'
    if name.startswith('display_') or name.startswith('clear_display_'):
        return 'Reads or clears display-only metadata without changing the underlying payload values.'
    if name in {'column_names', 'feature_names', 'field_names', 'keys', 'labels', 'axis0_labels', 'depth_labels'}:
        return 'Returns the current labels used to address fields, slots, rows, or depth items.'
    if name in {'columns', 'values', 'features', 'streams', 'slots', 'frames', 'catalogs', 'registries'}:
        return 'Materializes the current members as a collection you can inspect or reuse elsewhere.'
    if name in {'get', 'feature_ref', 'stream', 'slot', 'frame', 'catalog', 'registry', 'at', 'iat', 'iloc', 'loc', 'value', 'caselbl'}:
        return 'Fetches one value, member, or slice from the current object without changing the source object.'
    if name in {'meta'}:
        return 'Reads one metadata value from the current object without changing the payload.'
    if name in {'with_meta', 'with_meta_', 'drop_meta', 'drop_meta_', 'with_cases', 'with_cases_', 'clear_cases', 'clear_cases_', 'clear_caselbl', 'clear_caselbl_'}:
        return 'Updates object-level structure such as metadata, case labels, or validity state while preserving the underlying values and alignment model. Underscore forms mutate in place.'
    if name in {'head', 'tail', 'slice', 'slice_depth', 'slice_frames', 'take', 'sample', 'iloc', 'loc', 'view'}:
        return 'Returns a smaller preview cut from the current object so you can inspect rows or depth items without carrying the full payload forward.'
    if name in {'fillnull', 'fillnull_', 'dropnull', 'ffill', 'bfill', 'isnull', 'notnull', 'isnan', 'isfinite'}:
        return 'This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.'
    if name in {'add', 'sub', 'subtract', 'mul', 'multiply', 'div', 'divide', 'truediv', 'floordiv', 'mod', 'pow', 'radd', 'rsub', 'rmul', 'rdiv', 'rtruediv', 'rfloordiv', 'rmod', 'rpow', 'numeric_binary'}:
        return 'This applies arithmetic across the compatible numeric payloads in the current object.'
    if name in {'eq', 'ne', 'lt', 'le', 'gt', 'ge', 'compare'}:
        return 'This produces a comparison-style result you can inspect directly or reuse as a mask.'
    if name in {'sum', 'mean', 'min', 'max', 'std', 'var', 'count', 'count_by', 'mean_by', 'aggregate', 'group', 'describe', 'tally', 'aggregate_over_frames', 'aggregate_over_catalogs', 'type_counts', 'slot_sizes', 'profiles'}:
        return 'Use this when you want a summary view instead of carrying the full input forward unchanged.'
    if name in {'cast', 'astype', 'cast_series', 'cast_note', 'cast_timeline', 'cast_timeindex'}:
        return 'This changes the semantic or numeric type so later operations route through the correct implementation path.'
    if name in {'kindelize', 'tabelize', 'to_kindle', 'from_kindle', 'to_table', 'to_thread', 'as_kindle'}:
        return 'This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.'
    if name in {'query', 'where', 'filter', 'select', 'drop', 'select_features', 'grep', 'keep'}:
        return 'Use this to reduce the current object to the rows or fields that matter for the next step.'
    if name in {'rename', 'rename_', 'relabel_depth', 'set_axis', 'set_axis', 'add_prefix', 'add_suffix'}:
        return 'Changes names or labels only. The underlying payload values, row order, and aligned structure stay the same.'
    if name in {'join', 'update', 'combine', 'merge', 'unify', 'combine_values', 'concat', 'concat_depth'}:
        return 'Use this when you need to merge compatible objects or extend the current one with another input.'
    if name in {'assign', 'transform', 'apply', 'apply_', 'map', 'map_', 'map_frames', 'map_catalogs', 'apply_slot', 'set', 'set_', 'set_stream', 'set_stream_', 'set_streams', 'set_streams_', 'set_slot', 'set_slot_', 'set_slots', 'set_slots_', 'set_features', 'set_features_', 'with_cases', 'with_cases_', 'with_meta', 'with_meta_', 'push', 'push_', 'clear', 'clear_', 'clean', 'featurize', 'keep', 'keep_', 'morph', 'morph_', 'prune', 'prune_', 'take_', 'concat_'}:
        return 'Adds, replaces, filters, or rewrites part of the current object while preserving the surrounding alignment model. Methods ending with `_` perform the change in place.'
    if name in {'sql', 'prep_sql'}:
        return 'Use this when the transformation is easier to express as SQL than as a chained sequence of table verbs.'
    if name in {'plot'}:
        return 'This opens the plotting accessor for the current data object.'
    if name in {'display', 'info', 'show'}:
        return 'This renders a human-readable view for inspection instead of creating a new analytic object.'
    if name in {'to_string', 'to_json', 'info_string'}:
        return 'Serializes or renders the current object into a text form you can inspect, log, or export.'
    if name in {'begin', 'end'}:
        return 'Exposes iterator boundaries so the object can be traversed with a range-based loop or manual iteration.'
    if name in {'clean', 'keywords', 'similarity', 'grep', 'featurize', 'stats', 'tfidf', 'lower', 'replace', 'contains'}:
        return 'This is part of the text-processing surface for note-backed or text-oriented data.'
    if name == 'tl' and class_name == 'DataFrame':
        return 'Opens the timeline surface for the current selected DataFrame. Every selected feature must be Timeline/TimeIndex-backed; select timeline feature(s) first with `df.features({...}).tl()`.'
    if name in {'tl', 'ts', 'year', 'month', 'day', 'hour', 'to_note_weekday', 'shift', 'floor', 'ceil', 'round', 'resample'}:
        return 'This is part of the time-aware surface and expects timeline or time-index semantics.'
    return ''


STRUCTURED_TYPES = {
    'DataFrame',
    'Thread',
    'Atlas',
    'Stream',
    'Catalog',
    'Archive',
    'Slot',
    'Registry',
    'Axel',
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
        'Axel': 'axel',
    }[class_name]


def normalize_user_names(class_name: str, text: str) -> str:
    text = public_cpp_type(text)
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
    if t == 'Axel':
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
    'Axel',
    'ColumnRef',
    'ThreadValue',
    'pluss::Series',
    'pluss::Note',
    'pluss::Timeline',
    'pluss::TimeIndex',
)


NO_BEFORE_AFTER = {
    'defined', 'unique', 'size', 'kind', 'dtype', 'nrows', 'ncols', 'nfeatures', 'nstreams',
    'nfields', 'depth', 'empty', 'count', 'count_null_rows', 'has', 'has_any', 'has_all',
    'has_stream', 'has_slot', 'has_meta',
    'keys', 'field_names', 'column_names', 'feature_names', 'values', 'streams', 'slots',
    'frames', 'catalogs', 'registries', 'axis0_labels', 'depth_labels', 'labels',
    'to_string', 'to_json', 'info_string', 'begin', 'end',
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


def example_caption(class_name: str, method: Method) -> str:
    n = method.name
    sig = method.signature

    if class_name == 'DataFrame':
        if n == 'assign':
            if 'RangeView::Of<DeriveSpec>' in sig:
                return 'Derive one or more columns inside the call from the current frame state.'
            return 'Attach one or more already-computed threads by name.'
        if n == 'astype':
            if 'feature_dtypes' in sig:
                return 'Convert several selected features at once by supplying a feature-to-dtype map.'
            return 'Convert one or more selected features to a target numeric Series dtype.'
        if n == 'cast':
            if 'CastSpec' in sig:
                return 'Cast selected features with explicit target-family and dtype control.'
            return 'Cast selected features into another Pluss payload family.'
        if n == 'caselbl':
            if 'std::pair<std::int64_t, std::int64_t> row_span' in sig:
                return 'Extract a contiguous row span by case position.'
            if 'std::size_t feature_idx' in sig:
                return 'Read one cell by row position and feature index.'
            if 'std::string_view feature_name' in sig:
                return 'Read one cell by row position and feature name.'
            if 'std::int64_t row' in sig:
                return 'Extract one row by case position.'
            return 'Read the current case axis thread.'
        if n == 'combine':
            if 'const CombineSpec& spec' in sig:
                return 'Combine two aligned frames with an explicit combine-policy object.'
            return 'Combine two aligned frames with the default feature-wise policy.'
        if n == 'load':
            if 'ReadCsvOptions' in sig:
                return 'Load a delimited file through the format-switched entry point while controlling parsing with a typed CSV options object.'
            if 'initializer_list<pluss::loader::ConfigEntry>' in sig:
                return 'Load a runtime-selected source type while passing inline loader configuration entries.'
            if 'const pluss::loader::Config&' in sig:
                return 'Load a runtime-selected source type with a reusable loader configuration object.'
            if 'RangeView::Of<std::string_view> usefeats' in sig:
                return 'Load a runtime-selected delimited source while projecting only a selected feature subset from a lightweight string view.'
            if 'std::vector<std::string> usefeats' in sig:
                return 'Load a runtime-selected delimited source while overriding the separator, row limit, and projected feature list.'
            return 'Load a runtime-selected source type with the default reader for that format.'
        if n == 'load_csv':
            if 'ReadCsvOptions' in sig:
                return 'Load a CSV file with a typed options object so parsing rules live in one reusable config.'
            if 'initializer_list<pluss::loader::ConfigEntry>' in sig:
                return 'Load a CSV file while supplying inline configuration entries such as row limits or projected features.'
            if 'const pluss::loader::Config&' in sig:
                return 'Load a CSV file with a reusable loader configuration object.'
            if 'std::string_view header, std::string_view case_feature' in sig:
                return 'Load a CSV file while explicitly controlling header inference, case-axis promotion, engine selection, and row limit.'
            if 'std::string_view header, std::size_t ncases' in sig:
                return 'Load a headerless CSV file and auto-generate feature names while limiting the number of rows read.'
            if 'RangeView::Of<std::string_view> usefeats' in sig:
                return 'Load a CSV file with an explicit separator, row limit, and projected feature subset from a lightweight string view.'
            if 'std::vector<std::string> usefeats' in sig:
                return 'Load a CSV file with an explicit separator, row limit, and projected feature subset from a standard string container.'
            return 'Load a CSV file with the default parser settings.'
        if n == 'load_excel':
            if 'ReadExcelOptions' in sig:
                return 'Load one Excel sheet with a typed Excel options object.'
            if 'initializer_list<pluss::loader::ConfigEntry>' in sig:
                return 'Load an Excel workbook while supplying inline configuration entries instead of a named config object.'
            if 'const pluss::loader::Config&' in sig:
                return 'Load one Excel sheet with a reusable loader configuration object.'
            if 'std::nullptr_t' in sig:
                return 'Load every worksheet from an Excel workbook into one multi-sheet load bundle.'
            if 'std::initializer_list<std::string_view> sheet_names' in sig:
                return 'Load several worksheets by name using an inline list of sheet labels.'
            if 'const std::vector<std::string>& sheet_names' in sig:
                return 'Load several worksheets by name from a standard string container.'
            if 'RangeView::Of<std::string_view> sheet_names' in sig:
                return 'Load several worksheets by name from a lightweight string view.'
            if 'std::initializer_list<std::int64_t> sheet_indices' in sig:
                return 'Load several worksheets by zero-based sheet index using an inline integer list.'
            if 'const std::vector<std::int64_t>& sheet_indices' in sig:
                return 'Load several worksheets by zero-based sheet index from a standard integer container.'
            if 'RangeView::Int sheet_indices' in sig:
                return 'Load several worksheets by zero-based sheet index from a lightweight integer view.'
            if 'std::string_view sheet_name' in sig:
                return 'Load one worksheet by its sheet name.'
            if 'std::int64_t sheet_index' in sig:
                return 'Load one worksheet by its zero-based sheet index.'
            return 'Load the default worksheet from an Excel workbook.'
        if n in {'load_json', 'load_parquet', 'load_txt'}:
            format_name = n.removeprefix('load_').upper()
            if 'initializer_list<pluss::loader::ConfigEntry>' in sig:
                return f'Load a {format_name} source while supplying inline configuration entries.'
            if 'const pluss::loader::Config&' in sig:
                return f'Load a {format_name} source with a reusable loader configuration object.'
            return f'Load a {format_name} source with the default reader for that format.'
        if n == 'corr':
            if 'pluss::msharp::RangeView::Of<std::string_view> features, CorrMethod method' in sig:
                return 'Find the correlation between selected columns using a non-default correlation method.'
            if 'pluss::msharp::RangeView::Of<std::string_view> features' in sig:
                return 'Find the correlation for a selected subset of numeric columns.'
            if 'CorrMethod method' in sig:
                return 'Find the correlation across all numeric columns using a non-default correlation method.'
            return 'Find the correlation across all numeric columns using the default correlation method.'
        if n == 'corr_with':
            if 'RangeView::Of<DataFrame> others' in sig:
                return 'Compare the current frame against several other frames and return one correlation thread per comparison frame.'
            return 'Compare the current frame against one other frame and return one correlation value per shared numeric feature.'
        if n == 'case_study':
            if 'std::string_view feature_name' in sig:
                return 'Look up one cell using a row position and a column label.'
            if 'std::int64_t start, std::int64_t stop' in sig:
                return 'Study a contiguous row window by position.'
            return 'Study one row by position.'
        if n == 'drop':
            return 'Drop the named columns and keep the remaining schema unchanged.'
        if n == 'label().drop':
            if 'initializer_list<std::string_view>' in sig:
                return 'Drop one or more structured case labels by naming them inline.'
            return 'Drop one or more structured case labels from a reusable name range.'
        if n == 'feature_ref':
            return 'Open a reusable column reference without copying the feature into a new frame.'
        if n == 'caselbl':
            if 'std::string_view name' in sig:
                return 'Extract one structured case-label component by name.'
            if 'std::int64_t position' in sig:
                return 'Extract one structured case-label component by zero-based position.'
        if n == 'caselbls':
            return 'List the current structured case-label names in order.'
        if n == 'matmul':
            return 'Multiply the numeric matrix represented by this frame with another numeric frame.'
        if n == 'mode':
            return 'Find the most frequent observed value in each compatible feature.'
        if n == 'percentiles':
            return 'Compute several requested percentile points for each numeric feature.'
        if n in {'sum', 'mean', 'median', 'min', 'max', 'std', 'var', 'count'}:
            if 'std::int64_t axis' in sig or 'int axis' in sig:
                return f'Compute `{n}` across rows instead of returning one summary value per numeric column.'
            return f'Compute `{n}` for each numeric column in the frame.'
        if n in {'all', 'any'}:
            if 'std::int64_t axis' in sig:
                return f'Evaluate `{n}` across rows using a selected boolean feature set.'
            return f'Evaluate `{n}` down the selected boolean columns.'
        if n in {'filter', 'where'}:
            if 'const Thread& bool_mask' in sig:
                return 'Filter rows using a boolean mask built from an existing frame expression.'
            return 'Filter rows using an explicit boolean Series mask.'
        if n in {'set_features', 'set_features_'}:
            if 'RangeView::Of<std::string_view> names' in sig:
                return 'Add or replace several columns in one call.'
            return 'Add or replace one column by name.'
        if n == 'sql':
            if 'SqlOptions options' in sig:
                return 'Create the SQL accessor with an explicit SQL options object before running a query.'
            return 'Create the SQL accessor with default options and run a query immediately.'
        if n == 'prep_sql':
            if 'SqlOptions options' in sig:
                return 'Prepare a parameterized SQL statement with an explicit SQL options object.'
            return 'Prepare a parameterized SQL statement with default SQL options.'
        if n == 'ts':
            if 'std::string_view feature' in sig:
                return 'Bind the time-series accessor to one feature.'
            if 'std::initializer_list<std::string_view>' in sig:
                return 'Bind the time-series accessor by passing several feature names inline.'
            return 'Bind the time-series accessor to a selected feature subset.'
        if n == 'bucketize':
            if 'std::string_view feature_name' in sig:
                return 'Bucketize one numeric column into interval labels.'
            if 'RangeView::Of<std::string_view> feature_names' in sig:
                return 'Bucketize several selected numeric columns at once.'
            return 'Bucketize every compatible numeric column in the frame.'
        if n == 'features':
            if 'const NameRange& names' in sig:
                return 'Project a named feature subset from a reusable name-range container.'
            if 'RangeView::Of<std::string_view> names' in sig:
                return 'Project a named feature subset from a lightweight string view.'
            if 'StackArray<std::string_view> names' in sig:
                return 'Project a named feature subset from a fixed stack-allocated name list.'
            if 'initializer_list<const char*>' in sig:
                return 'Project a named feature subset from inline C-string feature names.'
            if 'initializer_list<std::string_view>' in sig:
                return 'Project a named feature subset from inline string-view feature names.'
            if 'std::string_view name' in sig:
                return 'Extract one feature thread by name.'
            return 'Materialize every feature thread in left-to-right order.'
        if n == 'select':
            return 'Project only the named columns into a new frame.'
        if n in {'fillnull', 'fillnull_'}:
            mutate = 'Fill missing values in place' if n.endswith('_') else 'Fill missing values in a returned copy'
            if 'RangeView::Of<std::string_view> features' in sig:
                return f'{mutate} only within a selected feature subset.'
            if 'TimeIndex' in sig:
                return f'{mutate} with a TimeIndex scalar.'
            if 'Timeline' in sig:
                return f'{mutate} with a Timeline scalar.'
            if 'const char* value' in sig or 'std::string_view value' in sig:
                return f'{mutate} with a text scalar.'
            return f'{mutate} with a numeric scalar.'
        if n == 'observe':
            if 'std::int64_t start, std::int64_t stop' in sig:
                return 'Preview a contiguous row window without changing the underlying frame.'
            return 'Preview one row without changing the underlying frame.'
        if n == 'join':
            return 'Attach columns from another case-aligned frame, adding the collision suffix only when names overlap.'
        if n == 'reorder':
            if 'initializer_list<ReorderSpec>' in sig:
                return 'Apply an inline multi-step reorder plan that moves only the columns you care about.'
            if 'pluss::RangeView<ReorderSpec>' in sig:
                return 'Apply a reusable multi-step reorder plan stored in a standard container.'
            if 'ReorderSpec' in sig:
                return 'Move selected columns to the front, back, or around an anchor while preserving the rest.'
            return 'Move the named columns to the front while preserving the relative order of every untouched column.'
        if n in {'rolling', 'window'}:
            return 'Build one or more derived rolling/window features from structured `WindowSpec` instructions.'
        if n == 'sample':
            if ' frac' in sig and 'std::uint64_t random_state' in sig:
                return 'Sample a fraction of the selected axis with an explicit random seed.'
            if ' frac' in sig:
                return 'Sample a fraction of the selected axis without an explicit random seed.'
            if 'std::uint64_t random_state' in sig:
                return 'Sample rows or features with an explicit random seed.'
            if 'bool replace, int axis' in sig:
                return 'Sample rows or features while controlling replacement and axis directly.'
            return 'Sample rows with the default settings.'
        if n == 'scan':
            return 'Run one cumulative statistic down each numeric feature.'
        if n == 'shuffle':
            return 'Shuffle row order while keeping the feature order unchanged.'
        if n == 'reshuffle':
            if 'axis = 1' in sig:
                return 'Shuffle feature order instead of row order.'
            return 'Shuffle rows or features depending on the selected axis.'
        if n == 'shift':
            if 'RangeView::Of<std::string_view> features' in sig:
                return 'Shift only selected columns by a fixed number of rows.'
            return 'Shift every compatible column by a fixed number of rows.'
        if n == 'plot':
            if 'DataFramePlotOptions' in sig:
                return 'Create the plotting accessor with an explicit plot-options object.'
            return 'Create the plotting accessor with default plot options.'
        if n == 'plot3d':
            if 'DataFramePlotOptions' in sig:
                return 'Create the 3D plotting accessor with an explicit plot-options object.'
            return 'Create the 3D plotting accessor with default plot options.'
        if n == 'query':
            return 'Filter rows with a boolean expression written against feature names.'
        if n == 'rename':
            if 'old_name, new_name' in user_signature(method, class_name):
                return 'Rename one column by giving its old name and its new name.'
            if 'initializer_list<std::pair<std::string_view, std::string_view>>' in sig:
                return 'Rename several columns at once with an inline old-name/new-name mapping.'
            if 'RangeView::Of<std::pair<std::string_view, std::string_view>>' in sig:
                return 'Rename several columns at once with a reusable old-name/new-name mapping.'
            if 'initializer_list<const char*>' in sig:
                return 'Replace the full feature axis by listing every final column name inline as C strings.'
            return 'Replace the full feature axis by passing every final column name in order.'
        if n == 'rename_':
            return 'Rename one column in place by giving its old name and its new name.'
        if n == 'select':
            return 'Project only the named columns into a new frame.'
        if n == 'unstack':
            if 'std::string_view caselbl' in sig:
                return 'Pivot one named case label into output columns using the default unstack options.'
            return 'Pivot one named case label into output columns while controlling drop, sort, and separator behavior.'
        if n == 'update':
            return 'Overlay another aligned frame onto the current one and resolve collisions with the overwrite switch.'
        if n == 'info':
            if 'std::ostream& os' in sig:
                return 'Write the frame summary to a caller-provided output stream.'
            return 'Print the frame summary with the default output route.'
        if n == 'show':
            if 'std::ostream& os' in sig:
                return 'Render the frame to a caller-provided output stream.'
            return 'Render the frame using the default display route.'
        if n in {'put', 'put_'}:
            mask_source = 'a boolean Series mask' if 'const pluss::Series& bool_mask' in sig else 'a boolean Thread mask'
            if 'const Thread& value' in sig:
                value_kind = 'another full Thread as the replacement values'
            elif 'pluss::null_t value' in sig:
                value_kind = '`pluss::null` to mark the matched cells as missing'
            elif 'std::string_view value' in sig or 'const char* value' in sig:
                value_kind = 'a text literal as the replacement value'
            else:
                value_kind = 'a scalar numeric replacement value'
            action = 'Rewrite rows in place' if n == 'put_' else 'Rewrite selected rows'
            return f'{action} using {mask_source} and {value_kind}.'
        if n.startswith('save_') or n == 'save_as':
            if 'WriteCsvOptions' in sig or 'WriteExcelOptions' in sig:
                return 'Save the frame with a typed writer-options object.'
            if 'const pluss::loader::Config& config' in sig:
                return 'Save the frame with an explicit loader-config object.'
            if 'initializer_list<pluss::loader::ConfigEntry>' in sig:
                return 'Save the frame with inline configuration entries.'
            if 'bool add_cases' in sig or 'std::string sheet_name' in sig:
                return 'Save the frame while controlling case-axis export and file-format details explicitly.'
            return 'Save the frame using the simplest filepath-only overload.'
        if n == 'kindelize':
            if 'KindelizePolicy' in sig:
                return 'Convert the frame into a Kindle catalog using an explicit kindelization policy.'
            return 'Convert the frame into a Kindle catalog using the default policy.'
        if n == 'tally':
            if 'std::string_view feature' in sig:
                return 'Count the distinct values in one named feature.'
            if 'RangeView::Of<std::string_view> features' in sig:
                return 'Count distinct combinations across several features.'
            return 'Count distinct row groups using the default tally target.'
        if n == 'sort_values':
            if 'ArrangeSpec' in sig:
                return 'Sort rows using explicit per-column sort specifications.'
            return 'Sort rows using one shared ascending and null-order policy for every selected sort key.'
        if n == 'place':
            if 'const Thread& feature' in sig:
                return 'Insert a new feature relative to an anchor feature without rebuilding the whole schema.'
            return 'Move one existing feature relative to an anchor feature.'
        if n == 'prod':
            if 'axis' in sig:
                return 'Compute products across rows instead of returning one product per numeric feature.'
            return 'Compute one product per numeric feature.'
        if n == 'reorder':
            if 'const ReorderSpec& spec' in sig:
                return 'Move selected features with an explicit partial-order spec.'
            return 'Move selected features together while preserving the relative order of everything else.'
        if n == 'set':
            if 'std::string_view name' in sig:
                return 'Open the assignment helper for one feature name.'
            if 'initializer_list<const char*>' in sig:
                return 'Open the multi-feature assignment helper from inline C-string names.'
            if 'initializer_list<std::string_view>' in sig:
                return 'Open the multi-feature assignment helper from inline string-view names.'
            return 'Open the multi-feature assignment helper from a reusable feature-name range.'
        if n == 'tabelize':
            if 'TabelizePolicy' in sig:
                return 'Convert a Kindle catalog into a frame with an explicit tabelization policy.'
            return 'Convert a Kindle catalog into a frame with the default tabelization policy.'
        if n == 'title':
            if 'std::string title' in sig:
                return 'Return a titled copy of the current frame.'
            return 'Read the current frame title string.'
        if n == 'transform':
            if 'const std::function<Thread(const Thread&)>& fn' in sig and 'std::string_view feature' not in sig:
                return 'Rewrite every feature with the same thread-level transform.'
            if 'RangeView::Of<TransformSpec>' in sig:
                return 'Rewrite one or more selected features with structured transform specs.'
            if 'std::string_view feature, const std::function<Thread(const DataFrame&)>& fn' in sig:
                return 'Derive or replace one feature by computing it from the full frame.'
            return 'Rewrite one named feature with a thread-level transform.'
        if n == 'unify':
            if 'std::string_view left_on' in sig:
                return 'Join two frames on one key feature name.'
            if 'initializer_list<std::string_view> left_on' in sig:
                return 'Join two frames on several key features listed inline.'
            return 'Join two frames on several key features supplied through a reusable string view.'
        if n == 'stitch_on':
            if 'initializer_list<std::string_view> left_features' in sig:
                return 'Attach right-hand features by matching on several key features listed inline.'
            return 'Attach right-hand features by matching on one key feature.'
        if n == 'unsqueeze':
            return 'Lift one frame into a one-layer atlas.'
        if n in {'eq', 'ne', 'lt', 'le', 'gt', 'ge', 'compare'}:
            op_caption = {
                'eq': 'Compare each compatible cell for equality',
                'ne': 'Compare each compatible cell for inequality',
                'lt': 'Compare each compatible cell using the `<` relation',
                'le': 'Compare each compatible cell using the `<=` relation',
                'gt': 'Compare each compatible cell using the `>` relation',
                'ge': 'Compare each compatible cell using the `>=` relation',
                'compare': 'Run the not-equal comparison alias',
            }[n]
            if 'const DataFrame& other' in sig:
                return f'{op_caption} against another aligned frame.'
            return f'{op_caption} against one broadcast scalar value.'
        if n in {'add', 'sub', 'subtract', 'mul', 'multiply', 'div', 'divide', 'truediv', 'floordiv', 'mod', 'pow'}:
            verbs = {
                'add': 'Add',
                'sub': 'Subtract',
                'subtract': 'Subtract',
                'mul': 'Multiply',
                'multiply': 'Multiply',
                'div': 'Divide',
                'divide': 'Divide',
                'truediv': 'Divide',
                'floordiv': 'Divide',
                'mod': 'Compute the modulus of',
                'pow': 'Raise',
            }
            if 'const DataFrame& other' in sig:
                return f'{verbs[n]} each compatible numeric cell against another aligned frame.'
            return f'{verbs[n]} each compatible numeric cell against one broadcast scalar value.'

    if class_name == 'Thread':
        if n in {'all', 'any'}:
            return f'Reduce the current thread with `{n}()` and inspect the scalar-like result.'
        if n == 'chars':
            return 'Extract a character slice from each text entry using explicit start, stop, and step controls.'
        if n.startswith('clear_display_'):
            return 'Remove display-only metadata from the thread while leaving the underlying values unchanged.'
        if n.startswith('display_'):
            return 'Inspect display-only metadata attached to the current thread.'
        if n in {'year', 'month', 'day', 'hour', 'to_note_weekday'}:
            return f'Extract the `{n}` component from each temporal value in the current thread.'

    if class_name == 'DataFramePlotAccessor':
        if n == 'area':
            if 'AreaPlotOptions' in sig:
                return 'Plot one or more y-features as an area chart with an explicit area-options object.'
            return 'Plot one or more y-features as an area chart.'
        if n == 'area_stacked':
            return 'Plot several y-features as a stacked area chart.'
        if n in {'bar', 'line'}:
            plot_name = 'bar' if n == 'bar' else 'line'
            if 'std::string_view y_feature' in sig and 'options' in sig:
                return f'Plot one y-feature as a {plot_name} chart with an explicit plot-options object.'
            if 'std::string_view y_feature' in sig:
                return f'Plot one y-feature as a {plot_name} chart.'
            if 'initializer_list<std::string_view> y_features' in sig and 'options' in sig:
                return f'Plot several y-features listed inline as a shared {plot_name} chart with explicit plot options.'
            if 'initializer_list<std::string_view> y_features' in sig:
                return f'Plot several y-features listed inline as a shared {plot_name} chart.'
            if 'RangeView::Of<std::string_view> y_features' in sig and 'options' in sig:
                return f'Plot several y-features from a reusable string view as a shared {plot_name} chart with explicit plot options.'
            return f'Plot several y-features from a reusable string view as a shared {plot_name} chart.'
        if n == 'hist':
            if 'std::string_view feature, const HistPlotOptions& options' in sig:
                return 'Plot one numeric feature as a histogram with an explicit histogram-options object.'
            if 'std::string_view feature, std::int64_t bins' in sig:
                return 'Plot one numeric feature as a histogram with an explicit bin count.'
            if 'initializer_list<std::string_view> features, const HistPlotOptions& options' in sig:
                return 'Plot several numeric features listed inline as histograms with explicit histogram options.'
            if 'initializer_list<std::string_view> features, std::int64_t bins' in sig:
                return 'Plot several numeric features listed inline as histograms with an explicit shared bin count.'
            if 'RangeView::Of<std::string_view> features, const HistPlotOptions& options' in sig:
                return 'Plot several numeric features from a reusable string view as histograms with explicit histogram options.'
            if 'RangeView::Of<std::string_view> features, std::int64_t bins' in sig:
                return 'Plot several numeric features from a reusable string view as histograms with an explicit shared bin count.'
            if 'const HistPlotOptions& options' in sig:
                return 'Plot every compatible numeric feature as a histogram with an explicit histogram-options object.'
            return 'Plot every compatible numeric feature as a histogram with a shared bin count.'
        if n == 'scatter':
            if 'ScatterPlotOptions' in sig:
                return 'Plot one x/y feature pair as a scatter chart with an explicit scatter-options object.'
            return 'Plot one x/y feature pair as a scatter chart.'
        if n == 'pie':
            if 'PiePlotOptions' in sig:
                return 'Plot one value feature as a pie chart with an explicit pie-options object.'
            return 'Plot one value feature as a pie chart.'
        if n == 'bar_stacked':
            return 'Plot several y-features as a stacked bar chart.'
        if n == 'step':
            if 'StepPlotOptions' in sig:
                return 'Plot one or more y-features as a step chart with an explicit step-options object.'
            return 'Plot one or more y-features as a step chart.'
        if n == 'violin':
            if 'ViolinPlotOptions' in sig:
                return 'Plot one or more numeric features as violin distributions with an explicit violin-options object.'
            return 'Plot one or more numeric features as violin distributions.'

    if class_name == 'DataFrameSqlPrepared':
        if n == 'execute':
            if 'identifier_params' in sig:
                return 'Run the prepared SQL statement while binding both scalar placeholders and SQL identifier placeholders.'
            if 'params' in sig:
                return 'Run the prepared SQL statement while binding named scalar placeholders.'
            return 'Run the prepared SQL statement with no external parameter bindings.'

    if 'std::ostream& os' in sig:
        return 'Write the output to a caller-provided stream.'
    if 'RangeView::Of<Thread>' in sig or 'initializer_list<Thread>' in sig:
        return 'Pass several thread values together in one call.'
    if 'RangeView::Of<std::string_view>' in sig and 'RangeView::Of<Thread>' in sig:
        return 'Provide several names together with their matching values.'
    if 'std::nullptr_t' in sig:
        return 'Request the overload that targets every available item rather than naming them individually.'
    if 'std::string_view filepath' in sig and 'ConfigEntry' not in sig and 'Config&' not in sig and 'Options&' not in sig:
        return 'Use the simplest filepath-based overload.'
    if 'std::initializer_list<pluss::loader::ConfigEntry>' in sig:
        return 'Pass inline configuration entries instead of constructing a separate config object.'
    if 'const pluss::loader::Config&' in sig:
        return 'Pass an explicit loader or saver config object.'
    if 'Write' in sig and 'Options& options' in sig:
        return 'Pass a typed writer-options object to control file export details.'
    if 'Read' in sig and 'Options& options' in sig:
        return 'Pass a typed reader-options object to control file loading details.'
    if 'bool render' in sig:
        return 'Choose whether the returned object should auto-render in display contexts.'
    if 'bool header' in sig:
        return 'Control header handling explicitly for this overload.'
    if 'std::initializer_list<std::string_view>' in sig:
        return 'Pass several feature names inline.'
    if 'std::vector<std::string>' in sig or 'const std::vector<std::string>&' in sig:
        return 'Pass several names together from a standard string container.'
    if 'std::vector<std::int64_t>' in sig or 'RangeView::Int' in sig or 'std::span<const std::int64_t>' in sig:
        return 'Pass several integer positions together.'
    if 'initializer_list<std::string_view>' in sig:
        return 'Pass several feature names inline.'
    if 'RangeView::Of<std::string_view> features' in sig:
        return 'Work with a selected feature subset.'
    if 'std::string_view feature' in sig:
        return 'Work with one named feature.'
    if 'Options& options' in sig:
        return 'Pass an explicit options object to control the operation.'
    if n.startswith('as_'):
        target = n.removeprefix('as_').replace('_', ' ')
        return f'Convert the current object into the `{target}` representation before continuing with another type-specific workflow.'
    if n in {'head', 'tail'}:
        direction = 'first' if n == 'head' else 'last'
        return f'Inspect the {direction} part of the current object without changing the source data.'
    if n == 'slice':
        return 'Inspect one contiguous positional slice from the current object.'
    if n == 'take':
        return 'Inspect explicitly chosen positions from the current object in the order you requested.'
    if n in {'abs', 'clip', 'log', 'exp'}:
        return f'Apply `{n}` elementwise and inspect the returned result.'
    if n in {'sum', 'mean', 'min', 'max', 'std', 'var', 'count'}:
        return f'Compute `{n}` and inspect the resulting summary value or summary object.'
    if n in {'defined', 'empty', 'kind', 'dtype', 'size', 'depth'}:
        return f'Inspect the `{n}` property of the current object.'
    if n in {'with_meta', 'drop_meta', 'with_cases', 'clear_cases', 'clear_caselbl'}:
        return 'Return an updated copy after changing structural metadata, cases, or validity state.'
    if n in {'with_meta_', 'drop_meta_', 'with_cases_', 'clear_cases_', 'clear_caselbl_'}:
        return 'Mutate the current object in place, then inspect the updated state.'
    if n in {'apply', 'apply_', 'transform', 'map'}:
        return 'Apply a user function and inspect the transformed result.'
    if class_name == 'Thread':
        if method.return_type.strip() in {'Thread', 'Thread&'}:
            if method.params:
                names = ', '.join(name for name, _ in method.params)
                return f'Apply `{n}` to the current thread using explicit `{names}` arguments.'
            return f'Apply `{n}` to the current thread values and inspect the result.'
        if method.return_type.strip() == 'ThreadValue':
            return f'Compute `{n}` and inspect the scalar-like thread result.'
    if '(' in sig:
        inside = sig.split('(', 1)[1].rsplit(')', 1)[0].strip()
        if inside in {'self', ''}:
            return 'Call the simplest overload.'
    return 'Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.'


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

DF_WITH_CASES_OUT = """df.label().values():
[0, 1, 2]

out.label().values():
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

out.label().values():
["Accra", "Tema", "Kumasi"]

out.head():
id  sales
1   100.0
2   120.0
3    90.0"""

DF_RESET_CASES_OUT = """with_cases.label().values():
["Accra", "Tema", "Kumasi"]

out.label().values():
[0, 1, 2]

out.head():
id  sales  case
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi"""

DF_ALIGN_CASES_OUT = """df.label().values():
["cs0", "cs1", "cs2"]

out.label().values():
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

DF_REORDER_OUT = """city    id  sales
Accra   1   100.0
Tema    2   120.0
Kumasi  3    90.0"""

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


def case_study_doc_base() -> str:
    return """using namespace pluss::table;

auto sales = DataFrame({
    {"city", pluss::make_note({"Accra", "Tema", "Kumasi", "Tamale"})},
    {"region", pluss::make_note({"south", "south", "middle", "north"})},
    {"units", pluss::make_series({10.0, 7.0, 12.0, 5.0})},
    {"price", pluss::make_series({3.0, 4.0, 2.5, 5.0})},
}).title("sales");

auto labeled = sales.with_cases(
    pluss::make_note({"A100", "B200", "B200", "D400"}));"""


def case_study_examples() -> list[tuple[str, str, str]]:
    base = case_study_doc_base()
    return [
        (
            'Study one row by zero-based position.',
            base + '\n\nauto out = sales.case_study(pos(1));',
            """city  region  units  price
Tema  south   7.0    4.0""",
        ),
        (
            'Study a half-open positional row window.',
            base + '\n\nauto out = sales.case_study(pos(1, 3));',
            """city    region  units  price
Tema    south   7.0    4.0
Kumasi  middle  12.0   2.5""",
        ),
        (
            'Reorder rows by explicit positions and keep only named features.',
            base + '\n\nauto out = sales.case_study(pos({3, 0}), features({"city", "price"}));',
            """city    price
Tamale  5.0
Accra   3.0""",
        ),
        (
            'Match every row whose case label equals one label.',
            base + '\n\nauto out = labeled.case_study(label("B200"));',
            """city    region  units  price
Tema    south   7.0    4.0
Kumasi  middle  12.0   2.5""",
        ),
        (
            'Request several labels in your own order and project one feature subset.',
            base + '\n\nauto out = labeled.case_study(labels({"D400", "A100"}), features({"city", "units"}));',
            """city    units
Tamale  5.0
Accra   10.0""",
        ),
        (
            'Slice by case labels inclusively and slice features by name inclusively.',
            base + '\n\nauto out = labeled.case_study(label_range("B200", "D400"), feature_range("region", "price"));',
            """region  units  price
south   7.0    4.0
middle  12.0   2.5
north   5.0    5.0""",
        ),
        (
            'Use a boolean mask just like a filter, then keep only the columns you care about.',
            base + '\n\nauto out = sales.case_study(mask(sales.features("units") >= 10.0), features({"city", "units"}));',
            """city    units
Accra   10.0
Kumasi  12.0""",
        ),
        (
            'Get a one-cell result without a separate scalar-only indexing method.',
            base + '\n\nauto out = labeled.case_study(label("A100"), feature("price"));',
            """price
3.0""",
        ),
        (
            'Count backward from the end with a negative row position.',
            base + '\n\nauto out = sales.case_study(pos(-1), features({"city", "price"}));',
            """city    price
Tamale  5.0""",
        ),
        (
            'Match one structured case label with `CasePick` after `label().set(...)`.',
            """using namespace pluss::table;

auto indexed = DataFrame({
    {"region", pluss::make_note({"West", "West", "East", "East"})},
    {"account", pluss::make_series({101.0, 101.0, 201.0, 201.0})},
    {"quarter", pluss::make_note({"Q1", "Q2", "Q1", "Q2"})},
    {"sales", pluss::make_series({10.0, 20.0, 30.0, 40.0})},
    {"cost", pluss::make_series({4.0, 8.0, 12.0, 16.0})},
}).label().set({"region", "account", "quarter"}, false);

auto out = indexed.case_study(DataFrame::CasePick{
    .label = "region",
    .value = "West",
}).features({"sales", "cost"});""",
            """sales  cost
10.0   4.0
20.0   8.0""",
        ),
        (
            'Look up one full structured case key with `CasePick`.',
            """using namespace pluss::table;

auto indexed = DataFrame({
    {"region", pluss::make_note({"West", "West", "East", "East"})},
    {"account", pluss::make_series({101.0, 101.0, 201.0, 201.0})},
    {"quarter", pluss::make_note({"Q1", "Q2", "Q1", "Q2"})},
    {"sales", pluss::make_series({10.0, 20.0, 30.0, 40.0})},
    {"cost", pluss::make_series({4.0, 8.0, 12.0, 16.0})},
}).label().set({"region", "account", "quarter"}, false);

auto out = indexed.case_study(DataFrame::CasePick{
    .key = {"East", "201", "Q2"},
}).features({"sales", "cost"});""",
            """sales  cost
40.0   16.0""",
        ),
    ]


def render_case_study_page(overloads: list[Method], class_name: str) -> list[str]:
    lines: list[str] = []
    method = overloads[0]
    seen: set[str] = set()
    merged_params_list: list[tuple[str, str]] = []
    for overload in overloads:
        for name, typ in overload.params:
            if name not in seen:
                seen.add(name)
                merged_params_list.append((name, typ))
    lines.append('### `case_study`')
    lines.append('')
    lines.append('#### Signatures')
    lines.append('')
    lines.append('```cpp')
    for overload in overloads:
        lines.append(user_signature(overload, class_name))
    lines.append('```')
    lines.append('')
    lines.append('#### Parameters')
    lines.append('')
    lines.append('**pick**')
    lines.append('    Structured-case selector used when the DataFrame has named case labels created by `label().set(...)`.')
    lines.append('')
    lines.append('    Use this overload for hierarchical case lookup rather than ordinary row slicing.')
    lines.append('')
    lines.append('    Supported forms:')
    lines.append('    - `.label = "region", .value = "West"` selects every row whose named case label equals the supplied value.')
    lines.append('    - `.caselbls = {"region", "quarter"}, .values = {"West", "Q2"}` matches several named case labels at once.')
    lines.append('    - `.key = {"East", "201", "Q2"}` performs a full structured-key lookup using the current case-label order.')
    lines.append('    - `.drop = true` removes the matched case caselbl(s) from the returned structured case metadata after selection.')
    lines.append('')
    lines.append('**rows**')
    lines.append('    Row selector used for ordinary case-axis lookup and positional indexing.')
    lines.append('')
    lines.append('    Supported forms:')
    lines.append('    - `pluss::table::pos(row)` selects one row by zero-based position. Negative positions count from the end.')
    lines.append('    - `pluss::table::pos(start, stop)` selects a half-open positional slice `[start, stop)`.')
    lines.append('    - `pluss::table::pos({r1, r2, ...})` selects explicit row positions in the exact output order you request.')
    lines.append('    - `pluss::table::label(value)` selects every row whose current case label equals `value`.')
    lines.append('    - `pluss::table::labels({...})` selects several labels in the requested order. Duplicate labels expand to all matching rows.')
    lines.append('    - `pluss::table::label_range(start, stop)` selects an inclusive case-label window from the first `start` match through the last `stop` match.')
    lines.append('    - `pluss::table::mask(bool_thread)` selects rows using a boolean thread with one value per case.')
    lines.append('')
    lines.append('**features**')
    lines.append('    Optional feature selector applied after the row selection has been resolved.')
    lines.append('')
    lines.append('    Supported forms:')
    lines.append('    - `pluss::table::all_features()` keeps every feature in source order.')
    lines.append('    - `pluss::table::feature("name")` keeps one named feature.')
    lines.append('    - `pluss::table::feature(position)` keeps one feature by zero-based position.')
    lines.append('    - `pluss::table::features({"a", "b"})` keeps named features in the requested order.')
    lines.append('    - `pluss::table::features({0, 2})` keeps feature positions in the requested order.')
    lines.append('    - `pluss::table::features(start, stop)` keeps a half-open positional feature slice `[start, stop)`.')
    lines.append('    - `pluss::table::feature_range("start", "stop")` keeps an inclusive feature-name window from `start` through `stop`.')
    lines.append('')
    lines.append('#### Returns')
    lines.append('')
    lines.append('`DataFrame`')
    lines.append('    Always returns a `DataFrame`, even when the selection resolves to one row, one column, or one cell.')
    lines.append('')
    lines.append('#### What It Does')
    lines.append('')
    lines.append('`case_study(...)` is the unified DataFrame indexing surface. It handles ordinary positional row access, case-label lookup, boolean-mask selection, feature projection, and structured multi-label case lookup through `CasePick` when the frame has hierarchical cases.')
    lines.append('')
    lines.append('#### Selector Rules')
    lines.append('')
    lines.append('- Positional row selectors use zero-based indexing. Negative positions count from the end.')
    lines.append('- Positional row slices are half-open: `pos(start, stop)` means `[start, stop)`.')
    lines.append('- Label selectors match against the current case axis returned by `caselbl()`.')
    lines.append('- `label(value)` returns every matching row, so duplicate case labels are preserved instead of collapsed.')
    lines.append('- `label_range(start, stop)` is inclusive on both ends and walks the current case-axis order.')
    lines.append('- Feature-position slices are half-open, while `feature_range(start, stop)` is inclusive on both ends.')
    lines.append('- `CasePick` reads named case labels created by `label().set(...)`; it does not inspect ordinary feature columns.')
    lines.append('- Partial `CasePick` matches preserve the existing row order already stored on the frame.')
    lines.append('- Because `case_study(...)` always returns a frame, you do not need separate `at(...)` or `iat(...)` methods for single-cell access.')
    lines.append('')
    lines.append('#### Notes')
    lines.append('')
    lines.append('- Use `features({...})` when you only need feature projection and no row lookup.')
    lines.append('- Use `filter(mask)` when you already have a boolean row mask and want full-row retention with no feature projection in the same call.')
    lines.append('- Use `observe(start, stop)` for display-oriented middle slices. Use `case_study(...)` when the selected result itself is part of later computation.')
    lines.append('- If your current case axis is numeric and you want those labels as numeric data, call `caselbl().as_series()` explicitly.')
    lines.append('')
    lines.append('#### Examples')
    lines.append('')
    for idx, (caption, code, output) in enumerate(case_study_examples(), start=1):
        lines.append(f'Example {idx}. {caption}')
        lines.append('')
        lines.append('```cpp')
        lines.append(code)
        lines.append('```')
        lines.append('')
        lines.append('##### Output')
        lines.append('')
        lines.append('```text')
        lines.append(output)
        lines.append('```')
        lines.append('')
    lines.append('#### Behavior Notes')
    lines.append('')
    lines.append('- `caselbl()` remains the accessor for the case axis itself. It no longer doubles as a row-selection API.')
    lines.append('- `CasePick` is the structured-case overload. Use it only after `label().set(...)` has created named case labels.')
    lines.append('- Duplicate label matches are not deduplicated. If the case axis contains the same label twice, both rows are returned.')
    lines.append('- When you only want to preview a row window instead of materializing a selection result for later work, use `observe(...)`.')
    lines.append('- If you already know the final feature order and only need column projection, `features({...})` is still the lighter-weight feature-only path.')
    lines.append('')
    return lines


def render_custom_method_page(
    class_name: str,
    name: str,
    overloads: list[Method],
    what_it_does: str,
    parameter_lines: list[str],
    returns_lines: list[str],
    examples: list[tuple[str, str, str]],
    note_lines: list[str] | None = None,
) -> list[str]:
    lines: list[str] = []
    lines.append(f'### `{name}`')
    lines.append('')
    lines.append('#### Signatures' if len(overloads) > 1 else '#### Signature')
    lines.append('')
    lines.append('```cpp')
    for idx, overload in enumerate(overloads):
        if idx:
            lines.append('')
        lines.append(user_signature(overload, class_name))
    lines.append('```')
    lines.append('')
    lines.append('#### What It Does')
    lines.append('')
    lines.append(what_it_does)
    lines.append('')
    lines.append('#### Parameters')
    lines.append('')
    lines.extend(parameter_lines)
    lines.append('')
    lines.append('#### Returns')
    lines.append('')
    lines.extend(returns_lines)
    lines.append('')
    if note_lines:
        lines.append('#### Notes')
        lines.append('')
        lines.extend(note_lines)
        lines.append('')
    lines.append('#### Examples')
    lines.append('')
    for idx, (caption, code, output) in enumerate(examples, start=1):
        lines.append(f'Example {idx}. {caption}')
        lines.append('')
        lines.append('```cpp')
        lines.append(code)
        lines.append('```')
        lines.append('')
        lines.append('##### Output')
        lines.append('')
        lines.append('```text')
        lines.append(output)
        lines.append('```')
        lines.append('')
    return lines


def df_a_sales_base() -> str:
    return """using namespace pluss::table;

auto sales = DataFrame({
    {"city", pluss::make_note({"Accra", "Tema", "Kumasi"})},
    {"units", pluss::make_series({10.0, 7.0, 12.0})},
    {"price", pluss::make_series({3.0, 4.0, 2.5})},
}).title("sales");"""


def df_a_frame_add_base() -> str:
    return """using namespace pluss::table;

auto left = DataFrame({
    {"midterm_1", pluss::make_series({15.0, 18.0, 12.0})},
    {"midterm_2", pluss::make_series({16.0, 14.0, 13.0})},
}).title("left");

auto curve = DataFrame({
    {"midterm_1", pluss::make_series({1.0, 1.0, 1.0})},
    {"midterm_2", pluss::make_series({0.5, 0.5, 0.5})},
}).title("curve");"""


def df_a_grades_base() -> str:
    return """using namespace pluss::table;

auto grades = DataFrame({
    {"student", pluss::make_note({"Ada", "Bola", "Chen"})},
    {"assignment", pluss::make_series({9.5, 8.0, 7.5})},
    {"final", pluss::make_series({28.0, 24.0, 21.5})},
}, pluss::make_note({"s1", "s2", "s3"})).title("grades");"""


def df_a_scratch_base() -> str:
    return """using namespace pluss::table;

auto grades = DataFrame({
    {"student", pluss::make_note({"Ada", "Bola", "Chen"})},
    {"assignment", pluss::make_series({9.5, 8.0, 7.5})},
    {"final", pluss::make_series({28.0, 24.0, 21.5})},
}, pluss::make_note({"s1", "s2", "s3"})).title("grades");

auto scratch = DataFrame({
    {"score", pluss::make_series({88.0, 79.0, 91.0})},
}, pluss::make_note({"s2", "s3", "s1"})).title("scratch");"""


def df_a_flags_base() -> str:
    return """using namespace pluss::table;

auto flags = DataFrame({
    {"quiz_ok", pluss::make_series(std::vector<bool>{true, true, false})},
    {"exam_ok", pluss::make_series(std::vector<bool>{true, false, false})},
    {"bonus_ok", pluss::make_series(std::vector<bool>{true, true, true})},
}).title("flags");"""


def df_a_raw_base() -> str:
    return """using namespace pluss::table;
using pluss::ScalarType;

auto raw = DataFrame({
    {"student_id", pluss::make_series({101.0, 102.0, 103.0})},
    {"passed", pluss::make_series({1.0, 0.0, 1.0})},
}).title("raw");"""


def thread_a_scores_base() -> str:
    return """using pluss::table::Thread;

auto scores = Thread(pluss::make_series({-2.5, 0.0, 3.5}));"""


def thread_a_flags_base() -> str:
    return """using pluss::table::Thread;

auto flags = Thread(pluss::make_series(std::vector<bool>{true, false, true}));"""


def thread_a_names_base() -> str:
    return """using pluss::table::Thread;

auto names = Thread(pluss::make_note({" Ada ", "BOLA", " chen "}));"""


def thread_a_timeline_base() -> str:
    return """using pluss::table::Thread;

auto stamps = Thread(pluss::make_timeline({
    "2024-01-01T09:10:00Z",
    "2024-02-01T11:00:00Z",
    "2024-03-05T15:30:00Z",
}));"""


def thread_a_timeindex_base() -> str:
    return """using pluss::table::Thread;

auto ticks = Thread(pluss::make_timeindex({
    "2024-01-01T00:10:00Z",
    "2024-01-01T00:20:00Z",
    "2024-01-01T01:10:00Z",
    "2024-01-02T00:15:00Z",
}, true));"""


def thread_a_ints_base() -> str:
    return """using pluss::ScalarType;
using pluss::table::Thread;

auto ints = Thread(pluss::make_series({1.0, 0.0, 1.0}));"""


def df_b_missing_base() -> str:
    return """using namespace pluss::table;

auto missing = DataFrame({
    {"score", pluss::make_series({72.0, std::numeric_limits<double>::quiet_NaN(), 81.0,
                                  std::numeric_limits<double>::quiet_NaN(), 95.0})},
    {"cost", pluss::make_series({10.0, std::numeric_limits<double>::quiet_NaN(),
                                 std::numeric_limits<double>::quiet_NaN(), 40.0,
                                 std::numeric_limits<double>::quiet_NaN()})},
    {"city", pluss::make_note({"Accra", "Tema", "Kumasi", "Tamale", "Sunyani"})},
}).title("missing");"""


def df_b_grades_base() -> str:
    return """using namespace pluss::table;

auto grades = DataFrame({
    {"student", pluss::make_note({"Ada", "Bola", "Chen", "Dede"})},
    {"assignment", pluss::make_series({9.5, 8.0, 7.5, 9.0})},
    {"final", pluss::make_series({28.0, 24.0, 21.5, 26.0})},
}, pluss::make_note({"s1", "s2", "s3", "s4"})).title("grades");"""


def df_b_metrics_base() -> str:
    return """using namespace pluss::table;

auto metrics = DataFrame({
    {"sales", pluss::make_series({12.0, 27.0, 35.0, 49.0})},
    {"cost", pluss::make_series({5.0, 18.0, 33.0, 60.0})},
    {"city", pluss::make_note({"Accra", "Tema", "Kumasi", "Tamale"})},
}).title("metrics");

auto numeric = metrics.features({"sales", "cost"});"""


def thread_b_missing_base() -> str:
    return """using pluss::table::Thread;

auto missing = Thread(pluss::make_series({
    10.0,
    std::numeric_limits<double>::quiet_NaN(),
    std::numeric_limits<double>::quiet_NaN(),
    40.0,
    50.0,
}));"""


def thread_b_scores_base() -> str:
    return """using pluss::table::Thread;

auto scores = Thread(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}));"""


def thread_b_dates_base() -> str:
    return """using pluss::table::Thread;

auto dates = Thread(pluss::make_timeline({
    "2024-01-01T00:00:00Z",
    "2024-01-02T00:00:00Z",
    "2024-01-03T00:00:00Z",
}));"""


def thread_b_rowwise_base() -> str:
    return """using pluss::table::Thread;

auto scores = Thread(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}));
auto lower = Thread(pluss::make_series({10.0, 20.0, 35.0, 35.0, 45.0}));
auto upper = Thread(pluss::make_series({15.0, 25.0, 35.0, 45.0, 50.0}));"""


def render_dataframe_a_page(name: str, overloads: list[Method], class_name: str) -> list[str] | None:
    if class_name != 'DataFrame':
        return None

    if name == 'add':
        return render_custom_method_page(
            class_name,
            name,
            overloads,
            'Adds numeric data either against one scalar or against another frame. This is the method-form of DataFrame arithmetic when you want the addition intent to be explicit instead of relying on `operator+`.',
            [
                '`other` : `double`',
                '    Broadcast the same scalar across every numeric feature in the frame. Non-numeric features such as `Note`, `Timeline`, and `TimeIndex` are passed through unchanged.',
                '',
                '`other` : `DataFrame`',
                '    Add one frame to another. PlussMic aligns by the frame labels it already carries, then applies numeric addition where the participating features are compatible.',
            ],
            [
                '`DataFrame`',
                '    A new frame with the same row structure as the left-hand input and the updated numeric payloads.',
            ],
            [
                (
                'Scalar arithmetic against every numeric feature.',
                df_a_sales_base() + '\n\nauto out = sales.add(1.0);',
                """===========================
     | city    units  price
---------------------------
0    | Accra   11     4
1    | Tema    8      5
2    | Kumasi  13     3.5
---------------------------
[3 cs x 3 feat]""",
            ),
                (
                'Feature-aligned addition against another frame.',
                df_a_frame_add_base() + '\n\nauto out = left.add(curve);',
                """===========================
     | midterm_1  midterm_2
---------------------------
0    | 16         16.5
1    | 19         14.5
2    | 13         13.5
---------------------------
[3 cs x 2 feat]""",
            ),
            ],
            [
                '- `add(1.0)` and `df + 1.0` are the same operation.',
                '- Use the method form when you want chainable, self-documenting arithmetic inside a longer pipeline.',
            ],
        )

    if name == 'add_prefix':
        return render_custom_method_page(
            class_name,
            name,
            overloads,
            'Prepends the same text to every feature name in the current frame. Only the column labels change; row order, case labels, and payload values stay exactly the same.',
            [
                '`prefix` : `std::string_view`',
                '    Text inserted before every feature name from left to right.',
            ],
            [
                '`DataFrame`',
                '    A new frame with renamed features and unchanged values.',
            ],
            [
                (
                    'Prefix a projected subset before a downstream merge or export.',
                    df_a_sales_base() + '\n\nauto out = sales.features({"units", "price"}).feataxis().prefix("exam_");',
                    """=============================
     | exam_units  exam_price
-----------------------------
0    | 10          3
1    | 7           4
2    | 12          2.5
-----------------------------
[3 cs x 2 feat]""",
                ),
            ],
            [
                '- `add_prefix(...)` is useful when you want to disambiguate a block of features before `join(...)`, `unify(...)`, or export.',
                '- Because the payload is untouched, it is safe for text, numeric, and temporal features alike.',
            ],
        )

    if name == 'add_suffix':
        return render_custom_method_page(
            class_name,
            name,
            overloads,
            'Appends the same text to every feature name in the current frame. Use this when the suffix communicates provenance, season, model stage, or any other output tag.',
            [
                '`suffix` : `std::string_view`',
                '    Text appended after every feature name.',
            ],
            [
                '`DataFrame`',
                '    A new frame with renamed features and unchanged values.',
            ],
            [
                (
                    'Suffix a feature block to preserve source provenance.',
                    df_a_sales_base() + '\n\nauto out = sales.features({"units", "price"}).feataxis().suffix("_winter");',
                    """=================================
     | units_winter  price_winter
---------------------------------
0    | 10            3
1    | 7             4
2    | 12            2.5
---------------------------------
[3 cs x 2 feat]""",
                ),
            ],
            [
                '- `add_suffix(...)` is the symmetric label operation to `add_prefix(...)`.',
                '- This is often cleaner than renaming each feature one-by-one when the rename pattern is uniform.',
            ],
        )

    if name == 'aggregate':
        return render_custom_method_page(
            class_name,
            name,
            overloads,
            'Builds a compact summary view from one or more aggregation specs. Use it when you want one summary row instead of grouped rows or the full underlying frame.',
            [
                '`specs` : `pluss::RangeView<SummarySpec>`',
                '    Aggregation specifications. Each `SummarySpec` has three fields:',
                '    - `output`: output feature name. Leave it empty when you want PlussMic to auto-name the result.',
                '    - `feature`: source feature to summarize.',
                '    - `op`: summary statistic to compute.',
                '',
                '    `op` supports:',
                '    - `Count`: count observed values.',
                '    - `Ndistinct`: count distinct observed values.',
                '    - `Sum`: numeric sum.',
                '    - `Mean`: arithmetic mean.',
                '    - `Median`: median.',
                '    - `Min`: minimum.',
                '    - `Max`: maximum.',
                '    - `Std`: sample standard deviation.',
                '    - `Var`: sample variance.',
                '    - `Prod`: numeric product.',
                '',
                '`render` : `bool`, optional',
                '    When `true`, the returned `DataFrameView` can auto-render immediately in temporary display contexts. Keep it `false` when you want to call `.to_string()` or `.show()` yourself.',
            ],
            [
                '`DataFrameView`',
                '    A one-row summary view. The case label is `summary`.',
            ],
            [
                (
                    'One summary metric with an explicit output name.',
                    df_a_grades_base() + '\n\nauto out = grades.aggregate({\n    {"mean_final", "final", DataFrame::SummaryOp::Mean},\n}, false);',
                    """====================
        | mean_final
--------------------
summary | 24.5
--------------------
[1 cs x 1 feat]""",
                ),
                (
                    'Several summary metrics at once.',
                    df_a_grades_base() + '\n\nauto out = grades.aggregate({\n    {"n_students", "final", DataFrame::SummaryOp::Count},\n    {"max_final", "final", DataFrame::SummaryOp::Max},\n    {"min_final", "final", DataFrame::SummaryOp::Min},\n}, false);',
                    """==========================================
        | n_students  max_final  min_final
------------------------------------------
summary | 3           28         21.5
------------------------------------------
[1 cs x 3 feat]""",
                ),
                (
                    'Let PlussMic auto-name the outputs when the statistic names are already clear.',
                    df_a_grades_base() + '\n\nauto out = grades.aggregate({\n    {"", "assignment", DataFrame::SummaryOp::Mean},\n    {"", "assignment", DataFrame::SummaryOp::Std},\n}, false);',
                    """=========================================
        | assignment_mean  assignment_std
-----------------------------------------
summary | 8.333333333      1.040833
-----------------------------------------
[1 cs x 2 feat]""",
                ),
            ],
            [
                '- `aggregate(...)` returns a summary view, not grouped rows.',
                '- `Count` and `Ndistinct` are valid on non-numeric features; the numeric summary ops expect numeric input.',
            ],
        )

    if name == 'label().align':
        return render_custom_method_page(
            class_name,
            name,
            overloads,
            'Reorders the current frame so its case axis follows the supplied case order. This is the row-alignment helper to use when you already know the target case ordering you want to conform to.',
            [
                '`new_label` : `Thread`',
                '    Target case labels in the exact order you want the output rows to follow.',
            ],
            [
                '`DataFrame`',
                '    A new frame whose rows and case labels follow `new_label`.',
            ],
            [
                (
                    'Match one frame to the case order of another frame.',
                    df_a_scratch_base() + '\n\nauto out = scratch.label().align(grades.label().values());',
                    """============
     | score
------------
s1   | 91
s2   | 88
s3   | 79
------------
[3 cs x 1 feat]""",
                ),
            ],
            [
                '- Every requested case must already exist in the source frame.',
                '- This is row-order alignment only. It does not add or remove features.',
            ],
        )

    if name == 'all':
        return render_custom_method_page(
            class_name,
            name,
            overloads,
            'Reduces boolean features with logical AND. Use it when you want to know whether every selected value is true either feature-wise or row-wise.',
            [
                '`axis` : `Axis`, optional',
                '    - `Axis::Feature`: reduce down rows and return one result per feature.',
                '    - `Axis::Case`: reduce across features and return one result per row.',
                '',
                '`features` : `pluss::RangeView<std::string_view>`, optional',
                '    Restrict the reduction to a boolean feature subset. Leave it empty to use every compatible boolean feature in the frame.',
                '',
                '`skipnull` : `bool`, optional',
                '    When `true`, missing boolean values are skipped. When `false`, missing state participates instead of being ignored.',
            ],
            [
                '`Thread`',
                '    A boolean-like thread holding the reduction result for the chosen axis.',
            ],
            [
                (
                    'Feature-wise logical AND across the selected boolean features.',
                    df_a_flags_base() + '\n\nauto out = flags.all();',
                    """ quiz_ok  false
 exam_ok  false
bonus_ok  true
ptype: Series""",
                ),
                (
                    'Row-wise logical AND across features.',
                    df_a_flags_base() + '\n\nusing pluss::table::Axis;\nauto out = flags.all(Axis::Case);',
                    """0  true
1  false
2  false
ptype: Series""",
                ),
            ],
            [
                '- `all()` is a boolean reduction, not a numeric reducer.',
                '- If you want feature-wise truth tests on only part of the frame, pass the feature subset explicitly.',
            ],
        )

    if name == 'any':
        return render_custom_method_page(
            class_name,
            name,
            overloads,
            'Reduces boolean features with logical OR. Use it when you want to know whether at least one selected value is true either feature-wise or row-wise.',
            [
                '`axis` : `Axis`, optional',
                '    - `Axis::Feature`: reduce down rows and return one result per feature.',
                '    - `Axis::Case`: reduce across features and return one result per row.',
                '',
                '`features` : `pluss::RangeView<std::string_view>`, optional',
                '    Restrict the reduction to a boolean feature subset. Leave it empty to use every compatible boolean feature in the frame.',
                '',
                '`skipnull` : `bool`, optional',
                '    When `true`, missing boolean values are skipped. When `false`, missing state participates instead of being ignored.',
            ],
            [
                '`Thread`',
                '    A boolean-like thread holding the reduction result for the chosen axis.',
            ],
            [
                (
                    'Feature-wise logical OR across the selected boolean features.',
                    df_a_flags_base() + '\n\nauto out = flags.any();',
                    """ quiz_ok  true
 exam_ok  true
bonus_ok  true
ptype: Series""",
                ),
                (
                    'Row-wise logical OR across features.',
                    df_a_flags_base() + '\n\nusing pluss::table::Axis;\nauto out = flags.any(Axis::Case);',
                    """0  true
1  true
2  true
ptype: Series""",
                ),
            ],
            [
                '- `any()` is the usual row/feature existence test before masked writes or filtering.',
                '- Pair it with `put(...)` when the boolean result should immediately drive a replacement policy.',
            ],
        )

    if name == 'assign':
        return render_custom_method_page(
            class_name,
            name,
            overloads,
            'Adds or replaces derived features without mutating the source frame. This is the frame-level “build one or more new columns” helper when the new values depend on the current DataFrame.',
            [
                '`assignments` : `pluss::RangeView<std::pair<std::string, Thread>>`',
                '    Use this form when you already computed the replacement thread outside the call. Each pair is `{feature_name, thread_value}`.',
                '',
                '`specs` : `pluss::RangeView<DeriveSpec>`',
                '    Use this form when the frame should compute the new feature inside the call. Each `DeriveSpec` has:',
                '    - `feature`: output feature name.',
                '    - `expr`: callable with signature `Thread(const DataFrame&)`.',
            ],
            [
                '`DataFrame`',
                '    A new frame containing the original features plus the derived ones, or replacements where the names already existed.',
            ],
            [
                (
                    'Assign one already-computed thread.',
                    df_a_grades_base() + '\n\nauto total_grade = [](const DataFrame& df) {\n    return df.features("assignment") + df.features("final");\n};\n\nauto out = grades.assign({\n    std::pair<std::string, Thread>{"total", total_grade(grades)},\n});',
                    """========================================
     | student  assignment  final  total
----------------------------------------
s1   | Ada      9.5         28     37.5
s2   | Bola     8           24     32
s3   | Chen     7.5         21.5   29
----------------------------------------
[3 cs x 4 feat]""",
                ),
                (
                    'Assign several derived features computed from the frame inside the call.',
                    df_a_grades_base() + '\n\nauto total_grade = [](const DataFrame& df) {\n    return df.features("assignment") + df.features("final");\n};\n\nauto out = grades.assign({\n    DataFrame::DeriveSpec{\n        .feature = "total",\n        .expr = total_grade,\n    },\n    DataFrame::DeriveSpec{\n        .feature = "passed",\n        .expr = [](const DataFrame& df) {\n            return total_grade(df) >= 30.0;\n        },\n    },\n});',
                    """================================================
     | student  assignment  final  total  passed
------------------------------------------------
s1   | Ada      9.5         28     37.5   true
s2   | Bola     8           24     32     true
s3   | Chen     7.5         21.5   29     false
------------------------------------------------
[3 cs x 5 feat]""",
                ),
            ],
            [
                '- Every derived thread must match the frame case count.',
                '- Use the pair form when the new data already exists. Use `DeriveSpec` when you want the frame to compute it from its current state inside the call.',
            ],
        )

    if name == 'astype':
        return render_custom_method_page(
            class_name,
            name,
            overloads,
            'Casts selected Series-backed features to new scalar dtypes while leaving every other feature unchanged. This is the direct DataFrame dtype-conversion path when you already know which numeric columns should change type.',
            [
                '`features` : `pluss::RangeView<std::string_view>`',
                '    Feature subset to cast together with one shared target dtype.',
                '',
                '`dtype` : `pluss::ScalarType`',
                '    Target numeric scalar type applied to every selected feature in the two-argument overload.',
                '',
                '`feature_dtypes` : `pluss::RangeView<std::pair<std::string_view, pluss::ScalarType>>`',
                '    Per-feature dtype mapping for the overload where different columns should cast to different target dtypes in the same call.',
            ],
            [
                '`DataFrame`',
                '    A new frame with the selected Series-backed features rewritten to the requested dtypes.',
            ],
            [
                (
                    'Cast one numeric indicator column to bool.',
                    df_a_raw_base() + '\n\nauto out = raw.astype({"passed"}, ScalarType::Bool);',
                    """=========================
     | student_id  passed
-------------------------
0    | 101         true
1    | 102         false
2    | 103         true
-------------------------
[3 cs x 2 feat]""",
                ),
                (
                    'Cast several features with one mapping object.',
                    df_a_raw_base() + '\n\nauto out = raw.astype({\n    {"student_id", ScalarType::Int64},\n    {"passed", ScalarType::Bool},\n});',
                    """=========================
     | student_id  passed
-------------------------
0    | 101         true
1    | 102         false
2    | 103         true
-------------------------
[3 cs x 2 feat]""",
                ),
            ],
            [
                '- Only the selected features are cast. Every other feature is passed through unchanged.',
                '- This is the Series-dtype path. For broader target-family conversion, use `cast(...)` instead.',
            ],
        )

    return None


def render_thread_a_page(name: str, overloads: list[Method], class_name: str) -> list[str] | None:
    if class_name != 'Thread':
        return None

    if name == 'abs':
        return render_custom_method_page(
            class_name,
            name,
            overloads,
            'Applies absolute value elementwise to a numeric Series-backed thread.',
            [
                '`None`',
            ],
            [
                '`Thread`',
                '    A numeric thread with every value replaced by its absolute magnitude.',
            ],
            [
                (
                    'Take the magnitude of each numeric value.',
                    thread_a_scores_base() + '\n\nauto out = scores.abs();',
                    """0  2.5
1  0
2  3.5
ptype: Series""",
                ),
            ],
            [
                '- `abs()` is elementwise and preserves the thread length.',
                '- For frame-wide absolute values, use `DataFrame::transform(...)` or select the target feature block first.',
            ],
        )

    if name == 'all':
        return render_custom_method_page(
            class_name,
            name,
            overloads,
            'Reduces one boolean thread with logical AND.',
            [
                '`skipnull` : `bool`, optional',
                '    When `true`, missing boolean values are skipped during the reduction.',
            ],
            [
                '`ThreadValue`',
                '    One scalar-like result representing whether every participating value is true.',
            ],
            [
                (
                    'Reduce a boolean thread to one truth value.',
                    thread_a_flags_base() + '\n\nauto out = flags.all().to_str();',
                    'false',
                ),
            ],
            [
                '- `all()` returns `ThreadValue`, not a full thread.',
                '- Calling `.to_str()`, `.to_int()`, or `.host()` is often the cleanest way to surface the scalar result in user code.',
            ],
        )

    if name == 'any':
        return render_custom_method_page(
            class_name,
            name,
            overloads,
            'Reduces one boolean thread with logical OR.',
            [
                '`skipnull` : `bool`, optional',
                '    When `true`, missing boolean values are skipped during the reduction.',
            ],
            [
                '`ThreadValue`',
                '    One scalar-like result representing whether any participating value is true.',
            ],
            [
                (
                    'Reduce a boolean thread to one truth value.',
                    thread_a_flags_base() + '\n\nauto out = flags.any().to_str();',
                    'true',
                ),
            ],
            [
                '- `any()` is the scalar truth test companion to `Thread::filter(...)`, `Thread::put(...)`, and `DataFrame::where(...)`.',
            ],
        )

    if name == 'as_note':
        return render_custom_method_page(
            class_name,
            name,
            overloads,
            'Exposes the underlying `pluss::Note` payload directly so you can use the native text surface.',
            [
                '`None`',
            ],
            [
                '`const pluss::Note&`',
                '    The underlying Note payload.',
            ],
            [
                (
                    'Hop into the native Note surface, then wrap back into Thread for display.',
                    thread_a_names_base() + '\n\nauto out = Thread(names.as_note().strip().lower());',
                    """0  ada
1  bola
2  chen
ptype: Note""",
                ),
            ],
            [
                '- `as_note()` itself does not create a new `Thread`; it returns the native payload.',
                '- When the next API expects a `Thread` again, wrapping with `Thread(...)` is enough because `Thread` is constructible from Pluss types.',
            ],
        )

    if name == 'as_series':
        return render_custom_method_page(
            class_name,
            name,
            overloads,
            'Exposes the underlying `pluss::Series` payload directly so you can use native numeric operations and Series methods.',
            [
                '`None`',
            ],
            [
                '`const pluss::Series&`',
                '    The underlying Series payload.',
            ],
            [
                (
                    'Use the native Series arithmetic surface, then wrap back into Thread for display.',
                    thread_a_scores_base() + '\n\nauto out = Thread(scores.as_series() + 5.0);',
                    """0  2.5
1  5
2  8.5
ptype: Series""",
                ),
            ],
            [
                '- `as_series()` is the native escape hatch when the operation you want lives on Series rather than on Thread.',
            ],
        )

    if name == 'as_timeline':
        return render_custom_method_page(
            class_name,
            name,
            overloads,
            'Exposes the underlying `pluss::Timeline` payload directly so you can use native temporal methods.',
            [
                '`None`',
            ],
            [
                '`const pluss::Timeline&`',
                '    The underlying Timeline payload.',
            ],
            [
                (
                    'Use a native temporal extraction method, then wrap the result back into a Thread.',
                    thread_a_timeline_base() + '\n\nauto out = Thread(stamps.as_timeline().year());',
                    """0  2024
1  2024
2  2024
ptype: Series""",
                ),
            ],
            [
                '- `as_timeline()` is the native path for calendar fields, formatting, rounding, timezone work, and other temporal operations that live below Thread.',
            ],
        )

    if name == 'as_timeindex':
        return render_custom_method_page(
            class_name,
            name,
            overloads,
            'Exposes the underlying `pluss::TimeIndex` payload directly so you can use native index-aware temporal methods such as resampling.',
            [
                '`None`',
            ],
            [
                '`const pluss::TimeIndex&`',
                '    The underlying TimeIndex payload.',
            ],
            [
                (
                    'Call a native TimeIndex resampling method directly.',
                    thread_a_timeindex_base() + '\n\nauto out = Thread(ticks.as_timeindex().resample_count("D", 1));',
                    """0  3
1  1
ptype: Series""",
                ),
            ],
            [
                '- `as_timeindex()` is the native path for index-style temporal operations rather than plain timestamp-field extraction.',
            ],
        )

    if name == 'astype':
        return render_custom_method_page(
            class_name,
            name,
            overloads,
            'Casts a Series-backed thread to a new numeric scalar dtype.',
            [
                '`dtype` : `pluss::ScalarType`',
                '    Target numeric scalar dtype.',
            ],
            [
                '`Thread`',
                '    A new thread carrying the converted Series payload.',
            ],
            [
                (
                    'Turn numeric indicator values into booleans.',
                    thread_a_ints_base() + '\n\nauto out = ints.astype(ScalarType::Bool);',
                    """0  true
1  false
2  true
ptype: Series""",
                ),
            ],
            [
                '- `astype(...)` is the direct Series dtype conversion path on Thread.',
                '- For conversion between payload families, use `cast(...)` instead of `astype(...)`.',
            ],
        )

    return None


def render_dataframe_b_page(name: str, overloads: list[Method], class_name: str) -> list[str] | None:
    if class_name != 'DataFrame':
        return None

    if name == 'bfill':
        return render_custom_method_page(
            class_name,
            name,
            overloads,
            'Backward-fills missing values feature-by-feature by pulling the next valid value upward. Use it when a later observed value should repair a preceding missing gap.',
            [
                '`limit` : `std::int64_t`, optional',
                '    Maximum number of consecutive missing rows to fill within each gap. Use `-1` for no limit.',
                '',
                '`options` : `FillOptions`',
                '    Structured fill configuration for the overload that targets selected features.',
                '    - `features`: feature subset to fill. Leave it empty to fill every compatible feature.',
                '    - `limit`: maximum number of consecutive rows to fill per gap.',
            ],
            [
                '`DataFrame`',
                '    A new frame with the selected missing values backward-filled.',
            ],
            [
                (
                    'Backward-fill every compatible feature with the default unlimited policy.',
                    df_b_missing_base() + '\n\nauto out = missing.bfill();',
                    """===========================
     | score  cost  city
---------------------------
0    | 72     10    Accra
1    | 81     40    Tema
2    | 81     40    Kumasi
3    | 95     40    Tamale
4    | 95     NA    Sunyani
---------------------------
[5 cs x 3 feat]""",
                ),
                (
                    'Backward-fill only one feature with a feature-scoped FillOptions object.',
                    df_b_missing_base() + '\n\nDataFrame::FillOptions fill_options;\nfill_options.features = {"score"};\nfill_options.limit = 1;\n\nauto out = missing.bfill(fill_options);',
                    """===========================
     | score  cost  city
---------------------------
0    | 72     10    Accra
1    | 81     nan   Tema
2    | 81     nan   Kumasi
3    | 95     40    Tamale
4    | 95     nan   Sunyani
---------------------------
[5 cs x 3 feat]""",
                ),
            ],
            [
                '- `bfill(...)` operates independently inside each feature.',
                '- Trailing missing runs remain missing when there is no later value to pull from.',
            ],
        )

    if name == 'bottom':
        return render_custom_method_page(
            class_name,
            name,
            overloads,
            'Returns the lowest `n` rows after sorting ascending by one feature.',
            [
                '`feature` : `std::string_view`',
                '    Feature used for the ascending sort key.',
                '',
                '`n` : `std::int64_t`',
                '    Number of rows to keep from the bottom of the ascending ordering.',
            ],
            [
                '`DataFrame`',
                '    A new frame containing the selected bottom rows.',
            ],
            [
                (
                    'Pick the two lowest rows by final score.',
                    df_b_grades_base() + '\n\nauto out = grades.bottom("final", 2);',
                    """=================================
     | student  assignment  final
---------------------------------
s3   | Chen     7.5         21.5
s2   | Bola     8           24
---------------------------------
[2 cs x 3 feat]""",
                ),
            ],
            [
                '- `bottom(feature, n)` is the lower-tail companion to `top(feature, n)`.',
                '- The output preserves the full row payload for every selected case.',
            ],
        )

    if name == 'bounded':
        return render_custom_method_page(
            class_name,
            name,
            overloads,
            'Applies a window statistic using caller-supplied window bounds instead of a fixed rolling width. This is the escape hatch for advanced vectorized window definitions that do not fit ordinary rolling or expanding patterns.',
            [
                '`bounds` : `WindowBoundsFn`',
                '    Callable with signature `(size, options) -> pair(starts, ends)` where each row receives one half-open window `[start, end)`.',
                '',
                '`statistic` : `WindowStatistic`, optional',
                '    Reduction applied inside each window:',
                '    - `Sum`: sum inside each window.',
                '    - `Mean`: arithmetic mean inside each window.',
                '    - `Minimum`: minimum inside each window.',
                '    - `Maximum`: maximum inside each window.',
                '    - `Var`: variance inside each window.',
                '    - `Std`: standard deviation inside each window.',
                '    - `Sem`: standard error of the mean.',
                '    - `Skew`: skewness.',
                '    - `Kurtosis`: kurtosis.',
                '',
                '`options` : `WindowOptions`, optional',
                '    Additional validity/statistics controls.',
                '    - `min_count`: minimum non-null observations required for a valid output.',
                '    - `ddof`: degrees of freedom for variance-style statistics.',
                '    - `centered`: centering flag used by APIs that interpret a natural window center.',
                '    - `closure`: interval-closure policy used by APIs that interpret left/right boundary semantics.',
            ],
            [
                '`DataFrame`',
                '    A new frame containing one windowed result per input row and per selected feature.',
            ],
            [
                (
                    'Build an expanding mean by making every window start at row zero.',
                    df_b_metrics_base() + '\n\nauto out = numeric.ts().bounded(\n    [](std::int64_t size, const WindowOptions&) {\n        std::vector<std::int64_t> starts(static_cast<std::size_t>(size), 0);\n        std::vector<std::int64_t> ends;\n        ends.reserve(static_cast<std::size_t>(size));\n        for (std::int64_t row = 0; row < size; ++row) ends.push_back(row + 1);\n        return std::pair{std::move(starts), std::move(ends)};\n    },\n    WindowStatistic::Mean);\n',
                    """===============================
     | sales        cost
-------------------------------
0    | 12           5
1    | 19.5         11.5
2    | 24.66666667  18.66666667
3    | 30.75        29
-------------------------------
[4 cs x 2 feat]""",
                ),
                (
                    'Build a trailing-two mean with a minimum-count requirement.',
                    df_b_metrics_base() + '\n\nauto out = numeric.ts().bounded(\n    [](std::int64_t size, const WindowOptions&) {\n        std::vector<std::int64_t> starts;\n        std::vector<std::int64_t> ends;\n        starts.reserve(static_cast<std::size_t>(size));\n        ends.reserve(static_cast<std::size_t>(size));\n        for (std::int64_t row = 0; row < size; ++row) {\n            starts.push_back(std::max<std::int64_t>(0, row - 1));\n            ends.push_back(row + 1);\n        }\n        return std::pair{std::move(starts), std::move(ends)};\n    },\n    WindowStatistic::Mean,\n    WindowOptions{.min_count = 2});\n',
                    """==================
     | sales  cost
------------------
0    | NA     NA
1    | 19.5   11.5
2    | 31     25.5
3    | 42     46.5
------------------
[4 cs x 2 feat]""",
                ),
            ],
            [
                '- `bounded(...)` is the lower-level custom-window surface behind more regular rolling and expanding helpers.',
                '- The bounds function defines the window shape; the statistic and options define how each window is reduced.',
            ],
        )

    if name == 'bucketize':
        return render_custom_method_page(
            class_name,
            name,
            overloads,
            'Places numeric values into interval bins and returns the bin labels as numeric outputs. Use it when you want coarse interval categories while keeping the operation inside the DataFrame pipeline.',
            [
                '`feature_name` / `feature_names` : feature selector',
                '    Choose one feature, several features, or leave the selector out entirely to apply bucketing across every compatible numeric feature.',
                '',
                '`bins` : `pluss::RangeView<double>`',
                '    Strictly increasing interval edges.',
                '',
                '`labels` : `pluss::RangeView<std::int64_t>`, optional',
                '    Explicit output labels for each interval. Leave it empty to get `0, 1, 2, ...`.',
                '',
                '`right` : `bool`, optional',
                '    When `true`, intervals are right-closed. When `false`, intervals are left-closed and right-open.',
                '',
                '`include_lowest` : `bool`, optional',
                '    When `true`, the first interval includes its left edge.',
            ],
            [
                '`DataFrame`',
                '    A new frame where the selected numeric features hold their assigned bucket labels.',
            ],
            [
                (
                    'Bucketize every compatible numeric feature with auto-generated labels.',
                    df_b_metrics_base().replace('\nauto numeric = metrics.features({"sales", "cost"});', '') + '\n\nauto out = metrics.bucketize({0.0, 20.0, 40.0, 60.0});',
                    """==========================
     | sales  cost  city
--------------------------
0    | 0      0     Accra
1    | 1      0     Tema
2    | 1      1     Kumasi
3    | 2      2     Tamale
--------------------------
[4 cs x 3 feat]""",
                ),
                (
                    'Bucketize one named feature with explicit labels.',
                    df_b_metrics_base().replace('\nauto numeric = metrics.features({"sales", "cost"});', '') + '\n\nauto out = metrics.bucketize("sales", {0.0, 20.0, 40.0, 60.0}, {10, 20, 30});',
                    """==========================
     | sales  cost  city
--------------------------
0    | 10     5     Accra
1    | 20     18    Tema
2    | 20     33    Kumasi
3    | 30     60    Tamale
--------------------------
[4 cs x 3 feat]""",
                ),
                (
                    'Bucketize a selected subset while leaving the other features untouched.',
                    """using namespace pluss::table;

auto metrics = DataFrame({
    {"sales", pluss::make_series({12.0, 27.0, 35.0, 49.0})},
    {"cost", pluss::make_series({5.0, 18.0, 33.0, 60.0})},
    {"city", pluss::make_note({"Accra", "Tema", "Kumasi", "Tamale"})},
}).title("metrics");
std::array<std::string_view, 2> pair_features{"sales", "cost"};

auto out = metrics.bucketize(pair_features, {0.0, 25.0, 50.0, 75.0}, {1, 2, 3});""",
                    """==========================
     | sales  cost  city
--------------------------
0    | 1      1     Accra
1    | 2      1     Tema
2    | 2      2     Kumasi
3    | 2      3     Tamale
--------------------------
[4 cs x 3 feat]""",
                ),
            ],
            [
                '- Bucketing changes only the selected numeric features.',
                '- Non-selected and non-numeric features pass through unchanged.',
            ],
        )

    return None


def render_thread_b_page(name: str, overloads: list[Method], class_name: str) -> list[str] | None:
    if class_name != 'Thread':
        return None

    if name == 'bfill':
        return render_custom_method_page(
            class_name,
            name,
            overloads,
            'Backward-fills one thread by pulling the next valid value upward within the same payload.',
            [
                '`limit` : `std::int64_t`, optional',
                '    Maximum number of consecutive missing entries to fill in each gap. Use `-1` for no limit.',
            ],
            [
                '`Thread`',
                '    A new thread with backward-filled values.',
            ],
            [
                (
                    'Backward-fill the whole gap with no limit.',
                    thread_b_missing_base() + '\n\nauto out = missing.bfill();',
                    """0  10
1  40
2  40
3  40
4  50
ptype: Series""",
                ),
                (
                    'Limit the backward fill to one value per gap.',
                    thread_b_missing_base() + '\n\nauto out = missing.bfill(1);',
                    """0  10
1  NA
2  40
3  40
4  50
ptype: Series""",
                ),
            ],
            [
                '- `bfill(limit)` works within one thread only.',
                '- A limited fill can leave part of a longer missing run untouched.',
            ],
        )

    if name == 'bounded':
        return render_custom_method_page(
            class_name,
            name,
            overloads,
            'Applies a window statistic using caller-supplied row bounds instead of a fixed rolling width.',
            [
                '`bounds` : `WindowBoundsFn`',
                '    Callable returning one `[start, end)` window per row.',
                '',
                '`statistic` : `WindowStatistic`, optional',
                '    Supported values:',
                '    - `Sum`',
                '    - `Mean`',
                '    - `Minimum`',
                '    - `Maximum`',
                '    - `Var`',
                '    - `Std`',
                '    - `Sem`',
                '    - `Skew`',
                '    - `Kurtosis`',
                '',
                '`options` : `WindowOptions`, optional',
                '    Additional validity/statistics controls.',
                '    - `min_count`',
                '    - `ddof`',
                '    - `centered`',
                '    - `closure`',
            ],
            [
                '`Thread`',
                '    A new thread with one reduced value per input row.',
            ],
            [
                (
                    'Create an expanding mean by anchoring every window at row zero.',
                    thread_b_scores_base() + '\n\nauto out = scores.bounded(\n    [](std::int64_t size, const WindowOptions&) {\n        std::vector<std::int64_t> starts(static_cast<std::size_t>(size), 0);\n        std::vector<std::int64_t> ends;\n        ends.reserve(static_cast<std::size_t>(size));\n        for (std::int64_t row = 0; row < size; ++row) ends.push_back(row + 1);\n        return std::pair{std::move(starts), std::move(ends)};\n    },\n    WindowStatistic::Mean);',
                    """0  10
1  15
2  20
3  25
4  30
ptype: Series""",
                ),
                (
                    'Create a trailing-two mean with a minimum-count requirement.',
                    thread_b_scores_base() + '\n\nauto out = scores.bounded(\n    [](std::int64_t size, const WindowOptions&) {\n        std::vector<std::int64_t> starts;\n        std::vector<std::int64_t> ends;\n        starts.reserve(static_cast<std::size_t>(size));\n        ends.reserve(static_cast<std::size_t>(size));\n        for (std::int64_t row = 0; row < size; ++row) {\n            starts.push_back(std::max<std::int64_t>(0, row - 1));\n            ends.push_back(row + 1);\n        }\n        return std::pair{std::move(starts), std::move(ends)};\n    },\n    WindowStatistic::Mean,\n    WindowOptions{.min_count = 2});',
                    """0  NA
1  15
2  25
3  35
4  45
ptype: Series""",
                ),
            ],
            [
                '- `bounded(...)` is the thread-level custom-window primitive.',
                '- The bounds function controls which rows participate in each output position.',
            ],
        )

    if name == 'btw':
        return render_custom_method_page(
            class_name,
            name,
            overloads,
            'Checks whether each value falls between lower and upper bounds. The supported overloads cover numeric scalar bounds, temporal string bounds, and row-wise thread bounds.',
            [
                '`a`, `b` : `double`',
                '    Lower and upper numeric bounds for Series-backed numeric threads.',
                '',
                '`lo`, `hi` : `std::string_view`',
                '    Lower and upper temporal bounds for Timeline/TimeIndex-backed threads.',
                '',
                '`lo`, `hi` : `Thread`',
                '    Row-wise lower and upper bounds. Both bound threads must match the source thread length.',
                '',
                '`inclusive` : `bool`, optional',
                '    Shortcut flag that applies the same closed/open choice to both sides.',
                '',
                '`include_left`, `include_right` : `bool`, optional',
                '    Side-specific boundary control for asymmetric intervals.',
            ],
            [
                '`Thread`',
                '    A boolean-like thread indicating whether each value falls inside the requested interval.',
            ],
            [
                (
                    'Numeric bounds with one shared inclusivity flag.',
                    thread_b_scores_base() + '\n\nauto out = scores.btw(20.0, 40.0, true);',
                    """0  false
1  true
2  true
3  true
4  false
ptype: Series""",
                ),
                (
                    'Numeric bounds with separate left and right boundary rules.',
                    thread_b_scores_base() + '\n\nauto out = scores.btw(20.0, 40.0, false, true);',
                    """0  false
1  false
2  true
3  true
4  false
ptype: Series""",
                ),
                (
                    'Temporal string bounds with one shared inclusivity flag.',
                    thread_b_dates_base() + '\n\nauto out = dates.btw("2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z", true);',
                    """0  true
1  true
2  false
ptype: Series""",
                ),
                (
                    'Temporal string bounds with asymmetric closure.',
                    thread_b_dates_base() + '\n\nauto out = dates.btw("2024-01-01T00:00:00Z", "2024-01-03T00:00:00Z", false, true);',
                    """0  false
1  true
2  true
ptype: Series""",
                ),
                (
                    'Row-wise thread bounds with one shared inclusivity flag.',
                    thread_b_rowwise_base() + '\n\nauto out = scores.btw(lower, upper, true);',
                    """0  true
1  true
2  false
3  true
4  true
ptype: Series""",
                ),
                (
                    'Row-wise thread bounds with asymmetric closure.',
                    thread_b_rowwise_base() + '\n\nauto out = scores.btw(lower, upper, false, true);',
                    """0  false
1  false
2  false
3  true
4  true
ptype: Series""",
                ),
            ],
            [
                '- Use the numeric overloads for numeric Series payloads.',
                '- Use the string-bound overloads for Timeline or TimeIndex payloads.',
                '- Use the thread-bound overloads when each row needs its own interval.',
            ],
        )

    if name == 'bucketize':
        return render_custom_method_page(
            class_name,
            name,
            overloads,
            'Places numeric thread values into interval bins and returns the assigned labels.',
            [
                '`bins` : `pluss::RangeView<double>`',
                '    Strictly increasing interval edges.',
                '',
                '`labels` : `pluss::RangeView<std::int64_t>`, optional',
                '    Explicit labels for the intervals. Leave it empty to get `0, 1, 2, ...`.',
                '',
                '`right` : `bool`, optional',
                '    When `true`, intervals are right-closed.',
                '',
                '`include_lowest` : `bool`, optional',
                '    When `true`, the first interval includes its left edge.',
            ],
            [
                '`Thread`',
                '    A numeric thread holding one bucket label per input value.',
            ],
            [
                (
                    'Bucketize with automatically generated labels.',
                    thread_b_scores_base() + '\n\nauto out = scores.bucketize({0.0, 20.0, 40.0, 60.0});',
                    """0  0
1  0
2  1
3  1
4  2
ptype: Series""",
                ),
                (
                    'Bucketize with explicit labels.',
                    thread_b_scores_base() + '\n\nauto out = scores.bucketize({0.0, 20.0, 40.0, 60.0}, {10, 20, 30}, true, true);',
                    """0  10
1  10
2  20
3  20
4  30
ptype: Series""",
                ),
            ],
            [
                '- `bucketize(...)` is the thread-level primitive behind DataFrame bucketization.',
            ],
        )

    return None

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
AXEL_CONCAT_OUT = """depth0 -> Registry(ids -> Slot([Series(1), Series(2)]))
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
AXEL_BASE_OUT = """depth0 -> Registry(ids -> Slot([Series(1), Series(2)]))
depth1 -> Registry(ids -> Slot([Series(3), Series(4)]))"""
SLOT_BASE_OUT = "Slot([Series(1), Series(2), Series(3)])"


def labeled_pair(source_label: str, source_body: str, out_body: str, out_label: str = 'out.head()') -> str:
    return f"{source_label}:\n{source_body}\n\n{out_label}:\n{out_body}"


def output_for(class_name: str, method: Method) -> str:
    n = method.name
    ret = method.return_type.strip()

    if class_name == 'DataFrame':
        if n.startswith('load') and ret == 'pluss::loader::LoadResult':
            return """Sheet1 -> [3 cs x 3 feat]
Sheet2 -> [3 cs x 3 feat]"""
        if n in {'defined', 'unique'}:
            return 'true'
        if n in {'nrows', 'ncols', 'nfeatures', 'ncases', 'count_null_rows'}:
            return '3'
        if n in {'column_names', 'feature_names'}:
            return '["id", "sales", "city"]'
        if n == 'caselbls':
            return '["student", "term"]'
        if n == 'caselbl':
            if 'const Thread& labels' in method.signature:
                return """df.label().values():
["r0", "r1", "r2"]

out.label().values():
["cs0", "cs1", "cs2"]"""
            return '["r0", "r1", "r2"]'
        if n == 'caselbl_':
            return """df.label().values():
["cs0", "cs1", "cs2"]

df.head():
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi"""
        if n in {'columns', 'values', 'features'} and 'string_view' not in method.signature and 'initializer_list' not in method.signature:
            return '[Thread(id), Thread(sales), Thread(city)]'
        if n in {'at', 'iat'} and ('feature_name' in method.signature or 'feature_idx' in method.signature):
            return 'Accra' if 'feature_name' in method.signature else '1'
        if n == 'case_study':
            if 'feature_name' in method.signature:
                return 'Tema'
            if 'start, std::int64_t stop' in method.signature:
                return """id  sales  city
1   100.0  Accra
2   120.0  Tema"""
            return """id  sales  city
2   120.0  Tema"""
        if n in {'get', 'column', 'features'} and 'string_view' in method.signature:
            return 'Thread([100.0, 120.0, 90.0])'
        if n == 'feature_ref':
            return 'ColumnRef(name="sales")'
        if n in {'has_feature', 'equals'}:
            return 'true'
        if n in {'with_cases'}:
            return DF_WITH_CASES_OUT
        if n in {'with_cases_'}:
            return """df.label().values():
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
        if n in {'rename', 'rename_', 'rename_features', 'relabel_features', 'set_axis', 'set_axis'}:
            if n == 'rename_features':
                return """id  revenue  location
1   100.0    Accra
2   120.0    Tema
3    90.0    Kumasi"""
            if n in {'set_axis', 'set_axis'}:
                return """revenue  location  id_new
100.0    Accra     1
120.0    Tema      2
90.0     Kumasi    3"""
            return DF_RENAME_OUT
        if n == 'place':
            if 'const Thread& feature' in method.signature:
                return """id  sales  profit  city
1   100.0  40.0    Accra
2   120.0  35.0    Tema
3    90.0  30.0    Kumasi"""
            return """city    id  sales
Accra   1   100.0
Tema    2   120.0
Kumasi  3    90.0"""
        if n == 'caselbl' and ('std::string_view name' in method.signature or 'std::int64_t position' in method.signature):
            return '["Ada", "Ada", "Kojo", "Kojo"]' if 'std::string_view name' in method.signature else '["mid", "final", "mid", "final"]'
        if n in {'drop'}:
            return DF_DROP_OUT
        if n == 'dropnull':
            return DF_DROPNA_OUT
        if n in {'insert', 'set_features', 'set_features_', 'assign'}:
            return """id  sales  city    profit
1   100.0  Accra   40.0
2   120.0  Tema    35.0
3    90.0  Kumasi  30.0"""
        if n == 'note_features':
            return """city
Accra
Tema
Kumasi"""
        if n == 'bucketize':
            return """id  sales  city
1   2      Accra
2   3      Tema
3   1      Kumasi"""
        if n == 'bottom':
            return DF_BOTTOM_OUT
        if n in {'head', 'tail', 'slice', 'take', 'sample', 'where', 'filter', 'query', 'top', 'unique_cases', 'observe'}:
            if n == 'query':
                return """id  sales  city
1   100.0  Accra
2   120.0  Tema"""
            if n == 'tail':
                return """id  sales  city
2   120.0  Tema
3    90.0  Kumasi"""
            if n == 'observe':
                if 'start, std::int64_t stop' in method.signature:
                    return """id  sales  city
1   100.0  Accra
2   120.0  Tema"""
                return """id  sales  city
2   120.0  Tema"""
            if n == 'top':
                return """id  sales  city
2   120.0  Tema
1   100.0  Accra"""
            if n == 'sample':
                if ' frac' in method.signature:
                    return """id  sales  city
1   100.0  Accra
3    90.0  Kumasi"""
                if 'std::uint64_t random_state' not in method.signature and 'bool replace, int axis' not in method.signature:
                    return """id  sales  city
1   100.0  Accra
3    90.0  Kumasi"""
                return """rows.head():
id  sales  city
1   100.0  Accra
3    90.0  Kumasi

cols.head():
sales  city
100.0  Accra
120.0  Tema
90.0   Kumasi"""
            if n == 'unique_cases':
                return """campus  student  score
North   Ada      88.0
West    Esi      84.0"""
            return DF_FILTER_OUT if n in {'where', 'filter', 'take'} else DF_BASE_OUT
        if n in {'label().align'}:
            return DF_ALIGN_CASES_OUT
        if n == 'match':
            return """matched.left.label().values():
["A", "C", "B"]

matched.left.features():
["x", "y", "z"]"""
        if n == 'extend':
            return """case  x     y     z
A     1.0   3.0   null
C     2.0   4.0   null
B     null  5.0   7.0
A     null  6.0   8.0"""
        if n == 'lengthen':
            return """id  feature  value
r1  a        1.0
r2  a        2.0
r1  b        3.0
r2  b        4.0"""
        if n == 'label().repeated':
            return '[false, true, false]'
        if n in {'sort_values', 'label().sort', 'reindex_like', 'reindex_cases', 'conform'}:
            if n == 'sort_values':
                return """id  sales  city
2   120.0  Tema
1   100.0  Accra
3    90.0  Kumasi"""
            if n == 'label().sort':
                return """with_cases.label().values():
["cs2", "cs0", "cs1"]

out.label().values():
["cs0", "cs1", "cs2"]

out.head():
id  sales  city
2   120.0  Tema
3    90.0  Kumasi
1   100.0  Accra"""
            return DF_BASE_OUT
        if n in {'join', 'update', 'combine'}:
            return """id  sales  cost
1   100.0  60.0
2   120.0  80.0
3    90.0  55.0"""
        if n == 'unify':
            if 'initializer_list<std::string_view> left_on' in method.signature:
                return """course   OrgDefinedId  midterm  course_code  Username  Final Points Grade
STAT200  20            82       STAT200      20        88
STAT300  30            93       STAT300      30        99"""
            return """OrgDefinedId  midterm  Final Points Grade
10            71       null
20            82       88
30            93       99"""
        if n == 'reorder':
            if 'initializer_list<ReorderSpec>' in method.signature:
                return """out.features():
["col_25", "col_10", "col_1", "col_2", "col_3", "col_4", "col_20", "col_5", "col_6", "col_7", "col_8", "col_15", "col_16", "col_17", "col_9", "col_21", "col_22", "col_23", "col_24"]

out.head():
col_25  col_10  col_1  col_2  col_3  col_4  col_20  col_5  col_6  col_7  col_8  col_15  col_16  col_17  col_9  col_21  col_22  col_23  col_24
25.0    10.0    1.0    2.0    3.0    4.0    20.0    5.0    6.0    7.0    8.0    15.0    16.0    17.0    9.0    21.0    22.0    23.0    24.0
125.0   110.0   101.0  102.0  103.0  104.0  120.0   105.0  106.0  107.0  108.0  115.0   116.0   117.0   109.0  121.0   122.0   123.0   124.0"""
            if 'pluss::RangeView<ReorderSpec>' in method.signature:
                return """city    sales  id
Accra   100.0  1
Tema    120.0  2
Kumasi  90.0   3"""
            if 'ReorderSpec' in method.signature:
                return """id  city    sales
1   Accra   100.0
2   Tema    120.0
3   Kumasi   90.0"""
            return DF_REORDER_OUT
        if n == 'shuffle':
            return """id  sales  city
3    90.0  Kumasi
1   100.0  Accra
2   120.0  Tema"""
        if n == 'reshuffle':
            return """rows.head():
id  sales  city
3    90.0  Kumasi
1   100.0  Accra
2   120.0  Tema

cols.head():
city    id  sales
Accra   1   100.0
Tema    2   120.0
Kumasi  3    90.0"""
        if n in {'group'}:
            return DF_GROUP_OUT
        if n in {'count_by', 'tally'}:
            return DF_COUNT_BY_OUT
        if n in {'mean_by'}:
            return DF_MEAN_BY_OUT
        if n in {'pivot', 'pivot_table'}:
            return DF_PIVOT_OUT
        if n in {'aggregate'}:
            return DF_SUMMARY_OUT
        if n == 'select_like':
            return """midterm_1  midterm_2
70.0       75.0
80.0       85.0
90.0       95.0"""
        if n == 'select':
            return """id  sales
1   100.0
2   120.0
3    90.0"""
        if n == 'corr':
            return DF_CORR_OUT
        if n == 'corr_with':
            if 'RangeView::Of<DataFrame> others' in method.signature:
                return """frame_0  frame_1
[1.0, 1.0]  [1.0, 1.0]"""
            return """id     sales
1.0000 0.981981"""
        if n in {'describe'}:
            return DF_DESCRIBE_OUT
        if n in {'transpose'}:
            return DF_TRANSPOSE_OUT
        if n == 'matmul':
            return """c     d
23.0  31.0
34.0  46.0"""
        if n == 'count':
            return DF_COUNT_OUT
        if n == 'percentiles':
            return """p      id   sales
0.25   1.5  95.0
0.50   2.0  100.0
0.75   2.5  110.0"""
        if n == 'prod':
            if 'axis' in method.signature:
                return """[2.0, 8.0, 15.0]"""
            return """feature  value
a        6.0
b        40.0"""
        if n == 'mode':
            return """id  sales  city
1   90.0   Accra"""
        if n == 'sum':
            return """feature  value
id       6
sales    310.0"""
        if n == 'mean':
            return """feature  value
id       2.0
sales    103.333333333"""
        if n == 'median':
            return """feature  value
id       2.0
sales    100.0"""
        if n == 'min':
            return """feature  value
id       1
sales    90.0"""
        if n == 'max':
            return """feature  value
id       3
sales    120.0"""
        if n == 'std':
            return """feature  value
id       1.0
sales    15.275252317"""
        if n == 'var':
            return """feature  value
id       1.0
sales    233.333333333"""
        if n == 'bfill':
            return DF_BFILL_OUT
        if n in {'fillnull', 'fillnull_', 'ffill', 'isna', 'isnull'}:
            return DF_BASE_OUT
        if n in {'notna', 'notnull'}:
            return """sales
true
false
true"""
        if n in {'cast', 'astype', 'cast_series', 'cast_note', 'cast_timeline', 'cast_timeindex'}:
            return DF_BASE_OUT
        if n == 'shift_time':
            return """ts
2024-01-01T10:10:00Z
2024-01-01T10:40:00Z
2024-01-01T11:05:00Z"""
        if n in {'add'}:
            return DF_PLUS10_OUT if 'const DataFrame&' not in method.signature else """id  sales
3   210.0
4   230.0
7   190.0"""
        if n == 'radd':
            return """a    b
11.0  12.0
12.0  14.0
13.0  15.0"""
        if n in {'mul', 'multiply'}:
            return """id  sales  city
10  1000.0  Accra
20  1200.0  Tema
30   900.0  Kumasi""" if 'const DataFrame&' not in method.signature else """id  sales
2   11000.0
4   13200.0
12   9000.0"""
        if n == 'rmul':
            return """a    b
3.0  6.0
6.0  12.0
9.0  15.0"""
        if n in {'mod'}:
            return """id  sales  city
1   0.0    Accra
2   0.0    Tema
3   0.0    Kumasi""" if 'const DataFrame&' not in method.signature else """id  sales
1   100.0
0    10.0
3    90.0"""
        if n == 'rmod':
            return """a    b
0.0  0.0
0.0  0.0
0.0  0.0"""
        if n in {'sub', 'subtract'}:
            return """id  sales  city
-9  90.0   Accra
-8  110.0  Tema
-7  80.0   Kumasi""" if 'const DataFrame&' not in method.signature else DF_BASE_OUT
        if n in {'div', 'divide', 'truediv'}:
            return """id  sales  city
0.1  10.0  Accra
0.2  12.0  Tema
0.3   9.0  Kumasi""" if 'const DataFrame&' not in method.signature else """id    sales
0.5   0.909091
1.0   1.090909
0.75  0.9"""
        if n in {'floordiv'}:
            return """id  sales  city
0.0  10.0  Accra
0.0  12.0  Tema
0.0   9.0  Kumasi""" if 'const DataFrame&' not in method.signature else DF_BASE_OUT
        if n == 'pow':
            if 'const DataFrame&' in method.signature:
                return """a    b
8.0  9.0
9.0  8.0
4.0  25.0"""
            return """a    b
1.0  4.0
4.0  16.0
9.0  25.0"""
        if n == 'rsub':
            return """a    b
9.0  8.0
8.0  6.0
7.0  5.0"""
        if n in {'rdiv', 'rtruediv'}:
            return """a         b
10.0      5.0
5.0       2.5
3.333333  2.0"""
        if n == 'rfloordiv':
            return """a   b
10  5
5   2
3   2"""
        if n == 'rpow':
            return """a      b
10.0   100.0
100.0  10000.0
1000.0 100000.0"""
        if n in {'eq', 'ne', 'le', 'gt', 'ge', 'compare'}:
            return """id  sales
1   true
2   true
3   false"""
        if n == 'lt':
            return """id  sales
1   true
2   false
3   true"""
        if n == 'view':
            return """id  sales  city
1   100.0  Accra
2   120.0  Tema"""
        if n in {'copy'}:
            return DF_BASE_OUT
        if n == 'matrix':
            return """series blocks: 1
note blocks: 1
timeline blocks: 0
timeindex blocks: 0"""
        if n == 'timeline_features':
            return """start                 end
2020-01-01T00:00:00Z  2020-01-01T01:30:00Z
2020-01-02T08:00:00Z  2020-01-02T09:00:00Z"""
        if n == 'title':
            if method.return_type.strip() == 'std::string':
                return 'sales_report'
            return """out.title():
sales_report

out.head():
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi"""
        if n == 'title_':
            return """df.title():
sales_report

df.head():
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi"""
        if n == 'ts':
            return 'DataFrameTsAccessor(features=["load"])'
        if n == 'note':
            return 'DataFrameNoteAccessor(features=["city"])'
        if n == 'tl':
            return 'DataFrameTimelineAccessor(features=["ts"])'
        if n in {'to_string', 'info_string'}:
            return DF_BASE_OUT
        if n in {'info'}:
            return "rows: 3\nfeatures: 3\nnumeric: id, sales\nnote: city"
        if n == 'show':
            return DF_BASE_OUT
        if n in {'rolling', 'window'}:
            return """sales_roll
100.0
110.0
105.0"""
        if n == 'scan':
            return """id  sales
1   100.0
3   220.0
6   310.0"""
        if n == 'replace':
            if 'std::string_view from, std::string_view to' in method.signature or 'const char* from, const char* to' in method.signature:
                return """midterm_1  midterm_2  city
0.0        8.0        Accra
12.0       0.0        Tamale
0.0        15.0       Tamale"""
            return """midterm_1  midterm_2  city
5.0        8.0        Accra
12.0       5.0        Tema
5.0        15.0       Tema"""
        if n == 'put':
            return """id  sales  city
1   100.0  Accra
2   999.0  Tema
3    90.0  Kumasi"""
        if n == 'put_':
            return """id  sales  city
1   100.0  Accra
2   120.0  Tamale
3    90.0  Kumasi"""
        if n == 'tabelize':
            return """sales
10.0
20.0
30.0"""
        if n == 'transform':
            if 'const std::function<Thread(const DataFrame&)>&' in method.signature:
                return """id  sales  city    sales_plus_10
1   100.0  Accra   110.0
2   120.0  Tema    130.0
3    90.0  Kumasi  100.0"""
            return """id  sales  city
1   100.0  Accra
2   110.0  Tema
3    90.0  Kumasi"""
        if n == 'unsqueeze':
            if 'depth_labels' in method.signature:
                return """depth labels:
["train"]

frame0:
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi"""
            return """frame0:
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi"""
        if n.startswith('save_') or n == 'save_as':
            return 'saved: out.*'
        if n == 'plot':
            return 'Figure(line/scatter/bar/hist)'
        if n == 'plot3d':
            return 'Figure(scatter3d/line3d)'
        if n == 'sql':
            return """id  sales
1   100.0
2   120.0"""
        if n == 'prep_sql':
            return 'SQL accessor / prepared statement created'
        if n == 'unstack':
            return """score_mid  score_final
88.0       91.0
84.0       89.0"""
        if n in {'unsqueeze'}:
            return ATLAS_BASE_OUT
        return DF_BASE_OUT if ret.startswith('DataFrame') else ''

    if class_name == 'Thread':
        if n in {'defined', 'unique'}:
            return 'true'
        if n == 'size':
            return '3'
        if n == 'kind':
            return 'Series'
        if n == 'dtype':
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
        if n == 'distinct':
            return THREAD_DISTINCT_OUT
        if n == 'display_labels':
            return THREAD_DISPLAY_LABELS_OUT
        if n == 'display_name':
            return THREAD_DISPLAY_NAME_OUT
        if n == 'describe':
            return THREAD_DESCRIBE_NUMERIC_OUT
        if n in {'btw', 'between'}:
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
        if n == 'dropnull':
            return '[10.0, 30.0]'
        if n in {'abs', 'clip', 'log', 'exp', 'fillnull', 'fillnull_', 'head', 'tail', 'slice', 'take', 'filter', 'ffill', 'lower', 'replace', 'window', 'cast', 'astype', 'clear_caselbl', 'clear_caselbl_'}:
            return THREAD_SERIES_OUT if 'Note' not in method.signature and n not in {'lower', 'replace'} else THREAD_NOTE_OUT
        if n == 'count':
            return THREAD_COUNT_OUT
        if n == 'corr':
            return THREAD_CORR_OUT
        if n in {'sum', 'mean', 'min', 'max', 'var', 'std'}:
            return '20.0'
        if n in {'to_note_weekday'}:
            return '[2024, 2024, 2024]' if n == 'year' else '[1, 2, 3]'
        if n in {'numeric_binary', 'compare'}:
            if n == 'numeric_binary':
                return '[20.0, 30.0, 40.0]'
            if n == 'compare':
                return THREAD_COMPARE_SCALAR_OUT if 'const Thread&' not in method.signature else '[false, true, false]'
        if n in {'to_string'}:
            return THREAD_SERIES_OUT
        if n == 'show':
            return THREAD_SERIES_OUT
        if n == 'has_caselbl':
            return 'true'
        if n == 'caselbl':
            if 'const Thread& labels' in method.signature:
                return """thread.label().values():
["row0", "row1", "row2"]

out.label().values():
["A", "B", "C"]"""
            return '["row0", "row1", "row2"]'
        if ret == 'Thread':
            if n in {'chars', 'lower', 'strip', 'lstrip', 'rstrip'}:
                return THREAD_NOTE_OUT
            if n in {'year', 'month', 'day', 'hour', 'to_note_weekday'}:
                return '[2024, 2024, 2024]' if n == 'year' else '[1, 2, 3]'
            if n in {'clear_display_labels', 'clear_display_name'}:
                return THREAD_SERIES_OUT
            return THREAD_SERIES_OUT
        if ret == 'Thread&':
            return THREAD_SERIES_OUT
        if ret == 'ThreadValue':
            return 'true' if n in {'all', 'any'} else '10.0'
        if ret == 'std::string':
            return 'sales'
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
        if ret == 'bool':
            return 'true'
        if ret == 'std::string':
            return 'Series'
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
        if n == 'heatmap':
            return 'Figure(heatmap)'
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
        if n == 'hist':
            return 'Figure(hist)'
        if n == 'lag':
            return 'Figure(lag)'
        if n == 'line':
            return 'Figure(line)'
        if n == 'parallel':
            return 'Figure(parallel)'
        if n == 'pie':
            return 'Figure(pie)'
        if n == 'radviz':
            return 'Figure(radviz)'
        if n == 'scatter':
            return 'Figure(scatter)'
        if n == 'scatter_matrix':
            return 'Figure(scatter_matrix)'
        if n == 'step':
            return 'Figure(step)'
        if n == 'violin':
            return 'Figure(violin)'
        return 'Figure(...)'

    if class_name == 'DataFrameSqlAccessor':
        if n == 'execute':
            return """id  sales
1   100.0
2   120.0"""
        return 'Prepared statement ready'

    if class_name == 'DataFrameSqlPrepared':
        if n == 'execute':
            return """id  sales
1   100.0
2   120.0"""
        return ''

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
        if n == 'dtype':
            return 'Float64'
        if n in {'has_cases', 'has_meta'}:
            return 'true'
        if n in {}:
            return '[true, false, true]'
        if n in {'cases'}:
            return '["cs0", "cs1", "cs2"]'
        if n in {'meta'}:
            return 'km'
        if n in {'isnull', 'notnull'}:
            return STREAM_SERIES_OUT
        if n == 'bfill':
            return STREAM_BFILL_OUT
        if n == 'dropnull':
            return 'Series([10.0, 30.0])'
        if n in {'fillnull', 'fillnull_', 'abs', 'clip', 'log', 'exp', 'with_cases', 'with_cases_', 'clear_cases', 'clear_cases_', 'with_meta', 'with_meta_', 'drop_meta', 'drop_meta_', 'head', 'tail', 'slice', 'take', 'filter', 'ffill', 'cast', 'astype'}:
            return STREAM_SERIES_OUT if 'Note' not in method.signature and n not in {'fillnull'} else STREAM_NOTE_OUT
        if n in {'sum', 'mean', 'min', 'max', 'var', 'std'}:
            return 'Series([20.0])'
        if n in {'all', 'any'}:
            return 'true'
        if n in {'as_series'}:
            return STREAM_SERIES_OUT
        if n in {'as_note'}:
            return STREAM_NOTE_OUT
        if n == 'as_timeline':
            return 'Timeline([...])'
        if n == 'as_timeindex':
            return 'TimeIndex([...])'
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
        if n in {'has'}:
            return 'true'
        if n in {'at', 'get'}:
            return 'Series([10.0, 20.0, 30.0])'
        if n == 'bfill':
            return CATALOG_BFILL_OUT
        if n == 'drop':
            return CATALOG_DROP_OUT
        if n == 'dropnull':
            return CATALOG_DROPNA_OUT
        if n == 'keywords':
            return CATALOG_KEYWORDS_OUT
        if n == 'featurize':
            return CATALOG_TEXT_FEATURES_OUT
        if n == 'stats':
            return CATALOG_TEXT_STATS_OUT
        if n == 'similarity':
            return CATALOG_SIMILARITY_OUT
        if n in {'head', 'tail', 'slice', 'take', 'sample', 'filter', 'where', 'query', 'select', 'rename', 'rename_', 'sort', 'fillnull', 'ffill', 'isnull', 'notnull', 'cast', 'astype', 'merge', 'apply', 'apply_', 'map', 'map_', 'transform', 'clean', 'grep', 'tfidf', 'set', 'set_'}:
            return labeled_pair('ct.head()', CATALOG_BASE_OUT, CATALOG_BASE_OUT)
        if n in {'sum', 'mean', 'min', 'max'}:
            return 'Series([20.0])'
        if n in {'group'}:
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
        if n in {'has', 'has_slot', 'has_all', 'has_any'}:
            return 'true'
        if n in {'slot', 'at', 'get', 'get_slot'}:
            return SLOT_BASE_OUT
        if n == 'clear':
            return REGISTRY_CLEAR_OUT
        if n == 'combine':
            return REGISTRY_COMBINE_OUT
        if n == 'concat':
            return REGISTRY_CONCAT_OUT
        if n == 'drop':
            return REGISTRY_DROP_OUT
        if n == 'prune':
            return REGISTRY_DROP_EMPTY_OUT
        if n in {'head', 'tail', 'slice', 'iloc', 'take', 'sample', 'select', 'rename', 'rename_', 'apply', 'apply_', 'map', 'map_', 'clear', 'clear_', 'set_slot', 'set_slot_', 'set_slots', 'set_slots_', 'push', 'push_', 'keep', 'keep_', 'morph', 'morph_', 'take_', 'concat', 'concat_'}:
            return labeled_pair('rg.head()', REGISTRY_BASE_OUT, REGISTRY_BASE_OUT)
        if n in {'slot_sizes'}:
            return '{"ids": 3, "city": 3}'
        if n in {'profiles'}:
            return '{"ids": {"Series": 3}, "city": {"Note": 3}}'
        if n in {'to_kindle', 'as_kindle', 'kindelize'}:
            return CATALOG_BASE_OUT
        if n in {'from_kindle'} or n.startswith('load'):
            return f"out.head():\n{REGISTRY_BASE_OUT}"
        if n.startswith('save_') or n == 'save_as':
            return 'saved: out.*'
        if n in {'to_string', 'to_json', 'show'}:
            return f"rg.head():\n{REGISTRY_BASE_OUT}"
        return ''

    if class_name == 'Axel':
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
            return AXEL_CONCAT_OUT
        if n in {'registry', 'head', 'tail', 'slice', 'map', 'select', 'apply_slot'}:
            return AXEL_BASE_OUT if ret.startswith('Axel') else REGISTRY_BASE_OUT
        if n in {'to_kindle', 'as_kindle', 'kindelize'}:
            return ARCHIVE_BASE_OUT
        if n in {'from_kindle'}:
            return AXEL_BASE_OUT
        if n in {'to_string', 'to_json', 'show'}:
            return AXEL_BASE_OUT
        return ''

    if ret in {'bool'}:
        return 'true'
    if ret in {'std::int64_t', 'std::size_t', 'int'}:
        return '3'
    if ret in {'double', 'float', 'Float'}:
        return '20.0'
    if ret in {'ThreadValue'}:
        if n in {'all', 'any', 'defined'}:
            return 'true'
        return '10.0'
    if ret in {'std::string', 'std::string_view'}:
        return 'demo'
    if ret in {'Catalog&'}:
        return labeled_pair('ct.head()', CATALOG_BASE_OUT, CATALOG_BASE_OUT)
    if ret in {'Registry&'}:
        return labeled_pair('rg.head()', REGISTRY_BASE_OUT, REGISTRY_BASE_OUT)
    if ret in {'Slot&'}:
        return SLOT_BASE_OUT
    if ret in {'Stream&'}:
        return STREAM_SERIES_OUT
    if ret in {'Thread&'}:
        return THREAD_SERIES_OUT
    if ret in {'Axel&'}:
        return AXEL_BASE_OUT
    if ret in {'Atlas&'}:
        return ATLAS_BASE_OUT
    if ret in {'Archive&'}:
        return ARCHIVE_BASE_OUT
    if ret in {'DataFrame&'}:
        return DF_BASE_OUT

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


def df_time_naive() -> str:
    return """auto df = pluss::table::DataFrame({
    {\"ts\", pluss::table::Thread(pluss::make_timeline({
        \"2024-01-01 09:10:00\",\n        \"2024-01-01 09:40:00\",\n        \"2024-01-01 10:05:00\",\n    }))},
    {\"load\", pluss::table::Thread(pluss::make_series({10.0, 20.0, 15.0}))},
});"""


def df_temporal_mixed() -> str:
    return """auto df = pluss::table::DataFrame({
    {\"start\", pluss::table::Thread(pluss::make_timeline({
        \"2020-01-01T00:00:00Z\",\n        \"2020-01-02T08:00:00Z\",\n    }))},
    {\"end\", pluss::table::Thread(pluss::make_timeindex({
        \"2020-01-01T01:30:00Z\",\n        \"2020-01-02T09:00:00Z\",\n    }))},
    {\"load\", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
    {\"city\", pluss::table::Thread(pluss::make_note({\"Accra\", \"Tema\"}))},
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


def df_pair_aligned() -> str:
    return """auto left = pluss::table::DataFrame({
    {\"id\", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {\"sales\", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {\"id\", pluss::table::Thread(pluss::make_series({2, 2, 4}))},
    {\"sales\", pluss::table::Thread(pluss::make_series({110.0, 110.0, 100.0}))},
});"""


def df_group() -> str:
    return """auto df = pluss::table::DataFrame({
    {\"city\", pluss::table::Thread(pluss::make_note({\"Accra\", \"Accra\", \"Tema\", \"Tema\"}))},
    {\"product\", pluss::table::Thread(pluss::make_note({\"A\", \"B\", \"A\", \"B\"}))},
    {\"sales\", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0, 150.0}))},
});"""


def df_named_metrics() -> str:
    return """auto df = pluss::table::DataFrame({
    {\"midterm_1\", pluss::table::Thread(pluss::make_series({70.0, 80.0, 90.0}))},
    {\"midterm_2\", pluss::table::Thread(pluss::make_series({75.0, 85.0, 95.0}))},
    {\"final_points\", pluss::table::Thread(pluss::make_series({88.0, 91.0, 94.0}))},
    {\"city\", pluss::table::Thread(pluss::make_note({\"Accra\", \"Tema\", \"Kumasi\"}))},
});"""


def df_small_numeric() -> str:
    return """auto df = pluss::table::DataFrame({
    {\"a\", pluss::table::Thread(pluss::make_series({1.0, 2.0, 3.0}))},
    {\"b\", pluss::table::Thread(pluss::make_series({2.0, 4.0, 5.0}))},
});"""


def df_pow_pair() -> str:
    return """auto left = pluss::table::DataFrame({
    {\"a\", pluss::table::Thread(pluss::make_series({2.0, 3.0, 4.0}))},
    {\"b\", pluss::table::Thread(pluss::make_series({3.0, 2.0, 5.0}))},
});
auto right = pluss::table::DataFrame({
    {\"a\", pluss::table::Thread(pluss::make_series({3.0, 2.0, 1.0}))},
    {\"b\", pluss::table::Thread(pluss::make_series({2.0, 3.0, 2.0}))},
});"""


def df_replace_numeric() -> str:
    return """auto df = pluss::table::DataFrame({
    {\"midterm_1\", pluss::table::Thread(pluss::make_series({0.0, 12.0, 0.0}))},
    {\"midterm_2\", pluss::table::Thread(pluss::make_series({8.0, 0.0, 15.0}))},
    {\"city\", pluss::table::Thread(pluss::make_note({\"Accra\", \"Tema\", \"Tema\"}))},
});"""


def df_unique_subset() -> str:
    return """auto df = pluss::table::DataFrame({
    {\"campus\", pluss::table::Thread(pluss::make_note({\"North\", \"North\", \"West\"}))},
    {\"student\", pluss::table::Thread(pluss::make_note({\"Ada\", \"Kojo\", \"Esi\"}))},
    {\"score\", pluss::table::Thread(pluss::make_series({88.0, 91.0, 84.0}))},
});"""


def df_duplicate_rows() -> str:
    return """auto df = pluss::table::DataFrame({
    {\"campus\", pluss::table::Thread(pluss::make_note({\"North\", \"North\", \"West\", \"North\"}))},
    {\"student\", pluss::table::Thread(pluss::make_note({\"Ada\", \"Ada\", \"Esi\", \"Kojo\"}))},
    {\"score\", pluss::table::Thread(pluss::make_series({88.0, 88.0, 84.0, 91.0}))},
});"""


def df_small_bool() -> str:
    return """auto df = pluss::table::DataFrame({
    {\"passed_math\", pluss::table::Thread(pluss::make_series({true, true, false}, pluss::ScalarType::Bool))},
    {\"passed_lab\", pluss::table::Thread(pluss::make_series({true, false, false}, pluss::ScalarType::Bool))},
});"""


def df_small_matmul_pair() -> str:
    return """auto left = pluss::table::DataFrame({
    {\"a\", pluss::table::Thread(pluss::make_series({1.0, 2.0}))},
    {\"b\", pluss::table::Thread(pluss::make_series({3.0, 4.0}))},
});
auto right = pluss::table::DataFrame({
    {\"c\", pluss::table::Thread(pluss::make_series({5.0, 6.0}))},
    {\"d\", pluss::table::Thread(pluss::make_series({7.0, 8.0}))},
});"""


def df_case_levels_base() -> str:
    return """auto grades = pluss::table::DataFrame({
    {\"student\", pluss::table::Thread(pluss::make_note({\"Ada\", \"Ada\", \"Kojo\", \"Kojo\"}))},
    {\"term\", pluss::table::Thread(pluss::make_note({\"mid\", \"final\", \"mid\", \"final\"}))},
    {\"score\", pluss::table::Thread(pluss::make_series({88.0, 91.0, 84.0, 89.0}))},
});
auto indexed = grades.label().set({\"student\", \"term\"}, true, \"|\");"""


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


def axel_base() -> str:
    return """auto r0 = pluss::bundle::Registry({
    {\"ids\", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {\"ids\", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto axel = pluss::bundle::Axel({r0, r1});"""


def feature_ref_base() -> str:
    return df_base() + '\n' + 'auto ref = pluss::table::ColumnRef("sales", df.features("sales"));'


def sql_accessor_base() -> str:
    return df_base() + '\n' + 'auto sql = df.sql();'


def sql_prepared_base() -> str:
    return df_base() + '\n' + 'auto stmt = df.prep_sql("SELECT id, sales FROM self WHERE sales > {min_sales}");'


def plot_accessor_base() -> str:
    return df_base() + '\n' + 'auto plot = df.plot();'


# --- Generic per-method example stubs for DataFrame accessor classes -------
#
# NOTE: unlike the hand-tuned example_for() branches above (and unlike the
# hand-authored .md pages under docs/sphinx/api/dataframe/*.md), the
# per-method pages these stubs feed are NOT individually verified against
# render_sim.py. They exist so the accessor's navigation surface (landing
# page + per-method sidebar, wired via CLASS_CONFIG/PER_METHOD_CLASSES) has
# *something* on every method page instead of a generator crash, exactly
# the same bootstrap state DataFrame/Thread method pages were in before each
# was replaced, batch by batch, with real hand-authored prose marked
# `<!-- pluss-hand-authored -->`. Treat every page under api/dataframe-*/ as
# an unverified stub until it carries that marker.

def note_accessor_base() -> str:
    return """auto df = pluss::table::DataFrame({
    {\"city\", pluss::table::Thread(pluss::make_note({\"  Accra\", \"TEMA \", \"Kumasi\"}))},
    {\"product\", pluss::table::Thread(pluss::make_note({\"Laptop\", \"desk\", \"Chair\"}))},
    {\"sales\", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto notes = df.nt();"""


def timeline_accessor_base() -> str:
    return """auto df = pluss::table::DataFrame({
    {\"ts\", pluss::table::Thread(pluss::make_timeline({
        \"2024-01-01T09:10:00Z\",\n        \"2024-01-01T09:40:00Z\",\n        \"2024-01-01T10:05:00Z\",\n    }))},
    {\"load\", pluss::table::Thread(pluss::make_series({10.0, 20.0, 15.0}))},
});
auto timelines = df.tl();"""


def label_accessor_base() -> str:
    return """auto df = pluss::table::DataFrame({
    {\"city\", pluss::table::Thread(pluss::make_note({\"Accra\", \"Tema\", \"Kumasi\"}))},
    {\"sales\", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
}, pluss::make_note({\"r1\", \"r2\", \"r3\"}));
auto labels = df.label();"""


def feataxis_accessor_base() -> str:
    return """auto df = pluss::table::DataFrame({
    {\"sales_units\", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {\"sales_price\", pluss::table::Thread(pluss::make_series({1.5, 2.0, 1.8}))},
    {\"city\", pluss::table::Thread(pluss::make_note({\"Accra\", \"Tema\", \"Kumasi\"}))},
});
auto feataxis = df.feataxis();"""


def plot3d_accessor_base() -> str:
    return """auto df = pluss::table::DataFrame({
    {\"x\", pluss::table::Thread(pluss::make_series({1.0, 2.0, 3.0}))},
    {\"y\", pluss::table::Thread(pluss::make_series({4.0, 5.0, 6.0}))},
    {\"z\", pluss::table::Thread(pluss::make_series({7.0, 8.0, 9.0}))},
});
auto plot3d = df.plot3d();"""


def ts_accessor_base() -> str:
    return """auto df = pluss::table::DataFrame({
    {\"ts\", pluss::table::Thread(pluss::make_timeline({
        \"2024-01-01T09:10:00Z\",\n        \"2024-01-01T09:40:00Z\",\n        \"2024-01-01T10:05:00Z\",\n    }))},
    {\"load\", pluss::table::Thread(pluss::make_series({10.0, 20.0, 15.0}))},
});
auto ts = df.ts();"""


def ts_window_accessor_base() -> str:
    return ts_accessor_base() + '\n' + 'auto window = ts.window("2D");'


def ts_resampler_accessor_base() -> str:
    return ts_accessor_base() + '\n' + 'auto resampler = ts.resample("ts", "1D");'


def thread_plot_accessor_base() -> str:
    return """auto revenue = pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0, 150.0}));
auto plot = revenue.plot();"""


def atlas_label_accessor_base() -> str:
    return """auto q1 = pluss::table::DataFrame({\"sales\", pluss::table::Thread(pluss::make_series({100.0, 120.0}))});
auto q2 = pluss::table::DataFrame({\"sales\", pluss::table::Thread(pluss::make_series({110.0, 90.0}))});
auto atlas = pluss::table::Atlas({q1, q2}, pluss::table::Thread(pluss::make_note({\"2024Q1\", \"2024Q2\"})));
auto labels = atlas.label();"""


def atlas_timeline_accessor_base() -> str:
    return """auto q1 = pluss::table::DataFrame({
    {\"ts\", pluss::table::Thread(pluss::make_timeline({\"2024-01-01T09:10:00Z\", \"2024-01-01T09:40:00Z\"}))},
});
auto q2 = pluss::table::DataFrame({
    {\"ts\", pluss::table::Thread(pluss::make_timeline({\"2024-01-02T09:10:00Z\", \"2024-01-02T09:40:00Z\"}))},
});
auto atlas = pluss::table::Atlas({q1, q2});
auto timelines = atlas.tl();"""


def atlas_ts_accessor_base() -> str:
    return """auto q1 = pluss::table::DataFrame({
    {\"ts\", pluss::table::Thread(pluss::make_timeline({
        \"2024-01-01T09:10:00Z\",\n        \"2024-01-01T09:40:00Z\",\n        \"2024-01-01T10:05:00Z\",\n    }))},
    {\"load\", pluss::table::Thread(pluss::make_series({10.0, 20.0, 15.0}))},
});
auto q2 = pluss::table::DataFrame({
    {\"ts\", pluss::table::Thread(pluss::make_timeline({
        \"2024-01-02T09:10:00Z\",\n        \"2024-01-02T09:40:00Z\",\n        \"2024-01-02T10:05:00Z\",\n    }))},
    {\"load\", pluss::table::Thread(pluss::make_series({12.0, 18.0, 14.0}))},
});
auto atlas = pluss::table::Atlas({q1, q2});
auto ts = atlas.ts();"""


def atlas_ts_resampler_accessor_base() -> str:
    return atlas_ts_accessor_base() + '\n' + 'auto resampler = ts.resample("ts", "1D");'


def atlas_plot_accessor_base() -> str:
    return """auto q1 = pluss::table::DataFrame({
    {\"x\", pluss::table::Thread(pluss::make_series({1.0, 2.0, 3.0}))},
    {\"y\", pluss::table::Thread(pluss::make_series({4.0, 5.0, 6.0}))},
});
auto q2 = pluss::table::DataFrame({
    {\"x\", pluss::table::Thread(pluss::make_series({1.0, 2.0, 3.0}))},
    {\"y\", pluss::table::Thread(pluss::make_series({7.0, 8.0, 9.0}))},
});
auto atlas = pluss::table::Atlas({q1, q2});
auto plot = atlas.plot();"""


def slot_cast_accessor_base() -> str:
    return slot_base() + '\n' + 'auto cast = slot.cast();'


def slot_note_accessor_base() -> str:
    return """auto slot = pluss::bundle::Slot({pluss::make_note(\"Accra\"), pluss::make_note(\"Tema\")});
auto notes = slot.nt();"""


def slot_timeline_accessor_base() -> str:
    return """auto slot = pluss::bundle::Slot({pluss::make_timeline(\"2024-01-01T09:10:00Z\"), pluss::make_timeline(\"2024-01-02T09:40:00Z\")});
auto timelines = slot.tl();"""


def registry_field_accessor_base() -> str:
    return registry_base() + '\n' + 'auto fields = rg.fields({"ids"});'


def registry_fieldaxis_accessor_base() -> str:
    return registry_base() + '\n' + 'auto fieldaxis = rg.fieldaxis();'


def registry_cast_accessor_base() -> str:
    return registry_base() + '\n' + 'auto cast = rg.fields({"ids"}).cast();'


def registry_note_accessor_base() -> str:
    return registry_base() + '\n' + 'auto notes = rg.fields({"city"}).nt();'


def registry_timeline_accessor_base() -> str:
    return """auto rg = pluss::bundle::Registry({
    {\"ids\", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
    {\"ts\", pluss::bundle::Slot({pluss::make_timeline(\"2024-01-01T09:10:00Z\"), pluss::make_timeline(\"2024-01-02T09:40:00Z\")})},
});
auto timelines = rg.fields({\"ts\"}).tl();"""


def axel_fieldaxis_accessor_base() -> str:
    return axel_base() + '\n' + 'auto fieldaxis = axel.fieldaxis();'


def axel_cast_accessor_base() -> str:
    return axel_base() + '\n' + 'auto cast = axel.fields({"ids"}).cast();'


def axel_note_accessor_base() -> str:
    return """auto r0 = pluss::bundle::Registry({
    {\"city\", pluss::bundle::Slot({pluss::make_note(\"Accra\"), pluss::make_note(\"Tema\")})},
});
auto r1 = pluss::bundle::Registry({
    {\"city\", pluss::bundle::Slot({pluss::make_note(\"Kumasi\"), pluss::make_note(\"Takoradi\")})},
});
auto axel = pluss::bundle::Axel({r0, r1});
auto notes = axel.fields({\"city\"}).nt();"""


def axel_timeline_accessor_base() -> str:
    return """auto r0 = pluss::bundle::Registry({
    {\"ts\", pluss::bundle::Slot({pluss::make_timeline(\"2024-01-01T09:10:00Z\"), pluss::make_timeline(\"2024-01-01T09:40:00Z\")})},
});
auto r1 = pluss::bundle::Registry({
    {\"ts\", pluss::bundle::Slot({pluss::make_timeline(\"2024-01-02T09:10:00Z\"), pluss::make_timeline(\"2024-01-02T09:40:00Z\")})},
});
auto axel = pluss::bundle::Axel({r0, r1});
auto timelines = axel.fields({\"ts\"}).tl();"""


ACCESSOR_STUB_BASE: dict[str, Callable[[], str]] = {
    'DataFrameNoteAccessor': note_accessor_base,
    'DataFrameTimelineAccessor': timeline_accessor_base,
    'DataFrameLabelAccessor': label_accessor_base,
    'DataFrameFeatureAxisAccessor': feataxis_accessor_base,
    'DataFramePlot3DAccessor': plot3d_accessor_base,
    'DataFrameTsAccessor': ts_accessor_base,
    'DataFrameTsWindowAccessor': ts_window_accessor_base,
    'DataFrameTsResampler': ts_resampler_accessor_base,
    'ThreadPlotAccessor': thread_plot_accessor_base,
    'AtlasLabelAccessor': atlas_label_accessor_base,
    'AtlasTimelineAccessor': atlas_timeline_accessor_base,
    'AtlasTsAccessor': atlas_ts_accessor_base,
    'AtlasTsResampler': atlas_ts_resampler_accessor_base,
    'AtlasPlotAccessor': atlas_plot_accessor_base,
    'SlotCastAccessor': slot_cast_accessor_base,
    'SlotNoteAccessor': slot_note_accessor_base,
    'SlotTimelineAccessor': slot_timeline_accessor_base,
    'RegistryFieldAxisAccessor': registry_fieldaxis_accessor_base,
    'RegistryCastAccessor': registry_cast_accessor_base,
    'RegistryNoteAccessor': registry_note_accessor_base,
    'RegistryTimelineAccessor': registry_timeline_accessor_base,
    'AxelFieldAxisAccessor': axel_fieldaxis_accessor_base,
    'AxelCastAccessor': axel_cast_accessor_base,
    'AxelNoteAccessor': axel_note_accessor_base,
    'AxelTimelineAccessor': axel_timeline_accessor_base,
}

ACCESSOR_STUB_VAR: dict[str, str] = {
    'DataFrameNoteAccessor': 'notes',
    'DataFrameTimelineAccessor': 'timelines',
    'DataFrameLabelAccessor': 'labels',
    'DataFrameFeatureAxisAccessor': 'feataxis',
    'DataFramePlot3DAccessor': 'plot3d',
    'DataFrameTsAccessor': 'ts',
    'DataFrameTsWindowAccessor': 'window',
    'DataFrameTsResampler': 'resampler',
    'ThreadPlotAccessor': 'plot',
    'AtlasLabelAccessor': 'labels',
    'AtlasTimelineAccessor': 'timelines',
    'AtlasTsAccessor': 'ts',
    'AtlasTsResampler': 'resampler',
    'AtlasPlotAccessor': 'plot',
    'SlotCastAccessor': 'cast',
    'SlotNoteAccessor': 'notes',
    'SlotTimelineAccessor': 'timelines',
    'RegistryFieldAxisAccessor': 'fieldaxis',
    'RegistryCastAccessor': 'cast',
    'RegistryNoteAccessor': 'notes',
    'RegistryTimelineAccessor': 'timelines',
    'AxelFieldAxisAccessor': 'fieldaxis',
    'AxelCastAccessor': 'cast',
    'AxelNoteAccessor': 'notes',
    'AxelTimelineAccessor': 'timelines',
}


def placeholder_arg_for_param(param_name: str, param_type: str) -> str:
    """Best-effort literal for one parameter, used only by the generic
    accessor-stub fallback in example_for(). Not meant to be exhaustively
    correct for every overload -- just enough to render a plausible call
    until the method gets a real, hand-verified example."""
    t = param_type.strip()
    p = param_name.lower()
    if t == 'bool':
        return 'true'
    if 'string_view' in t and ('vector' in t or 'RangeView' in t or 'initializer_list' in t):
        return '{"units", "price"}'
    if 'string_view' in t:
        if 'duration' in p or 'freq' in p:
            return '"2D"'
        if 'unit' in p:
            return '"D"'
        if 'pattern' in p or 'needle' in p:
            return '"units"'
        if 'name' in p or 'feature' in p:
            return '"units"'
        return '"units"'
    if 'int64_t' in t or t in {'int', 'std::size_t', 'size_t', 'std::int32_t'}:
        return '1'
    if t in {'double', 'float'}:
        return '0.0'
    if 'TimeUnit' in t:
        return 'pluss::table::TimeUnit::Nanos'
    if 'WindowStatistic' in t:
        return 'pluss::table::DataFrame::WindowStatistic::Mean'
    if 'WindowClosure' in t:
        return 'pluss::table::DataFrame::WindowClosure::Right'
    if 'Thread' in t and '&' in t:
        return 'df.features("load")'
    return '{}'


def accessor_stub_example(class_name: str, method: Method) -> str:
    base = ACCESSOR_STUB_BASE[class_name]()
    var = ACCESSOR_STUB_VAR[class_name]
    args = ', '.join(placeholder_arg_for_param(name, typ) for name, typ in method.params)
    return base + f'\n\nauto out = {var}.{method.name}({args});\nstd::cout << out;'


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


def thread_fallback_args(method: Method) -> tuple[str, str]:
    args: list[str] = []
    prelude: list[str] = []
    for name, typ in method.params:
        if name in {'skipnull', 'ascending', 'inclusive', 'right', 'include_lowest', 'dropnull', 'unbiased'} or typ == 'bool':
            args.append('true')
        elif name == 'start':
            args.append('0')
        elif name == 'stop':
            args.append('2')
        elif name == 'step':
            args.append('1')
        elif name in {'limit', 'window', 'periods'} or typ in {'std::int64_t', 'std::size_t', 'int'}:
            args.append('2')
        elif name in {'min_value', 'max_value', 'a', 'b', 'lo', 'hi'} or typ in {'double', 'float', 'Float'}:
            if name in {'min_value', 'a', 'lo'}:
                args.append('0.0')
            elif name in {'max_value', 'b', 'hi'}:
                args.append('25.0')
            else:
                args.append('10.0')
        elif 'std::optional<std::int64_t>' in typ:
            args.append('std::nullopt')
        elif 'std::string_view' in typ or 'std::string' in typ or 'const char*' in typ:
            if name in {'needle', 'pattern'}:
                args.append('"Acc"')
            elif name == 'chars':
                args.append('"#"')
            else:
                args.append('"demo"')
        elif 'pluss::ScalarType' in typ:
            args.append('pluss::ScalarType::Float64')
        elif 'ThreadCastTarget' in typ:
            args.append('pluss::table::ThreadCastTarget::Series')
        elif 'ThreadBinaryMathOp' in typ:
            args.append('pluss::table::ThreadBinaryMathOp::Add')
        elif 'ThreadBinaryCompareOp' in typ:
            args.append('pluss::table::ThreadBinaryCompareOp::Gt')
        elif 'ThreadCorrMethod' in typ:
            args.append('pluss::table::ThreadCorrMethod::Pearson')
        elif 'ThreadWindowOp' in typ:
            args.append('pluss::table::ThreadWindowOp::RollingMean')
        elif 'const Thread&' in typ:
            prelude.append('auto other = pluss::table::Thread(pluss::make_series({15.0, 10.0, 35.0}));')
            args.append('other')
        elif 'std::vector<std::int64_t>' in typ or 'RangeView::Int' in typ or 'std::span<const std::int64_t>' in typ:
            prelude.append('std::vector<std::int64_t> picks{0, 2};')
            args.append('picks')
        elif 'RangeView<double>' in typ or 'std::vector<double>' in typ:
            prelude.append('std::vector<double> bins{0.0, 20.0, 30.0, 40.0};')
            args.append('bins')
        elif 'RangeView<std::int64_t>' in typ or 'std::vector<std::int64_t>' in typ:
            prelude.append('std::vector<std::int64_t> labels{0, 1, 2};')
            args.append('labels')
        else:
            args.append('{}')
    return '\n'.join(prelude), ', '.join(args)


def example_for(class_name: str, method: Method) -> str:
    n = method.name
    sig = method.signature
    # DataFrame
    if class_name == 'DataFrame':
        if n == 'nt':
            return df_base() + '\n\nauto out = df.features({"city"}).nt();'
        if n == 'tl':
            return df_time() + '\n\nauto out = df.features({"ts"}).tl();'
        if n.startswith('load'):
            if n == 'load':
                if 'ReadCsvOptions' in sig:
                    return (
                        'pluss::loader::ReadCsvOptions options;\n'
                        'options.ncases = 8;\n'
                        'options.case_col = "Agency Name";\n'
                        f'auto out = pluss::table::DataFrame::load("csv", "{GENERIC_CSV_PATH}", options);'
                    )
                if 'const pluss::loader::Config&' in sig:
                    return (
                        'pluss::loader::Config config{\n'
                        '    {"ncases", 8},\n'
                        '    {"features", {"Agency Name", "State"}},\n'
                        '};\n'
                        f'auto out = pluss::table::DataFrame::load("csv", "{GENERIC_CSV_PATH}", config);'
                    )
                if 'initializer_list<pluss::loader::ConfigEntry>' in sig:
                    return (
                        f'auto out = pluss::table::DataFrame::load("csv", "{GENERIC_CSV_PATH}", {{\n'
                        '    {"ncases", 8},\n'
                        '    {"features", {"Agency Name", "State"}},\n'
                        '});'
                    )
                if 'RangeView::Of<std::string_view> usefeats' in sig:
                    return (
                        'std::array<std::string_view, 2> usefeats{"Agency Name", "State"};\n'
                        f'auto out = pluss::table::DataFrame::load("csv", "{GENERIC_CSV_PATH}", \',\', 8, usefeats, std::nullopt);'
                    )
                if 'std::vector<std::string> usefeats' in sig:
                    return (
                        f'auto out = pluss::table::DataFrame::load("csv", "{GENERIC_CSV_PATH}", \',\', 8, '
                        'std::vector<std::string>{"Agency Name", "State"}, std::nullopt);'
                    )
                return f'auto out = pluss::table::DataFrame::load("csv", "{GENERIC_CSV_PATH}");'
            if n == 'load_csv':
                if 'ReadCsvOptions' in sig:
                    return (
                        'pluss::loader::ReadCsvOptions options;\n'
                        'options.ncases = 8;\n'
                        'options.case_col = "Agency Name";\n'
                        f'auto out = pluss::table::DataFrame::load_csv("{GENERIC_CSV_PATH}", options);'
                    )
                if 'const pluss::loader::Config&' in sig:
                    return (
                        'pluss::loader::Config config{\n'
                        '    {"ncases", 8},\n'
                        '    {"features", {"Agency Name", "State"}},\n'
                        '};\n'
                        f'auto out = pluss::table::DataFrame::load_csv("{GENERIC_CSV_PATH}", config);'
                    )
                if 'initializer_list<pluss::loader::ConfigEntry>' in sig:
                    return (
                        f'auto out = pluss::table::DataFrame::load_csv("{GENERIC_CSV_PATH}", {{\n'
                        '    {"ncases", 8},\n'
                        '    {"features", {"Agency Name", "State"}},\n'
                        '});'
                    )
                if 'std::string_view header, std::string_view case_feature' in sig:
                    return f'auto out = pluss::table::DataFrame::load_csv("{GENERIC_CSV_PATH}", "infer", "Agency Name", "auto", 8);'
                if 'std::string_view header, std::size_t ncases' in sig:
                    return f'auto out = pluss::table::DataFrame::load_csv("{GENERIC_CSV_PATH}", "none", static_cast<std::size_t>(8));'
                if 'RangeView::Of<std::string_view> usefeats' in sig:
                    return (
                        'std::array<std::string_view, 2> usefeats{"Agency Name", "State"};\n'
                        f'auto out = pluss::table::DataFrame::load_csv("{GENERIC_CSV_PATH}", \',\', 8, usefeats, std::nullopt);'
                    )
                if 'std::vector<std::string> usefeats' in sig:
                    return (
                        f'auto out = pluss::table::DataFrame::load_csv("{GENERIC_CSV_PATH}", \',\', 8, '
                        'std::vector<std::string>{"Agency Name", "State"}, std::nullopt);'
                    )
                return f'auto out = pluss::table::DataFrame::load_csv("{GENERIC_CSV_PATH}");'
            if n == 'load_excel':
                if 'nullptr_t' in sig:
                    return f'auto out = pluss::table::DataFrame::load_excel("{GENERIC_XLSX_PATH}", nullptr, true);'
                if 'ReadExcelOptions' in sig:
                    return f'pluss::loader::ReadExcelOptions options;\nauto out = pluss::table::DataFrame::load_excel("{GENERIC_XLSX_PATH}", options);'
                if 'const pluss::loader::Config&' in sig:
                    return (
                        'pluss::loader::Config config{{"sheet_name", "Sheet 1"}};\n'
                        f'auto out = pluss::table::DataFrame::load_excel("{GENERIC_XLSX_PATH}", config);'
                    )
                if 'initializer_list<pluss::loader::ConfigEntry>' in sig:
                    return (
                        f'auto out = pluss::table::DataFrame::load_excel("{GENERIC_XLSX_PATH}", {{\n'
                        '    {"sheet_name", "Sheet 1"},\n'
                        '});'
                    )
                if 'std::initializer_list<std::string_view> sheet_names' in sig:
                    return f'auto out = pluss::table::DataFrame::load_excel("{GENERIC_XLSX_PATH}", {{"Sheet 1", "Sheet 2"}}, true);'
                if 'const std::vector<std::string>& sheet_names' in sig:
                    return (
                        'std::vector<std::string> sheets{"Sheet 1", "Sheet 2"};\n'
                        f'auto out = pluss::table::DataFrame::load_excel("{GENERIC_XLSX_PATH}", sheets, true);'
                    )
                if 'RangeView::Of<std::string_view> sheet_names' in sig:
                    return (
                        'std::array<std::string_view, 2> sheets{"Sheet 1", "Sheet 2"};\n'
                        f'auto out = pluss::table::DataFrame::load_excel("{GENERIC_XLSX_PATH}", sheets, true);'
                    )
                if 'std::initializer_list<std::int64_t> sheet_indices' in sig:
                    return f'auto out = pluss::table::DataFrame::load_excel("{GENERIC_XLSX_PATH}", {{0, 1}}, true);'
                if 'const std::vector<std::int64_t>& sheet_indices' in sig:
                    return (
                        'std::vector<std::int64_t> sheets{0, 1};\n'
                        f'auto out = pluss::table::DataFrame::load_excel("{GENERIC_XLSX_PATH}", sheets, true);'
                    )
                if 'RangeView::Int sheet_indices' in sig:
                    return (
                        'std::array<std::int64_t, 2> sheets{0, 1};\n'
                        f'auto out = pluss::table::DataFrame::load_excel("{GENERIC_XLSX_PATH}", sheets, true);'
                    )
                if 'sheet_name' in sig:
                    return f'auto out = pluss::table::DataFrame::load_excel("{GENERIC_XLSX_PATH}", "Sheet 1", true);'
                if 'sheet_index' in sig or 'sheet_index,' in sig:
                    return f'auto out = pluss::table::DataFrame::load_excel("{GENERIC_XLSX_PATH}", 0, true);'
                return f'auto out = pluss::table::DataFrame::load_excel("{GENERIC_XLSX_PATH}");'
            if 'Config' in sig or 'ConfigEntry' in sig:
                path = GENERIC_CSV_PATH
                fn = n
                if 'excel' in n:
                    path = GENERIC_XLSX_PATH
                elif 'json' in n:
                    path = GENERIC_JSON_PATH
                elif 'parquet' in n:
                    path = GENERIC_PARQUET_PATH
                elif 'txt' in n:
                    path = GENERIC_TXT_PATH
                if 'ConfigEntry' in sig:
                    return (
                        f'auto out = pluss::table::DataFrame::{fn}("{path}", {{\n'
                        '    {"ncases", 8},\n'
                        '});'
                    )
                return (
                    'pluss::loader::Config config{{"ncases", 8}};\n'
                    f'auto out = pluss::table::DataFrame::{fn}("{path}", config);'
                )
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
            if 'TabelizePolicy' in sig:
                return 'using namespace pluss::kindle::literals;\nauto catalog = pluss::kindle::Catalog({"sales"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}))});\npluss::kindle::TabelizePolicy policy;\nauto out = catalog.tabelize(policy);'
            return 'using namespace pluss::kindle::literals;\nauto catalog = pluss::kindle::Catalog({"sales"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}))});\nauto out = catalog.tabelize();'
        if n == 'kindelize':
            if 'KindelizePolicy' in sig:
                return (
                    df_base()
                    + '\n\n'
                    + 'pluss::kindle::KindelizePolicy policy;\n'
                    + 'auto out = df.kindelize(policy);'
                )
            return df_base() + '\n\nauto out = df.kindelize();'
        if n in {'nrows','ncols','nfeatures','values','feature_names'}:
            return df_base() + f'\n\nauto out = df.{n}();'
        if n == 'caselbl':
            if 'const Thread& labels' in sig:
                return df_base() + '\n\nauto labels = pluss::table::Thread(pluss::make_note({"cs0", "cs1", "cs2"}));\nauto out = df.caselbl(labels);'
            return df_base() + '\n\nauto out = df.label().values();'
        if n == 'caselbl_':
            return df_base() + '\n\nauto labels = pluss::table::Thread(pluss::make_note({"cs0", "cs1", "cs2"}));\ndf.caselbl_(labels);'
        if n == 'caselbls':
            return df_case_levels_base() + '\n\nauto out = indexed.label().names();'
        if n == 'caselbl':
            if 'std::string_view name' in sig:
                return df_case_levels_base() + '\n\nauto out = indexed.caselbl("student");'
            return df_case_levels_base() + '\n\nauto out = indexed.caselbl(1);'
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
        if n == 'select':
            return df_base() + '\n\nauto out = df.select({"id", "sales"});'
        if n in {'series_features','note_features','timeline_features','with_cases','with_cases_','cast_series','cast_note','cast_timeline','cast_timeindex'}:
            if n == 'with_cases':
                return df_base() + '\n\nauto cases = pluss::table::Thread(pluss::make_note({"cs0", "cs1", "cs2"}));\nauto out = df.with_cases(cases);'
            if n == 'with_cases_':
                return df_base() + '\n\nauto cases = pluss::table::Thread(pluss::make_note({"cs0", "cs1", "cs2"}));\ndf.with_cases_(cases);'
            if n == 'cast_series':
                return 'auto df = pluss::table::DataFrame({{"sales", pluss::table::Thread(pluss::make_note({"100.0", "120.0", "90.0"}))}});\n\nauto out = df.cast_series({"sales"}, pluss::codem::ScalarType::Float64);'
            if n == 'cast_timeline':
                return 'auto df = pluss::table::DataFrame({{"ts", pluss::table::Thread(pluss::make_note({"2024-01-01T09:10:00Z", "2024-01-01T09:40:00Z"}))}});\n\nauto out = df.cast_timeline({"ts"});'
            if n == 'timeline_features':
                return df_temporal_mixed() + '\n\nauto out = df.feataxis().timeline();'
            return df_base() + f'\n\nauto out = df.{n}();' if n.endswith('_features') else df_base() + f'\n\nauto out = df.{n}({{"sales"}});'
        if n in {'astype','cast'}:
            if 'feature_dtypes' in sig:
                return 'auto df = pluss::table::DataFrame({{"sales", pluss::table::Thread(pluss::make_note({"100.0", "120.0", "90.0"}))}});\n\nauto out = df.astype({{"sales", pluss::codem::ScalarType::Float64}});'
            if 'CastSpec' in sig:
                return 'auto df = pluss::table::DataFrame({{"sales", pluss::table::Thread(pluss::make_note({"100.0", "120.0", "90.0"}))}});\n\nauto out = df.cast({{.feature = "sales", .target = pluss::table::ThreadCastTarget::Series, .series_dtype = pluss::codem::ScalarType::Float64}});'
            if n == 'astype':
                return 'auto df = pluss::table::DataFrame({{"sales", pluss::table::Thread(pluss::make_note({"100.0", "120.0", "90.0"}))}});\n\nauto out = df.astype({"sales"}, pluss::codem::ScalarType::Float64);'
        if n in {'insert','drop','reorder','add_prefix','add_suffix','rename','rename_','rename_features','relabel_features','set_axis','set_axis','place'}:
            if n == 'insert':
                return df_base() + '\n\nauto out = df.insert(1, "profit", pluss::table::Thread(pluss::make_series({40.0, 35.0, 30.0})));'
            if n == 'place':
                if 'const Thread& feature' in sig:
                    return df_base() + '\n\nauto out = df.place("profit", pluss::table::Thread(pluss::make_series({40.0, 35.0, 30.0})), "sales", true);'
                return df_base() + '\n\nauto out = df.place("city", "id", false);'
            if n == 'drop':
                return df_base() + '\n\nauto out = df.drop({"city"});'
            if n == 'reorder':
                if 'initializer_list<ReorderSpec>' in sig:
                    return (
                        'auto df = pluss::table::DataFrame({\n'
                        '    {"col_1", pluss::make_series({1.0, 101.0})},\n'
                        '    {"col_2", pluss::make_series({2.0, 102.0})},\n'
                        '    {"col_3", pluss::make_series({3.0, 103.0})},\n'
                        '    {"col_4", pluss::make_series({4.0, 104.0})},\n'
                        '    {"col_5", pluss::make_series({5.0, 105.0})},\n'
                        '    {"col_6", pluss::make_series({6.0, 106.0})},\n'
                        '    {"col_7", pluss::make_series({7.0, 107.0})},\n'
                        '    {"col_8", pluss::make_series({8.0, 108.0})},\n'
                        '    {"col_9", pluss::make_series({9.0, 109.0})},\n'
                        '    {"col_10", pluss::make_series({10.0, 110.0})},\n'
                        '    {"col_15", pluss::make_series({15.0, 115.0})},\n'
                        '    {"col_16", pluss::make_series({16.0, 116.0})},\n'
                        '    {"col_17", pluss::make_series({17.0, 117.0})},\n'
                        '    {"col_20", pluss::make_series({20.0, 120.0})},\n'
                        '    {"col_21", pluss::make_series({21.0, 121.0})},\n'
                        '    {"col_22", pluss::make_series({22.0, 122.0})},\n'
                        '    {"col_23", pluss::make_series({23.0, 123.0})},\n'
                        '    {"col_24", pluss::make_series({24.0, 124.0})},\n'
                        '    {"col_25", pluss::make_series({25.0, 125.0})},\n'
                        '});\n\n'
                        'using Move = pluss::table::DataFrame::ReorderSpec;\n'
                        'auto out = df.reorder({\n'
                        '    Move::move({"col_25", "col_10"}).first(),\n'
                        '    Move::move("col_20").before("col_5"),\n'
                        '    Move::move({"col_15", "col_16", "col_17"}).after("col_8"),\n'
                        '    Move::move_regex("^col_2[1-4]$").last(),\n'
                        '});'
                    )
                if 'pluss::RangeView<ReorderSpec>' in sig:
                    return (
                        df_base()
                        + '\n\n'
                        + 'using Move = pluss::table::DataFrame::ReorderSpec;\n'
                        + 'std::vector<Move> plan{\n'
                        + '    Move::move({"city", "sales"}).first(),\n'
                        + '    Move::move("id").after("sales"),\n'
                        + '};\n'
                        + 'auto out = df.reorder(plan);'
                    )
                if 'ReorderSpec' in sig:
                    return (
                        df_base()
                        + '\n\n'
                        + 'auto spec = pluss::table::DataFrame::ReorderSpec::move("sales").after("city");\n'
                        + 'auto out = df.reorder(spec);'
                    )
                return df_base() + '\n\nauto out = df.reorder({"city"});'
            if n == 'add_prefix':
                return df_base() + '\n\nauto out = df.feataxis().prefix("feat_");'
            if n == 'add_suffix':
                return df_base() + '\n\nauto out = df.feataxis().suffix("_2024");'
            if n == 'rename':
                if 'old_name' in sig:
                    return df_base() + '\n\nauto out = df.rename("sales", "revenue");'
                if 'initializer_list<std::pair<std::string_view, std::string_view>>' in sig:
                    return df_base() + '\n\nauto out = df.rename({{"sales", "revenue"}, {"city", "location"}});'
                if 'RangeView::Of<std::pair<std::string_view, std::string_view>>' in sig:
                    return (
                        df_base()
                        + '\n\n'
                        + 'std::vector<std::pair<std::string_view, std::string_view>> renames{{"sales", "revenue"}, {"city", "location"}};\n'
                        + 'auto out = df.rename(renames);'
                    )
                if 'initializer_list<const char*>' in sig:
                    return df_base() + '\n\nauto out = df.rename({"id_new", "revenue", "location"});'
                return (
                    df_base()
                    + '\n\n'
                    + 'std::vector<std::string_view> names{"id_new", "revenue", "location"};\n'
                    + 'auto out = df.rename(names);'
                )
            if n == 'rename_':
                return df_base() + '\n\ndf.rename_("sales", "revenue");'
            if n == 'rename_features':
                return df_base() + '\n\nauto out = df.rename_features({{"sales", "revenue"}, {"city", "location"}});'
            if n == 'relabel_features' or n == 'set_axis' or n == 'set_axis':
                return df_base() + f'\n\nauto out = df.{n}({{"revenue", "location", "id_new"}});'
        if n == 'bucketize':
            if 'pluss::msharp::RangeView::Of<std::string_view> feature_names' in sig:
                return (
                    df_base()
                    + '\n\n'
                    + 'std::vector<double> bins{0.0, 95.0, 110.0, 130.0};\n'
                    + 'std::vector<std::int64_t> labels{1, 2, 3};\n'
                    + 'auto out = df.bucketize({"id", "sales"}, bins, labels, true, true);'
                )
            if 'std::string_view feature_name' in sig:
                return (
                    df_base()
                    + '\n\n'
                    + 'std::vector<double> bins{0.0, 95.0, 110.0, 130.0};\n'
                    + 'std::vector<std::int64_t> labels{1, 2, 3};\n'
                    + 'auto out = df.bucketize("sales", bins, labels, true, true);'
                )
            return (
                df_base()
                + '\n\n'
                + 'std::vector<double> bins{0.0, 2.0, 3.0, 4.0};\n'
                + 'std::vector<std::int64_t> labels{1, 2, 3};\n'
                + 'auto out = df.bucketize(bins, labels, true, true);'
            )
        if n == 'case_study':
            if 'std::string_view feature_name' in sig:
                return df_base() + '\n\nauto value = df.case_study(1, "city");'
            if 'std::int64_t start, std::int64_t stop' in sig:
                return df_base() + '\n\nauto out = df.case_study(0, 2);'
            return df_base() + '\n\nauto out = df.case_study(1);'
        if n == 'observe':
            if 'std::int64_t start, std::int64_t stop' in sig:
                return df_base() + '\n\nauto preview = df.observe(0, 2, false);\npreview.show();'
            return df_base() + '\n\nauto preview = df.observe(1, false);\npreview.show();'
        if n in {'arrange','label().sort','top','bottom','label().repeated','unique_cases','label().align'}:
            if n == 'arrange':
                if 'ArrangeSpec' in sig:
                    return df_base() + '\n\nauto out = df.arrange({{.feature = "sales", .ascending = false}});'
                return df_base() + '\n\nauto out = df.arrange({"sales"}, false, true);'
            if n == 'label().sort':
                return df_base() + '\n\nauto with_cases = df.with_cases(pluss::table::Thread(pluss::make_note({"cs2", "cs0", "cs1"})));\nauto out = with_cases.label().sort(true, true);'
            if n == 'top':
                return df_base() + '\n\nauto out = df.top("sales", 2);'
            if n == 'bottom':
                return df_base() + '\n\nauto out = df.bottom("sales", 2);'
            if n == 'label().repeated':
                return df_base() + '\n\nauto dup = df.with_cases(pluss::table::Thread(pluss::make_note({"cs0", "cs0", "cs2"})));\nauto out = dup.label().repeated();'
            if n == 'unique_cases':
                return df_unique_subset() + '\n\nauto out = df.drop_duplicates({"campus"});'
            if n == 'label().align':
                return df_base() + '\n\ndf = df.with_cases(pluss::table::Thread(pluss::make_note({"cs0", "cs1", "cs2"})));\nauto new_label = pluss::table::Thread(pluss::make_note({"cs1", "cs0", "cs2"}));\nauto out = df.label().align(new_label);'
        if n == 'match':
            return (
                'auto left = pluss::table::DataFrame(\n'
                '    {{"x", pluss::make_series({1.0, 2.0})},\n'
                '     {"y", pluss::make_series({10.0, 20.0})}},\n'
                '    pluss::table::Thread(pluss::make_note({"A", "C"})));\n'
                'auto right = pluss::table::DataFrame(\n'
                '    {{"y", pluss::make_series({100.0, 200.0})},\n'
                '     {"z", pluss::make_series({7.0, 8.0})}},\n'
                '    pluss::table::Thread(pluss::make_note({"B", "A"})));\n'
                'auto matched = left.match(right);'
            )
        if n == 'extend':
            return (
                'auto left = pluss::table::DataFrame(\n'
                '    {{"x", pluss::make_series({1.0, 2.0})},\n'
                '     {"y", pluss::make_series({3.0, 4.0})}},\n'
                '    pluss::table::Thread(pluss::make_note({"A", "C"})));\n'
                'auto right = pluss::table::DataFrame(\n'
                '    {{"y", pluss::make_series({5.0, 6.0})},\n'
                '     {"z", pluss::make_series({7.0, 8.0})}},\n'
                '    pluss::table::Thread(pluss::make_note({"B", "A"})));\n'
                'auto out = left.extend(right);'
            )
        if n == 'lengthen':
            return (
                'auto wide = pluss::table::DataFrame({\n'
                '    {"id", pluss::make_note({"r1", "r2"})},\n'
                '    {"a", pluss::make_series({1.0, 2.0})},\n'
                '    {"b", pluss::make_series({3.0, 4.0})},\n'
                '});\n'
                'auto out = wide.lengthen({"id"}, {"a", "b"});'
            )
        if n in {'join','update','conform','reindex_like','reindex_cases','stitch_on','combine'}:
            if n == 'conform' or n == 'reindex_like':
                return df_two() + f'\n\nauto out = left.{n}(right);'
            if n == 'reindex_cases':
                return df_base() + '\n\ndf = df.with_cases(pluss::table::Thread(pluss::make_note({"cs0", "cs1", "cs2"})));\nauto cases = pluss::table::Thread(pluss::make_note({"cs1", "cs0", "cs2"}));\nauto out = df.reindex_cases(cases);'
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
            if n == 'set_features' or n == 'set_features_':
                if 'pluss::msharp::RangeView::Of<std::string_view> names' in sig:
                    return df_base() + (
                        '\n\nauto out = df.set(\n'
                        '    {"profit", "cost"},\n'
                        '    {\n'
                        '        pluss::table::Thread(pluss::make_series({40.0, 35.0, 30.0})),\n'
                        '        pluss::table::Thread(pluss::make_series({60.0, 85.0, 60.0})),\n'
                        '    });'
                        if not n.endswith('_') else
                        '\n\ndf.set_(\n'
                        '    {"profit", "cost"},\n'
                        '    {\n'
                        '        pluss::table::Thread(pluss::make_series({40.0, 35.0, 30.0})),\n'
                        '        pluss::table::Thread(pluss::make_series({60.0, 85.0, 60.0})),\n'
                        '    });'
                    )
                return df_base() + (f'\n\nauto out = df.{n}("profit", pluss::table::Thread(pluss::make_series({{40.0, 35.0, 30.0}})));' if not n.endswith('_') else '\n\ndf.set_("profit", pluss::table::Thread(pluss::make_series({40.0, 35.0, 30.0})));')
            if n == 'set':
                if 'initializer_list' in sig:
                    return df_base() + '\n\ndf.set({"profit", "cost"}) = {pluss::table::Thread(pluss::make_series({40.0, 35.0, 30.0})), pluss::table::Thread(pluss::make_series({60.0, 85.0, 60.0}))};'
                return df_base() + '\n\ndf.set("profit") = pluss::table::Thread(pluss::make_series({40.0, 35.0, 30.0}));'
        if n == 'plot':
            if 'DataFramePlotOptions' in sig:
                return (
                    df_base()
                    + '\n\n'
                    + 'pluss::table::DataFramePlotOptions options;\n'
                    + 'options.default_x_feature = "id";\n'
                    + 'auto fig = df.plot(options).line("sales");'
                )
            return df_base() + '\n\nauto fig = df.plot().line("sales");'
        if n == 'plot3d':
            if 'DataFramePlotOptions' in sig:
                return (
                    'auto df = pluss::table::DataFrame({\n'
                    '    {"x", pluss::make_series({1.0, 2.0, 3.0})},\n'
                    '    {"y", pluss::make_series({4.0, 3.0, 2.0})},\n'
                    '    {"z", pluss::make_series({2.0, 5.0, 4.0})},\n'
                    '});\n\n'
                    'pluss::table::DataFramePlotOptions options;\n'
                    'options.width = 900.0;\n'
                    'auto fig = df.plot3d(options).scatter("x", "y", "z");'
                )
            return (
                'auto df = pluss::table::DataFrame({\n'
                '    {"x", pluss::make_series({1.0, 2.0, 3.0})},\n'
                '    {"y", pluss::make_series({4.0, 3.0, 2.0})},\n'
                '    {"z", pluss::make_series({2.0, 5.0, 4.0})},\n'
                '});\n\n'
                'auto fig = df.plot3d().scatter("x", "y", "z");'
            )
        if n == 'sql':
            if 'SqlOptions options' in sig:
                return (
                    df_base()
                    + '\n\n'
                    + 'pluss::table::DataFrame::SqlOptions options;\n'
                    + 'auto sql = df.sql(options);\n'
                    + 'auto out = sql.execute("SELECT id, sales FROM self WHERE sales > 95");'
                )
            return df_base() + '\n\nauto out = df.sql().execute("SELECT id, sales FROM self WHERE sales > 95");'
        if n == 'prep_sql':
            if 'SqlOptions options' in sig:
                return (
                    df_base()
                    + '\n\n'
                    + 'pluss::table::DataFrame::SqlOptions options;\n'
                    + 'auto stmt = df.prep_sql("SELECT id, sales FROM self WHERE sales > {min_sales}", options);'
                )
            return df_base() + '\n\nauto stmt = df.prep_sql("SELECT id, sales FROM self WHERE sales > {min_sales}");'
        if n == 'select_like':
            return df_named_metrics() + '\n\nauto out = df.feataxis().like("midterm");'
        if n == 'unstack':
            if 'std::string_view caselbl' in sig:
                return df_case_levels_base() + '\n\nauto out = indexed.unstack("term");'
            return (
                df_case_levels_base()
                + '\n\n'
                + 'pluss::table::DataFrame::UnstackOptions options;\n'
                + 'options.label = "term";\n'
                + 'options.drop = true;\n'
                + 'options.sort = true;\n'
                + 'options.separator = "_";\n'
                + 'auto out = indexed.unstack(options);'
            )
        if n == 'replace':
            if 'std::string_view from, std::string_view to' in sig or 'const char* from, const char* to' in sig:
                return df_replace_numeric() + '\n\nauto out = df.replace("Tema", "Tamale", {"city"});'
            return df_replace_numeric() + '\n\nauto out = df.replace(0.0, 5.0, {"midterm_1", "midterm_2"});'
        if n in {'put', 'put_'}:
            thread_mask = 'auto mask = df.features("id") == 2.0;'
            series_mask = 'auto mask = pluss::make_series({false, true, false}, pluss::codem::ScalarType::Bool);'
            if 'const pluss::Series& bool_mask' in sig and 'const char* value' in sig:
                return df_base() + f'\n\n{series_mask}\n' + ('auto out = df.put("city", mask, "Tamale");' if n == 'put' else 'df.put_("city", mask, "Tamale");')
            if 'const pluss::Series& bool_mask' in sig and 'const Thread& value' in sig:
                return df_base() + f'\n\n{series_mask}\nauto repl = pluss::table::Thread(pluss::make_series({{500.0, 999.0, 500.0}}));\n' + ('auto out = df.put("sales", mask, repl);' if n == 'put' else 'df.put_("sales", mask, repl);')
            if 'const pluss::Series& bool_mask' in sig and 'pluss::null_t value' in sig:
                return df_base() + f'\n\n{series_mask}\n' + ('auto out = df.put("sales", mask, pluss::null);' if n == 'put' else 'df.put_("sales", mask, pluss::null);')
            if 'const pluss::Series& bool_mask' in sig and 'std::string_view value' in sig:
                return df_base() + f'\n\n{series_mask}\n' + ('auto out = df.put("city", mask, std::string_view("Tamale"));' if n == 'put' else 'df.put_("city", mask, std::string_view("Tamale"));')
            if 'const pluss::Series& bool_mask' in sig:
                return df_base() + f'\n\n{series_mask}\n' + ('auto out = df.put("sales", mask, 999.0);' if n == 'put' else 'df.put_("sales", mask, 999.0);')
            if 'const Thread& bool_mask' in sig and 'const char* value' in sig:
                return df_base() + f'\n\n{thread_mask}\n' + ('auto out = df.put("city", mask, "Tamale");' if n == 'put' else 'df.put_("city", mask, "Tamale");')
            if 'const Thread& bool_mask' in sig and 'const Thread& value' in sig:
                return df_base() + f'\n\n{thread_mask}\nauto repl = pluss::table::Thread(pluss::make_series({{500.0, 999.0, 500.0}}));\n' + ('auto out = df.put("sales", mask, repl);' if n == 'put' else 'df.put_("sales", mask, repl);')
            if 'const Thread& bool_mask' in sig and 'pluss::null_t value' in sig:
                return df_base() + f'\n\n{thread_mask}\n' + ('auto out = df.put("sales", mask, pluss::null);' if n == 'put' else 'df.put_("sales", mask, pluss::null);')
            if 'const Thread& bool_mask' in sig and 'std::string_view value' in sig:
                return df_base() + f'\n\n{thread_mask}\n' + ('auto out = df.put("city", mask, std::string_view("Tamale"));' if n == 'put' else 'df.put_("city", mask, std::string_view("Tamale"));')
            return df_base() + f'\n\n{thread_mask}\n' + ('auto out = df.put("sales", mask, 999.0);' if n == 'put' else 'df.put_("sales", mask, 999.0);')
        if n in {'head','tail','sample','slice','take','where','filter','query','assign','transform'}:
            if n == 'head':
                return df_base() + '\n\nauto out = df.head(2, false);'
            if n == 'tail':
                return df_base() + '\n\nauto out = df.tail(2, false);'
            if n == 'sample':
                if ' frac' in sig and 'std::uint64_t random_state' in sig:
                    return df_base() + '\n\nauto out = df.sample(0.67, 7, false, 0);'
                if ' frac' in sig:
                    return df_base() + '\n\nauto out = df.sample(0.67, false, 0);'
                if 'std::uint64_t random_state' in sig:
                    return df_base() + '\n\nauto rows = df.sample(2, 7, false, 0);\nauto cols = df.sample(2, 7, false, 1);'
                if 'bool replace, int axis' in sig:
                    return df_base() + '\n\nauto rows = df.sample(2, false, 0);\nauto cols = df.sample(2, false, 1);'
                return df_base() + '\n\nauto out = df.sample(2);'
            if n == 'slice':
                return df_base() + '\n\nauto out = df.slice(0, 2);'
            if n == 'take':
                return df_base() + '\n\nstd::vector<std::int64_t> picks{0, 2};\nauto out = df.take(picks);'
            if n == 'where' or n == 'filter':
                if 'const Thread& bool_mask' in sig:
                    return df_base() + '\n\nauto mask = df.features("sales") >= 95.0;\nauto out = df.' + n + '(mask);'
                return df_base() + '\n\nauto mask = pluss::make_series({true, false, true}, pluss::codem::ScalarType::Bool);\nauto out = df.' + n + '(mask);'
            if n == 'query':
                return df_base() + '\n\nauto out = df.query("sales > 95");'
            if n == 'assign':
                if 'pair<std::string, Thread>' in sig:
                    return df_base() + '\n\nauto out = df.assign({{"profit", pluss::table::Thread(pluss::make_series({40.0, 35.0, 30.0}))}});'
                return df_base() + '\n\nauto out = df.assign({{.feature = "profit", .expr = [](const pluss::table::DataFrame& frame) { return pluss::table::Thread(frame.features("sales").as_series().sub(60)); }}});'
            if n == 'transform':
                if 'const std::function<Thread(const DataFrame&)>&' in sig:
                    return df_base() + '\n\nauto out = df.transform("sales_plus_10", [](const pluss::table::DataFrame& frame) { return pluss::table::Thread(frame.features("sales").as_series().add(10.0)); });'
                if 'std::string_view feature' in sig:
                    return df_base() + '\n\nauto out = df.transform("sales", [](const pluss::table::Thread& thread) { return thread.clip(0.0, 110.0); });'
                if 'TransformSpec' in sig:
                    return df_base() + '\n\nauto out = df.transform({{.feature = "sales", .expr = [](const pluss::table::Thread& thread) { return thread.clip(0.0, 110.0); }}});'
                return df_base() + '\n\nauto out = df.transform([](const pluss::table::Thread& thread) { return thread.kind() == pluss::table::ThreadKind::Series ? thread.clip(0.0, 110.0) : thread; });'
        if n == 'shift':
            if 'pluss::msharp::RangeView::Of<std::string_view> features' in sig:
                return df_base() + '\n\nauto out = df.shift({"sales"}, 2);'
            return df_base() + '\n\nauto out = df.shift(2);'
        if n == 'shuffle':
            return df_base() + '\n\nauto out = df.shuffle(7);'
        if n == 'reshuffle':
            return df_base() + '\n\nauto rows = df.reshuffle(0, 7);\nauto cols = df.reshuffle(1, 7);'
        if n in {'aggregate','group','pivot','pivot_table','count_by','mean_by'}:
            if n == 'aggregate':
                return df_group() + '\n\nauto out = df.aggregate({{"sales_mean", "sales", pluss::table::DataFrame::SummaryOp::Mean}}, false);'
            if n == 'group':
                return (
                    df_group()
                    + '\n\n'
                    + 'auto out = df.group({.dropnull = true})\n'
                    + '             .by({"city"})\n'
                    + '             .agg({{"sales_sum", "sales", pluss::table::DataFrame::GroupOp::Sum}});'
                )
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
            if 'pluss::msharp::RangeView::Of<std::string_view> features, CorrMethod method' in sig:
                return df_base() + '\n\nauto out = df.corr({"id", "sales"}, pluss::table::DataFrame::CorrMethod::KendallTau, false);'
            if 'pluss::msharp::RangeView::Of<std::string_view> features' in sig:
                return df_base() + '\n\nauto out = df.corr({"id", "sales"}, false);'
            if 'CorrMethod method' in sig:
                return df_base() + '\n\nauto out = df.corr(pluss::table::DataFrame::CorrMethod::Spearman, false);'
            return df_base() + '\n\nauto out = df.corr(false);'
        if n == 'corr_with':
            if 'RangeView::Of<DataFrame> others' in sig:
                return (
                    df_small_numeric()
                    + '\n\n'
                    + 'auto other0 = pluss::table::DataFrame({\n'
                    + '    {"a", pluss::table::Thread(pluss::make_series({1.0, 2.0, 3.0}))},\n'
                    + '    {"b", pluss::table::Thread(pluss::make_series({2.0, 4.0, 5.0}))},\n'
                    + '});\n'
                    + 'auto other1 = pluss::table::DataFrame({\n'
                    + '    {"a", pluss::table::Thread(pluss::make_series({3.0, 2.0, 1.0}))},\n'
                    + '    {"b", pluss::table::Thread(pluss::make_series({5.0, 4.0, 2.0}))},\n'
                    + '});\n'
                    + 'std::vector<pluss::table::DataFrame> others{other0, other1};\n'
                    + 'auto out = df.corr_with(others, pluss::table::DataFrame::CorrMethod::Pearson, 1);'
                )
            return (
                df_small_numeric()
                + '\n\n'
                + 'auto other = pluss::table::DataFrame({\n'
                + '    {"a", pluss::table::Thread(pluss::make_series({1.0, 2.0, 3.0}))},\n'
                + '    {"b", pluss::table::Thread(pluss::make_series({2.0, 4.0, 5.0}))},\n'
                + '});\n'
                + 'auto out = df.corr_with(other, pluss::table::DataFrame::CorrMethod::Pearson, 1);'
            )
        if n == 'matmul':
            return df_small_matmul_pair() + '\n\nauto out = left.matmul(right);'
        if n in {'add','sub','subtract','mul','multiply','div','divide','truediv','floordiv','mod','pow','eq','ne','lt','le','gt','ge','compare'}:
            if 'const DataFrame&' in sig:
                if n == 'pow':
                    return df_pow_pair() + '\n\nauto out = left.pow(right);'
                if n == 'truediv':
                    return df_pair_aligned() + '\n\nauto out = left.truediv(right);'
                return df_pair_aligned() + f'\n\nauto out = left.{n}(right);'
            if n == 'pow':
                return df_small_numeric() + '\n\nauto out = df.pow(2);'
            if n == 'lt':
                return df_base() + '\n\nauto out = df.lt(110);'
            return df_base() + f'\n\nauto out = df.{n}(10);'
        if n in {'radd','rsub','rmul','rdiv','rtruediv','rfloordiv','rmod','rpow'}:
            return df_small_numeric() + f'\n\nauto out = df.{n}(10);'
        if n in {'sum','mean','median','min','max','std','var','count','describe','transpose','tally','window','rolling','matrix','prod'}:
            if n == 'window' or n == 'rolling':
                return df_base() + '\n\nauto out = df.' + n + '({{.output = "sales_roll", .feature = "sales", .op = pluss::table::ThreadWindowOp::RollingMean, .window = 2, .min_periods = 1}}, false);'
            if n == 'prod':
                if 'axis' in sig:
                    return df_small_numeric() + '\n\nauto out = df.prod(1, true);'
                return df_small_numeric() + '\n\nauto out = df.prod(false);'
            if n == 'transpose':
                return df_base() + '\n\nauto out = df.transpose(false);'
            if n == 'matrix':
                return (
                    df_base()
                    + '\n\n'
                    + 'auto blocks = df.matrix();\n'
                    + 'auto out = fmt::format(\n'
                    + '    "series blocks: {}\\nnote blocks: {}\\ntimeline blocks: {}\\ntimeindex blocks: {}",\n'
                    + '    blocks.series_span().size(),\n'
                    + '    blocks.note_span().size(),\n'
                    + '    blocks.timeline_span().size(),\n'
                    + '    blocks.timeindex_span().size());'
                )
            if n == 'tally':
                if 'pluss::msharp::RangeView::Of<std::string_view> features' in sig:
                    return df_group() + '\n\nauto out = df.tally({"city", "product"}, 5, true, false);'
                if 'std::string_view feature' in sig:
                    return df_group() + '\n\nauto out = df.tally("city", 3, true, false);'
                return df_group() + '\n\nauto out = df.tally(3, true, false);'
            if n == 'count':
                if 'int axis' in sig:
                    return df_small_numeric() + '\n\nauto out = df.count(1);'
                if 'std::int64_t axis' in sig:
                    return df_small_numeric() + '\n\nauto out = df.count(static_cast<std::int64_t>(1));'
                return df_base() + '\n\nauto out = df.count(false);'
            if n in {'sum', 'mean', 'median', 'min', 'max', 'std', 'var'}:
                if 'int axis' in sig:
                    return df_small_numeric() + f'\n\nauto out = df.{n}(1, true);'
                if 'std::int64_t axis' in sig:
                    return df_small_numeric() + f'\n\nauto out = df.{n}(static_cast<std::int64_t>(1), true);'
                return df_base() + f'\n\nauto out = df.{n}(false);'
            return df_base() + f'\n\nauto out = df.{n}(false);'
        if n == 'mode':
            return df_duplicate_rows() + '\n\nauto out = df.mode(true);'
        if n == 'percentiles':
            return df_base() + '\n\nstd::vector<double> points{0.25, 0.5, 0.75};\nauto out = df.percentiles(points, 1);'
        if n == 'scan':
            return df_base() + '\n\nauto out = df.scan(pluss::table::ScanStatistic::Sum, true);'
        if n in {'all', 'any'}:
            base = (
                'auto df = pluss::table::DataFrame({\n'
                '    {"passed_math", pluss::table::Thread(pluss::make_series({true, true, false}, pluss::codem::ScalarType::Bool))},\n'
                '    {"passed_lab", pluss::table::Thread(pluss::make_series({true, false, false}, pluss::codem::ScalarType::Bool))},\n'
                '});\n\n'
            )
            if 'std::int64_t axis' in sig:
                return base + f'auto out = df.{n}(1, {{"passed_math", "passed_lab"}}, true);'
            return base + f'auto out = df.{n}({{"passed_math", "passed_lab"}}, true);'
        if n in {'isna','isnull','notna','notnull'}:
            if n in {'notna', 'notnull'}:
                return (
                    'auto sales = pluss::table::Thread(pluss::make_series({100.0, std::numeric_limits<double>::quiet_NaN(), 90.0}));\n'
                    'auto df = pluss::table::DataFrame({\n'
                    '    {"sales", sales},\n'
                    '});\n\n'
                    f'auto out = df.{n}({{"sales"}});'
                )
            return df_base() + f'\n\nauto out = df.{n}({{"sales"}});'
        if n in {'fillnull','fillnull_','dropnull','dropna_all','ffill','bfill','count_null_rows'}:
            if n == 'fillnull' or n == 'fillnull_':
                mut = n.endswith('_')
                if 'std::string_view' in sig or 'const char*' in sig:
                    code = 'auto df = pluss::table::DataFrame({{"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))}});'
                    call = ('df.fillnull_("Unknown");' if mut else 'auto out = df.fillnull("Unknown");')
                    return code + '\n\n' + call
                if 'Timeline' in sig:
                    return 'auto df = pluss::table::DataFrame({{"ts", pluss::table::Thread(pluss::make_timeline({"2024-01-01T09:10:00Z", "2024-01-01T09:40:00Z"}))}});\n\n' + ('df.fillnull_(pluss::make_timeline("2024-01-01T00:00:00Z"));' if mut else 'auto out = df.fillnull(pluss::make_timeline("2024-01-01T00:00:00Z"));')
                if 'TimeIndex' in sig:
                    return 'auto df = pluss::table::DataFrame({{"idx", pluss::table::Thread(pluss::make_timeindex({"2024-01-01T09:10:00Z", "2024-01-01T09:40:00Z"}))}});\n\n' + ('df.fillnull_(pluss::make_timeindex("2024-01-01T00:00:00Z"));' if mut else 'auto out = df.fillnull(pluss::make_timeindex("2024-01-01T00:00:00Z"));')
                return df_base() + ('\n\ndf.fillnull_(0.0);' if mut else '\n\nauto out = df.fillnull(0.0, {"sales"});')
            if n == 'dropna_all':
                return df_base() + '\n\nauto out = df.dropna_all({"sales"});'
            if n == 'count_null_rows':
                return df_base() + '\n\nauto out = df.count_null_rows();'
            if n == 'bfill':
                return (
                    'auto score = pluss::table::Thread(pluss::make_series({71.0, std::numeric_limits<double>::quiet_NaN(), 88.0, std::numeric_limits<double>::quiet_NaN()}));\n'
                    'auto df = pluss::table::DataFrame({\n'
                    '    {"score", score},\n'
                    '    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi", "Tamale"}))},\n'
                    '});\n'
                    '\n'
                    'auto out = df.bfill();'
                )
            if n == 'dropnull':
                return (
                    'auto sales = pluss::table::Thread(pluss::make_series({100.0, std::numeric_limits<double>::quiet_NaN(), 90.0}));\n'
                    'auto df = pluss::table::DataFrame({\n'
                    '    {"id", pluss::make_series({1, 2, 3})},\n'
                    '    {"sales", sales},\n'
                    '    {"city", pluss::make_note({"Accra", "Tema", "Kumasi"})},\n'
                    '});\n\n'
                    'auto out = df.dropnull();'
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
                if 'std::ostream& os' in sig:
                    return df_base() + '\n\nstd::ostringstream os;\ndf.info(os);'
                return df_base() + '\n\ndf.info();'
            if n.startswith('save_') or n == 'save_as':
                if 'const pluss::loader::WriteCsvOptions& options' in sig:
                    return df_base() + f'\n\npluss::loader::WriteCsvOptions options;\ndf.save_csv("{GENERIC_OUTPUT_CSV_PATH}", options);'
                if 'const pluss::loader::WriteExcelOptions& options' in sig:
                    return df_base() + f'\n\npluss::loader::WriteExcelOptions options;\ndf.save_excel("{GENERIC_OUTPUT_XLSX_PATH}", options);'
                if 'const pluss::loader::Config& config' in sig:
                    if n == 'save_as':
                        return df_base() + f'\n\npluss::loader::Config config{{}};\ndf.save_as("json", "{GENERIC_OUTPUT_JSON_PATH}", config);'
                    target = {
                        'save_csv': GENERIC_OUTPUT_CSV_PATH,
                        'save_excel': GENERIC_OUTPUT_XLSX_PATH,
                        'save_txt': GENERIC_OUTPUT_TXT_PATH,
                        'save_json': GENERIC_OUTPUT_JSON_PATH,
                        'save_parquet': GENERIC_OUTPUT_PARQUET_PATH,
                    }[n]
                    return df_base() + f'\n\npluss::loader::Config config{{}};\ndf.{n}("{target}", config);'
                if 'initializer_list<pluss::loader::ConfigEntry>' in sig:
                    if n == 'save_as':
                        return df_base() + f'\n\ndf.save_as("json", "{GENERIC_OUTPUT_JSON_PATH}", {{"indent", 2}});'
                    target = {
                        'save_csv': GENERIC_OUTPUT_CSV_PATH,
                        'save_excel': GENERIC_OUTPUT_XLSX_PATH,
                        'save_txt': GENERIC_OUTPUT_TXT_PATH,
                        'save_json': GENERIC_OUTPUT_JSON_PATH,
                        'save_parquet': GENERIC_OUTPUT_PARQUET_PATH,
                    }[n]
                    return df_base() + f'\n\ndf.{n}("{target}", {{"indent", 2}});'
                if 'bool add_cases' in sig:
                    return df_base() + f'\n\ndf.save_csv("{GENERIC_OUTPUT_CSV_PATH}", true, true, \',\', "NA", "case_id");'
                if 'std::string sheet_name' in sig:
                    return df_base() + f'\n\ndf.save_excel("{GENERIC_OUTPUT_XLSX_PATH}", "Sheet1", true, true, "NA", "case_id");'
                target = {
                    'save_csv': f'"{GENERIC_OUTPUT_CSV_PATH}"', 'save_excel': f'"{GENERIC_OUTPUT_XLSX_PATH}"', 'save_txt': f'"{GENERIC_OUTPUT_TXT_PATH}"',
                    'save_json': f'"{GENERIC_OUTPUT_JSON_PATH}"', 'save_parquet': f'"{GENERIC_OUTPUT_PARQUET_PATH}"', 'save_as': f'"json", "{GENERIC_OUTPUT_JSON_PATH}"'
                }[n]
                return df_base() + f'\n\ndf.{n}({target});'
        if n == 'title':
            if method.return_type.strip() == 'std::string':
                return df_base() + '\n\nauto titled = df.title("sales_report");\nauto out = titled.title();'
            return df_base() + '\n\nauto out = df.title("sales_report");'
        if n == 'title_':
            return df_base() + '\n\ndf.title_("sales_report");'
        if n in {'defined','unique'}:
            return df_base() + f'\n\nauto out = df.{n}();'
        if n == 'ts':
            if 'std::initializer_list<std::string_view> features' in sig or 'pluss::msharp::RangeView::Of<std::string_view> features' in sig:
                return df_time() + '\n\nauto out = df.features({"load"}).ts();'
            return df_time() + '\n\nauto out = df.features({"load"}).ts();'
        if n == 'show':
            if 'std::ostream& os' in sig:
                return df_base() + '\n\nstd::ostringstream os;\ndf.show(os);'
            return df_base() + '\n\ndf.show();'
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
        if n == 'step':
            if 'StepPlotOptions' in sig:
                return (
                    base + '\n\npluss::table::StepPlotOptions options;\n'
                    'options.line_width = 2.0;\n'
                    'auto fig = plot.step("sales", "id", options);'
                )
            if 'initializer_list<std::string_view>' in sig or 'RangeView::Of<std::string_view> y_features' in sig:
                return (
                    'auto df = pluss::table::DataFrame({\n'
                    '    {"month", pluss::make_note({"Jan", "Feb", "Mar"})},\n'
                    '    {"sales", pluss::make_series({100.0, 120.0, 90.0})},\n'
                    '    {"cost", pluss::make_series({60.0, 80.0, 55.0})},\n'
                    '});\n'
                    'auto plot = df.plot();\n\n'
                    'auto fig = plot.step({"sales", "cost"}, "month");'
                )
            return base + '\n\nauto fig = plot.step("sales", "id");'
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
        if n == 'area_stacked':
            if 'AreaStackedPlotOptions' in sig:
                return (
                    'auto df = pluss::table::DataFrame({\n'
                    '    {"month", pluss::make_note({"Jan", "Feb", "Mar"})},\n'
                    '    {"sales", pluss::make_series({100.0, 120.0, 90.0})},\n'
                    '    {"cost", pluss::make_series({60.0, 80.0, 55.0})},\n'
                    '});\n'
                    'auto plot = df.plot();\n\n'
                    'pluss::table::AreaStackedPlotOptions options;\n'
                    'options.alpha = 0.6;\n'
                    'auto fig = plot.area_stacked({"sales", "cost"}, "month", options);'
                )
            return (
                'auto df = pluss::table::DataFrame({\n'
                '    {"month", pluss::make_note({"Jan", "Feb", "Mar"})},\n'
                '    {"sales", pluss::make_series({100.0, 120.0, 90.0})},\n'
                '    {"cost", pluss::make_series({60.0, 80.0, 55.0})},\n'
                '});\n'
                'auto plot = df.plot();\n\n'
                'auto fig = plot.area_stacked({"sales", "cost"}, "month");'
            )
        if n == 'scatter':
            return base + '\n\nauto fig = plot.scatter("id", "sales");'
        if n == 'violin':
            if 'value_feature, std::string_view group_feature' in sig and 'ViolinPlotOptions' in sig:
                return (
                    'auto df = pluss::table::DataFrame({\n'
                    '    {"section", pluss::make_note({"A", "A", "B", "B"})},\n'
                    '    {"score", pluss::make_series({71.0, 88.0, 79.0, 95.0})},\n'
                    '});\n'
                    'auto plot = df.plot();\n\n'
                    'pluss::table::ViolinPlotOptions options;\n'
                    'options.inner = pluss::pl::ViolinInner::Quartile;\n'
                    'auto fig = plot.violin("score", "section", options);'
                )
            if 'value_feature, std::string_view group_feature' in sig:
                return (
                    'auto df = pluss::table::DataFrame({\n'
                    '    {"section", pluss::make_note({"A", "A", "B", "B"})},\n'
                    '    {"score", pluss::make_series({71.0, 88.0, 79.0, 95.0})},\n'
                    '});\n'
                    'auto plot = df.plot();\n\n'
                    'auto fig = plot.violin("score", "section");'
                )
            if 'ViolinPlotOptions' in sig:
                return (
                    'auto df = pluss::table::DataFrame({\n'
                    '    {"sales", pluss::make_series({100.0, 120.0, 90.0})},\n'
                    '    {"cost", pluss::make_series({60.0, 80.0, 55.0})},\n'
                    '});\n'
                    'auto plot = df.plot();\n\n'
                    'pluss::table::ViolinPlotOptions options;\n'
                    'options.width = 0.65;\n'
                    'auto fig = plot.violin({"sales", "cost"}, options);'
                )
            return (
                'auto df = pluss::table::DataFrame({\n'
                '    {"sales", pluss::make_series({100.0, 120.0, 90.0})},\n'
                '    {"cost", pluss::make_series({60.0, 80.0, 55.0})},\n'
                '});\n'
                'auto plot = df.plot();\n\n'
                'auto fig = plot.violin({"sales", "cost"});'
            )
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
            if 'HistPlotOptions' in sig:
                return (
                    base + '\n\npluss::table::HistPlotOptions options;\n'
                    'options.bins = 12;\n'
                    'auto fig = plot.hist("sales", options);'
                )
            if 'RangeView::Of<std::string_view> features' in sig or 'initializer_list<std::string_view> features' in sig:
                return (
                    'auto df = pluss::table::DataFrame({\n'
                    '    {"sales", pluss::make_series({100.0, 120.0, 90.0})},\n'
                    '    {"cost", pluss::make_series({60.0, 80.0, 55.0})},\n'
                    '});\n'
                    'auto plot = df.plot();\n\n'
                    'auto fig = plot.hist({"sales", "cost"}, 12);'
                )
            return base + '\n\nauto fig = plot.hist("sales", 12);'
        if n == 'pie':
            if 'PiePlotOptions' in sig:
                return (
                    'auto df = pluss::table::DataFrame({\n'
                    '    {"city", pluss::make_note({"Accra", "Tema", "Kumasi"})},\n'
                    '    {"sales", pluss::make_series({100.0, 120.0, 90.0})},\n'
                    '});\n'
                    'auto plot = df.plot();\n\n'
                    'pluss::table::PiePlotOptions options;\n'
                    'options.show_value = true;\n'
                    'auto fig = plot.pie("sales", "city", options);'
                )
            return (
                'auto df = pluss::table::DataFrame({\n'
                '    {"city", pluss::make_note({"Accra", "Tema", "Kumasi"})},\n'
                '    {"sales", pluss::make_series({100.0, 120.0, 90.0})},\n'
                '});\n'
                'auto plot = df.plot();\n\n'
                'auto fig = plot.pie("sales", "city");'
            )
        if n == 'heatmap':
            if 'case_feature' in sig and 'HeatmapPlotOptions' in sig:
                return (
                    'auto df = pluss::table::DataFrame({\n'
                    '    {"campus", pluss::make_note({"North", "North", "West", "West"})},\n'
                    '    {"exam", pluss::make_note({"Midterm", "Final", "Midterm", "Final"})},\n'
                    '    {"score", pluss::make_series({71.0, 88.0, 79.0, 95.0})},\n'
                    '});\n'
                    'auto plot = df.plot();\n\n'
                    'pluss::table::HeatmapPlotOptions options;\n'
                    'options.annotate_levels = true;\n'
                    'auto fig = plot.heatmap("campus", "exam", "score", pluss::table::DataFrame::GroupOp::Mean, options);'
                )
            if 'case_feature' in sig:
                return (
                    'auto df = pluss::table::DataFrame({\n'
                    '    {"campus", pluss::make_note({"North", "North", "West", "West"})},\n'
                    '    {"exam", pluss::make_note({"Midterm", "Final", "Midterm", "Final"})},\n'
                    '    {"score", pluss::make_series({71.0, 88.0, 79.0, 95.0})},\n'
                    '});\n'
                    'auto plot = df.plot();\n\n'
                    'auto fig = plot.heatmap("campus", "exam", "score", pluss::table::DataFrame::GroupOp::Mean);'
                )
            if 'HeatmapPlotOptions' in sig:
                return (
                    'auto df = pluss::table::DataFrame({\n'
                    '    {"sales", pluss::make_series({100.0, 120.0, 90.0})},\n'
                    '    {"cost", pluss::make_series({60.0, 80.0, 55.0})},\n'
                    '});\n'
                    'auto plot = df.plot();\n\n'
                    'pluss::table::HeatmapPlotOptions options;\n'
                    'options.annotate_levels = true;\n'
                    'auto fig = plot.heatmap({"sales", "cost"}, options);'
                )
            return (
                'auto df = pluss::table::DataFrame({\n'
                '    {"sales", pluss::make_series({100.0, 120.0, 90.0})},\n'
                '    {"cost", pluss::make_series({60.0, 80.0, 55.0})},\n'
                '});\n'
                'auto plot = df.plot();\n\n'
                'auto fig = plot.heatmap({"sales", "cost"});'
            )
        if n == 'scatter_matrix':
            if 'ScatterMatrixTriangle triangle' in sig:
                return (
                    'auto df = pluss::table::DataFrame({\n'
                    '    {"x1", pluss::make_series({1.0, 2.0, 3.0})},\n'
                    '    {"x2", pluss::make_series({4.0, 3.0, 2.0})},\n'
                    '    {"x3", pluss::make_series({2.0, 5.0, 4.0})},\n'
                    '});\n'
                    'auto plot = df.plot();\n\n'
                    'auto fig = plot.scatter_matrix({"x1", "x2", "x3"}, pluss::table::ScatterMatrixTriangle::Lower);'
                )
            if 'ScatterMatrixPlotOptions' in sig:
                return (
                    'auto df = pluss::table::DataFrame({\n'
                    '    {"x1", pluss::make_series({1.0, 2.0, 3.0})},\n'
                    '    {"x2", pluss::make_series({4.0, 3.0, 2.0})},\n'
                    '    {"x3", pluss::make_series({2.0, 5.0, 4.0})},\n'
                    '});\n'
                    'auto plot = df.plot();\n\n'
                    'pluss::table::ScatterMatrixPlotOptions options;\n'
                    'options.corner = true;\n'
                    'auto fig = plot.scatter_matrix({"x1", "x2", "x3"}, options);'
                )
            return (
                'auto df = pluss::table::DataFrame({\n'
                '    {"x1", pluss::make_series({1.0, 2.0, 3.0})},\n'
                '    {"x2", pluss::make_series({4.0, 3.0, 2.0})},\n'
                '    {"x3", pluss::make_series({2.0, 5.0, 4.0})},\n'
                '});\n'
                'auto plot = df.plot();\n\n'
                'auto fig = plot.scatter_matrix({"x1", "x2", "x3"});'
            )
        if n == 'parallel':
            return (
                'auto df = pluss::table::DataFrame({\n'
                '    {"x1", pluss::make_series({1.0, 2.0, 3.0})},\n'
                '    {"x2", pluss::make_series({4.0, 3.0, 2.0})},\n'
                '    {"x3", pluss::make_series({2.0, 5.0, 4.0})},\n'
                '    {"group", pluss::make_note({"A", "B", "A"})},\n'
                '});\n'
                'auto plot = df.plot();\n\n'
                'auto fig = plot.parallel({"x1", "x2", "x3"}, "group");'
            )
        if n == 'radviz':
            return (
                'auto df = pluss::table::DataFrame({\n'
                '    {"x1", pluss::make_series({1.0, 2.0, 3.0})},\n'
                '    {"x2", pluss::make_series({4.0, 3.0, 2.0})},\n'
                '    {"x3", pluss::make_series({2.0, 5.0, 4.0})},\n'
                '    {"group", pluss::make_note({"A", "B", "A"})},\n'
                '});\n'
                'auto plot = df.plot();\n\n'
                'auto fig = plot.radviz({"x1", "x2", "x3"}, "group");'
            )
        if n == 'lag':
            return (
                base + '\n\npluss::table::LagPlotOptions options;\n'
                'options.lag = 2;\n'
                'auto fig = plot.lag("sales", options);'
            )
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
            return (
                'auto df = pluss::table::DataFrame({\n'
                '    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},\n'
                '    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},\n'
                '    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},\n'
                '});\n'
                'auto stmt = df.prep_sql("SELECT id, sales FROM self WHERE sales > 95");\n\n'
                'auto out = stmt.execute();'
            )
        return base + f'\n\nauto out = stmt.{n}();'

    if class_name == 'Thread':
        if n in {'defined','unique','size','kind','dtype','value','values','has_caselbl'}:
            base = thread_series()
            return base + f'\n\nauto out = thread.{n}();'
        if n == 'caselbl':
            if 'const Thread& labels' in sig:
                return (
                    'auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}))'
                    '.caselbl(pluss::table::Thread(pluss::make_note({"row0", "row1", "row2"})));\n'
                    'auto labels = pluss::table::Thread(pluss::make_note({"A", "B", "C"}));\n\n'
                    'auto out = thread.caselbl(labels);'
                )
            return (
                'auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}))'
                '.caselbl(pluss::table::Thread(pluss::make_note({"row0", "row1", "row2"})));\n\n'
                'auto out = thread.label().values();'
            )
        if n == 'caselbl_':
            return (
                'auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));\n'
                'auto labels = pluss::table::Thread(pluss::make_note({"A", "B", "C"}));\n\n'
                'thread.caselbl_(labels);'
            )
        if n in {'clear_caselbl', 'clear_caselbl_'}:
            base = (
                'auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}))'
                '.caselbl(pluss::table::Thread(pluss::make_note({"row0", "row1", "row2"})));'
            )
            return base + ('\n\nauto out = thread.clear_caselbl();' if n == 'clear_caselbl' else '\n\nthread.clear_caselbl_();')
        if n == 'align':
            return (
                'auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}))'
                '.caselbl(pluss::table::Thread(pluss::make_note({"s1", "s2", "s3"})));\n'
                'auto target = pluss::table::Thread(pluss::make_note({"s3", "s1", "s2"}));\n\n'
                'auto out = thread.align(target);'
            )
        if n in {'isnull','notnull','isnan','isfinite','abs','log','exp','dropnull','ffill','bfill','sum','mean','min','max'}:
            if n == 'dropnull':
                return (
                    'auto thread = pluss::table::Thread(pluss::make_series({10.0, std::numeric_limits<double>::quiet_NaN(), 30.0}));\n\n'
                    'auto out = thread.dropnull();'
                )
            if n == 'bfill':
                return (
                    'auto thread = pluss::table::Thread(pluss::make_series({10.0, std::numeric_limits<double>::quiet_NaN(), 30.0, std::numeric_limits<double>::quiet_NaN()}));\n\n'
                    'auto out = thread.bfill();'
                )
            return thread_series() + f'\n\nauto out = thread.{n}();'
        if n in {'var','std'}:
            return thread_series() + f'\n\nauto out = thread.{n}(true);'
        if n in {'fillnull','fillnull_','clip'}:
            if 'std::string_view' in sig or 'const char*' in sig:
                base = thread_note()
                return base + ('\n\nthread.fillnull_("Unknown");' if n.endswith('_') else '\n\nauto out = thread.fillnull("Unknown");')
            if 'Timeline' in sig:
                base = thread_time()
                return base + ('\n\nthread.fillnull_(pluss::make_timeline("2024-01-01T00:00:00Z"));' if n.endswith('_') else '\n\nauto out = thread.fillnull(pluss::make_timeline("2024-01-01T00:00:00Z"));')
            if 'TimeIndex' in sig:
                base = 'auto thread = pluss::table::Thread(pluss::make_timeindex({"2024-01-01T09:10:00Z", "2024-02-01T11:00:00Z"}));'
                return base + ('\n\nthread.fillnull_(pluss::make_timeindex("2024-01-01T00:00:00Z"));' if n.endswith('_') else '\n\nauto out = thread.fillnull(pluss::make_timeindex("2024-01-01T00:00:00Z"));')
            if n == 'clip':
                return thread_series() + '\n\nauto out = thread.clip(0.0, 25.0);'
            return thread_series() + ('\n\nthread.fillnull_(0.0);' if n.endswith('_') else '\n\nauto out = thread.fillnull(0.0);')
        if n in {'series','note','timeline','timeindex','as_series','as_note','as_timeline','as_timeindex'}:
            base = thread_series() if 'series' in n else thread_note() if 'note' in n else thread_time()
            return base + f'\n\nauto out = thread.{n}();'
        if n in {'head','tail','slice','take','filter','window','cast','astype','bucketize','btw','between','corr','count'}:
            if n == 'head': return thread_series() + '\n\nauto out = thread.head(2);'
            if n == 'tail': return thread_series() + '\n\nauto out = thread.tail(2);'
            if n == 'slice': return thread_series() + '\n\nauto out = thread.slice(0, 2);'
            if n == 'take': return thread_series() + '\n\nstd::vector<std::int64_t> picks{0, 2};\nauto out = thread.take(picks);'
            if n == 'filter': return thread_series() + '\n\nauto mask = pluss::make_series({true, false, true}, pluss::codem::ScalarType::Bool);\nauto out = thread.filter(mask);'
            if n == 'window': return thread_series() + '\n\nauto out = thread.window(2, pluss::table::ThreadWindowOp::RollingMean, 1);'
            if n == 'cast': return 'auto thread = pluss::table::Thread(pluss::make_note({"1.0", "2.0", "3.0"}));\n\nauto out = thread.cast(pluss::table::ThreadCastTarget::Series, pluss::codem::ScalarType::Float64);'
            if n == 'astype': return thread_series() + '\n\nauto out = thread.astype(pluss::codem::ScalarType::Float64);'
            if n == 'corr': return 'auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));\nauto other = pluss::table::Thread(pluss::make_series({12.0, 18.0, 29.0}));\n\nauto out = thread.corr(other, pluss::table::ThreadCorrMethod::Pearson);'
            if n == 'count': return 'auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));\n\nauto out = thread.count(true);'
            if n == 'bucketize':
                return (
                    'auto thread = pluss::table::Thread(pluss::make_series({12.0, 18.0, 27.0, 35.0, std::numeric_limits<double>::quiet_NaN()}));\n'
                    'std::vector<double> bins{0.0, 20.0, 30.0, 40.0};\n\n'
                    'auto out = thread.bucketize(bins, {}, true, true);'
                )
            if n in {'btw', 'between'}:
                if 'double a, double b' in sig:
                    return (
                        'auto thread = pluss::table::Thread(pluss::make_series({45.0, 65.0, 82.0, 101.0}));\n\n'
                        f'auto out = thread.{n}(50.0, 100.0, true);'
                    )
                if 'std::string_view lo, std::string_view hi' in sig:
                    return (
                        'auto thread = pluss::table::Thread(pluss::make_timeline({\n'
                        '    "2024-01-05T09:00:00Z",\n'
                        '    "2024-01-18T12:00:00Z",\n'
                        '    "2024-02-02T08:00:00Z",\n'
                        '}));\n\n'
                        f'auto out = thread.{n}("2024-01-01T00:00:00Z", "2024-01-31T23:59:59Z", true);'
                    )
                return (
                    'auto thread = pluss::table::Thread(pluss::make_series({45.0, 65.0, 82.0}));\n'
                    'auto lo = pluss::table::Thread(pluss::make_series({40.0, 60.0, 80.0}));\n'
                    'auto hi = pluss::table::Thread(pluss::make_series({50.0, 70.0, 81.0}));\n\n'
                    f'auto out = thread.{n}(lo, hi, true);'
                )
        if n in {'lower','replace','strip','lstrip','rstrip','distinct','describe','display_labels','display_name'}:
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
            return thread_note() + '\n\nauto out = thread.lower();'
        if n in {'to_note_weekday'}:
            return thread_time() + f'\n\nauto out = thread.{n}();'
        if n in {'numeric_binary','compare'}:
            if n == 'numeric_binary':
                return thread_series() + '\n\nauto out = thread.numeric_binary(10.0, pluss::table::ThreadBinaryMathOp::Add, false);'
            if 'const Thread&' in sig:
                return thread_series() + '\n\nauto other = pluss::table::Thread(pluss::make_series({15.0, 10.0, 35.0}));\nauto out = thread.compare(other, pluss::table::ThreadBinaryCompareOp::Gt);'
            return thread_series() + '\n\nauto out = thread.compare(15.0, pluss::table::ThreadBinaryCompareOp::Gt);'
        if n in {'to_string','show'}:
            return thread_series() + ('\n\nauto out = thread.to_string();' if n == 'to_string' else '\n\nthread.show();')
        base = thread_note() if n in {'chars', 'lower', 'strip', 'lstrip', 'rstrip'} else thread_time() if n in {'year', 'month', 'day', 'hour', 'to_note_weekday'} else thread_series()
        prelude, args = thread_fallback_args(method)
        return base + ('\n' + prelude if prelude else '') + f'\n\n{call_line("thread", n, args)}'

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
            if n == 'tabelize': return 'using namespace pluss::kindle::literals;\nauto c0 = pluss::kindle::Catalog({"sales"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0}))});\nauto c1 = pluss::kindle::Catalog({"sales"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0}))});\nauto archive = pluss::kindle::Archive({c0, c1});\nauto out = archive.tabelize();'
            if n == 'kindelize': return atlas_base() + '\n\nauto out = atlas.kindelize();'
            if n == 'to_string': return atlas_base() + '\n\nauto out = atlas.to_string();'
            if n == 'show': return atlas_base() + '\n\natlas.show();'
        return atlas_base() + f'\n\nauto out = atlas.{n}();'

    if class_name == 'Stream':
        base = stream_series()
        if n in {'defined','unique','size','kind','dtype','has_cases','cases','to_string','to_json'}:
            return base + f'\n\nauto out = stream.{n}();'
        if n in {'isnull','notnull','abs','log','exp','clear_cases','clear_cases_','head','tail','dropnull','ffill','bfill','sum','mean','min','max'}:
            if n == 'bfill':
                return (
                    'auto stream = pluss::kindle::Stream(pluss::make_series({10.0, std::numeric_limits<double>::quiet_NaN(), 30.0, std::numeric_limits<double>::quiet_NaN()}));\n\n'
                    'auto out = stream.bfill();'
                )
            if n == 'dropnull':
                return (
                    'auto stream = pluss::kindle::Stream(pluss::make_series({10.0, std::numeric_limits<double>::quiet_NaN(), 30.0}));\n\n'
                    'auto out = stream.dropnull();'
                )
            if n.endswith('_'): return base + f'\n\nstream.{n}();'
            return base + f'\n\nauto out = stream.{n}();'
        if n in {'var','std'}:
            return base + f'\n\nauto out = stream.{n}(true);'
        if n in {'fillnull','clip','with_cases','with_cases_','with_meta','with_meta_','drop_meta','drop_meta_','meta','has_meta','take','filter','slice','cast','astype','to_thread'}:
            if n == 'fillnull':
                if 'std::string_view' in sig or 'const char*' in sig:
                    return stream_note() + '\n\nauto out = stream.fillnull("Unknown");'
                if 'Timeline' in sig:
                    return stream_time() + '\n\nauto out = stream.fillnull(pluss::make_timeline("2024-01-01T00:00:00Z"));'
                if 'TimeIndex' in sig:
                    return 'auto stream = pluss::kindle::Stream(pluss::make_timeindex({"2024-01-01T09:10:00Z", "2024-01-01T09:40:00Z"}));\n\nauto out = stream.fillnull(pluss::make_timeindex("2024-01-01T00:00:00Z"));'
                return base + '\n\nauto out = stream.fillnull(0.0);'
            if n == 'clip': return base + '\n\nauto out = stream.clip(0.0, 25.0);'
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
            if n == 'load_labeled':
                return f'auto out = pluss::kindle::Catalog::load_labeled("{GENERIC_CSV_PATH}", "id");'
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
        if n in {'defined','unique','empty','keys','nrows','minrows','maxrows','nkeys','shape','size','to_string','to_json'}:
            return base + f'\n\nauto out = catalog.{n}();'
        if n in {'has','at','get'}:
            return base + f'\n\nauto out = catalog.{n}("metric");'
        if n in {'head','tail','dropnull','ffill','bfill','isnull','notnull','drop','display','info'}:
            if n == 'bfill':
                return (
                    'using namespace pluss::kindle::literals;\n'
                    'auto catalog = pluss::kindle::Catalog({\n'
                    '    "Final Points Grade"_k = pluss::kindle::Stream(pluss::make_series({71.0, std::numeric_limits<double>::quiet_NaN(), 88.0, std::numeric_limits<double>::quiet_NaN()})),\n'
                    '    "Midterm 3"_k = pluss::kindle::Stream(pluss::make_series({60.0, std::numeric_limits<double>::quiet_NaN(), 75.0, 91.0})),\n'
                    '    "Name"_k = pluss::kindle::Stream(pluss::make_note({"A", "B", "C", "D"})),\n'
                    '});\n\n'
                    'auto out = catalog.keys({"Final Points Grade", "Midterm 3"}).bfill();'
                )
            if n == 'dropnull':
                return (
                    'using namespace pluss::kindle::literals;\n'
                    'auto metric = pluss::kindle::Stream(pluss::make_series({10.0, std::numeric_limits<double>::quiet_NaN(), 30.0}));\n'
                    'auto catalog = pluss::kindle::Catalog({\n'
                    '    "metric"_k = metric,\n'
                    '    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),\n'
                    '});\n\n'
                    'auto out = catalog.keys({"metric"}).dropnull();'
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
        if n in {'slice','take','sample','filter','where','query','select','rename','rename_','sort','group','fillnull','cast','astype','sum','mean','min','max','merge','apply','apply_','map','map_','transform','clean','keywords','similarity','grep','featurize','stats','tfidf','set','set_','tabelize','to_table','save_csv','save_txt','save_excel','save_json','save_parquet','save_as','begin','end'}:
            if n == 'slice': return base + '\n\nauto out = catalog.slice(0, 2);'
            if n == 'take': return base + '\n\nstd::vector<std::int64_t> picks{0, 2};\nauto out = catalog.take(picks);'
            if n == 'sample': return base + '\n\nauto out = catalog.sample(2, 7);'
            if n == 'filter' or n == 'where': return base + f'\n\nauto mask = pluss::make_series({{true, false, true}}, pluss::codem::ScalarType::Bool);\nauto out = catalog.{n}(mask);'
            if n == 'query': return base + '\n\nauto out = catalog.query([](const pluss::kindle::Catalog& c) { return c.at("metric").as_series().gt(15.0); });'
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
            if n == 'sort':
                if 'span<const std::string_view>' in sig or 'initializer_list<std::string_view>' in sig or 'vector<std::string>' in sig:
                    return base + '\n\nauto out = catalog.sort({"city", "id"}, {true, false});'
                return base + '\n\nauto out = catalog.sort("metric", false);'
            if n == 'group': return base + '\n\nauto out = catalog.group().by("city");'
            if n == 'fillnull':
                if 'std::string_view' in sig or 'const char*' in sig:
                    return 'using namespace pluss::kindle::literals;\nauto catalog = pluss::kindle::Catalog({"city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"}))});\n\nauto out = catalog.keys({"city"}).fillnull("Unknown");'
                if 'Timeline' in sig:
                    return 'using namespace pluss::kindle::literals;\nauto catalog = pluss::kindle::Catalog({"ts"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-01-01T09:10:00Z", "2024-01-01T09:40:00Z"}))});\n\nauto out = catalog.keys({"ts"}).fillnull(pluss::make_timeline("2024-01-01T00:00:00Z"));'
                if 'TimeIndex' in sig:
                    return 'using namespace pluss::kindle::literals;\nauto catalog = pluss::kindle::Catalog({"idx"_k = pluss::kindle::Stream(pluss::make_timeindex({"2024-01-01T09:10:00Z", "2024-01-01T09:40:00Z"}))});\n\nauto out = catalog.keys({"idx"}).fillnull(pluss::make_timeindex("2024-01-01T00:00:00Z"));'
                return base + '\n\nauto out = catalog.keys({"metric"}).fillnull(0.0);'
            if n == 'cast':
                return 'using namespace pluss::kindle::literals;\nauto catalog = pluss::kindle::Catalog({"metric"_k = pluss::kindle::Stream(pluss::make_note({"10.0", "20.0", "30.0"}))});\n\nauto out = catalog.keys({"metric"}).cast().series();'
            if n == 'astype':
                return base + '\n\nauto out = catalog.keys({"metric"}).astype(pluss::codem::ScalarType::Float64);'
            if n in {'sum','mean','min','max'}: return base + f'\n\nauto out = catalog.{n}("metric");'
            if n == 'merge':
                return base + '\n\nusing namespace pluss::kindle::literals;\nauto right = pluss::kindle::Catalog({"id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})), "score"_k = pluss::kindle::Stream(pluss::make_series({0.9, 0.7, 0.8}))});\nauto out = catalog.merge(right, "id", "inner");'
            if n in {'apply','apply_'}:
                return base + ('\n\nauto out = catalog.apply("metric", [](const pluss::kindle::Stream& s) { return s.clip(0.0, 25.0); });' if not n.endswith('_') else '\n\ncatalog.apply_("metric", [](const pluss::kindle::Stream& s) { return s.clip(0.0, 25.0); });')
            if n in {'map','map_','transform'}:
                if n == 'transform' and 'std::string_view field' in sig:
                    return base + '\n\nauto out = catalog.transform("metric", [](const pluss::kindle::Stream& s) { return s.clip(0.0, 25.0); });'
                if n == 'transform':
                    return base + '\n\nauto out = catalog.transform([](const pluss::kindle::Stream& s) { return s.kind() == pluss::kindle::StreamKind::Series ? s.clip(0.0, 25.0) : s; });'
                return base + (f'\n\nauto out = catalog.{n}([](const pluss::kindle::Stream& s) {{ return s.kind() == pluss::kindle::StreamKind::Series ? s.clip(0.0, 25.0) : s; }});' if not n.endswith('_') else f'\n\ncatalog.{n}([](const pluss::kindle::Stream& s) {{ return s.kind() == pluss::kindle::StreamKind::Series ? s.clip(0.0, 25.0) : s; }});')
            if n in {'clean','keywords','similarity','grep','featurize','stats','tfidf'}:
                if n == 'similarity': return catalog_text() + '\n\nauto out = catalog.similarity("text", "text", "cosine");'
                if n == 'keywords': return catalog_text() + '\n\nauto out = catalog.keywords("text", 5);'
                if n == 'grep': return catalog_text() + '\n\nauto out = catalog.grep("text", "rain");'
                if n == 'stats': return catalog_text() + '\n\nauto out = catalog.stats("text", "mean");'
                return catalog_text() + f'\n\nauto out = catalog.{n}("text");'
            if n in {'set','set_'}:
                return base + ('\n\nauto out = catalog.set("flag", pluss::kindle::Stream(pluss::make_series({1, 0, 1})));' if not n.endswith('_') else '\n\ncatalog.set_("flag", pluss::kindle::Stream(pluss::make_series({1, 0, 1})));')
            if n == 'tabelize' or n == 'to_table':
                return base + f'\n\npluss::kindle::TabelizePolicy policy;\npolicy.align = pluss::kindle::Align::Inner;\nauto out = catalog.{n}(policy' + (', true' if n == 'to_table' else '') + ');'
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
        if n in {'defined','unique','nslots','nfields','empty','keys','field_names','slots','slot_sizes','profiles','to_string','to_json'}:
            return base + f'\n\nauto out = registry.{n}();'
        if n in {'has','has_slot','slot','at','get','get_slot','select','drop','head','tail','slice','iloc','take','take_','sample','rename','rename_','apply','apply_','map','map_','combine','clear','clear_','prune','prune_','set_slot','set_slot_','set_slots','set_slots_','push','push_','keep','keep_','morph','morph_','concat','concat_','to_kindle','as_kindle','kindelize','save_csv','save_txt','save_excel','save_json','save_parquet','save_as','show','begin','end'}:
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
            if n == 'take':
                if 'std::string_view key' in sig:
                    return base + '\n\nstd::vector<std::int64_t> picks{0, 2};\nauto out = registry.take("ids", picks);'
                return base + '\n\nstd::vector<std::int64_t> picks{0, 2};\nauto out = registry.take(picks);'
            if n == 'take_': return base + '\n\nstd::vector<std::int64_t> picks{0, 2};\nregistry.take_("ids", picks);'
            if n == 'sample': return base + '\n\nauto out = registry.sample(2, 7);'
            if n == 'rename' or n == 'rename_':
                if 'unordered_map' in sig: return base + '\n\nstd::unordered_map<std::string, std::string> renames{{"ids", "ids_new"}};\nauto out = registry.rename(renames);'
                if 'initializer_list' in sig: return base + '\n\nauto out = registry.rename({{"ids", "ids_new"}});'
                return base + ('\n\nauto out = registry.rename("ids", "ids_new");' if not n.endswith('_') else '\n\nregistry.rename_("ids", "ids_new");')
            if n == 'apply' or n == 'apply_':
                if 'initializer_list<std::string_view> keys' in sig:
                    return base + (f'\n\nauto out = registry.{n}({{"ids"}}, [](const pluss::bundle::Slot& slot) {{ return slot; }});' if not n.endswith('_') else '\n\nregistry.apply_({"ids"}, [](const pluss::bundle::Slot& slot) { return slot; });')
                return base + (f'\n\nauto out = registry.{n}("ids", [](const pluss::bundle::Slot& slot) {{ return slot; }});' if not n.endswith('_') else '\n\nregistry.apply_("ids", [](const pluss::bundle::Slot& slot) { return slot; });')
            if n == 'map': return base + '\n\nauto out = registry.map([](const pluss::bundle::Slot& slot) { return slot; });'
            if n == 'map_': return base + '\n\nregistry.map_([](const pluss::bundle::Slot& slot) { return slot; });'
            if n == 'combine': return base + '\n\nauto other = pluss::bundle::Registry({{"flag", pluss::bundle::Slot({pluss::make_series(true, pluss::codem::ScalarType::Bool), pluss::make_series(false, pluss::codem::ScalarType::Bool), pluss::make_series(true, pluss::codem::ScalarType::Bool)})}});\nauto out = registry.combine(other, "_rhs");'
            if n == 'clear' or n == 'clear_': return base + ('\n\nauto out = registry.clear();' if not n.endswith('_') else '\n\nregistry.clear_();')
            if n == 'prune':
                return (
                    'auto registry = pluss::bundle::Registry({\n'
                    '    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},\n'
                    '    {"empty", pluss::bundle::Slot({})},\n'
                    '    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},\n'
                    '});\n\n'
                    'auto out = registry.prune();'
                )
            if n == 'prune_':
                return (
                    'auto registry = pluss::bundle::Registry({\n'
                    '    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},\n'
                    '    {"empty", pluss::bundle::Slot({})},\n'
                    '    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},\n'
                    '});\n\n'
                    'registry.prune_();'
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
            if n == 'push':
                if 'std::string_view key' in sig:
                    return base + '\n\nauto out = registry.push("ids", pluss::make_series(4));'
                return base + '\n\nauto out = registry.push(pluss::make_note("ok"));'
            if n == 'push_':
                if 'std::string_view key' in sig:
                    return base + '\n\nregistry.push_("ids", pluss::make_series(4));'
                return base + '\n\nregistry.push_(pluss::make_note("ok"));'
            if n in {'keep','keep_'}: return base + ('\n\nauto out = registry.keep("ids", [](const pluss::bundle::Slot::Value& value) { return std::holds_alternative<pluss::Series>(value); });' if not n.endswith('_') else '\n\nregistry.keep_("ids", [](const pluss::bundle::Slot::Value& value) { return std::holds_alternative<pluss::Series>(value); });')
            if n in {'morph','morph_'}: return base + ('\n\nauto out = registry.morph("ids", [](const pluss::bundle::Slot::Value& value) -> pluss::bundle::Slot::Value { if (std::holds_alternative<pluss::Series>(value)) return std::get<pluss::Series>(value).add(10); return value; });' if not n.endswith('_') else '\n\nregistry.morph_("ids", [](const pluss::bundle::Slot::Value& value) -> pluss::bundle::Slot::Value { if (std::holds_alternative<pluss::Series>(value)) return std::get<pluss::Series>(value).add(10); return value; });')
            if n in {'concat','concat_'}: return base + ('\n\nauto extra = pluss::bundle::Slot({pluss::make_series(4), pluss::make_series(5)});\nauto out = registry.concat("ids", extra);' if not n.endswith('_') else '\n\nauto extra = pluss::bundle::Slot({pluss::make_series(4), pluss::make_series(5)});\nregistry.concat_("ids", extra);')
            if n in {'to_kindle','as_kindle','kindelize'}: return base + f'\n\nauto out = registry.{n}();'
            if n.startswith('save_') or n == 'save_as':
                target = {'save_csv':f'"{GENERIC_OUTPUT_CSV_PATH}"','save_txt':f'"{GENERIC_OUTPUT_TXT_PATH}"','save_excel':f'"{GENERIC_OUTPUT_XLSX_PATH}"','save_json':f'"{GENERIC_OUTPUT_JSON_PATH}"','save_parquet':f'"{GENERIC_OUTPUT_PARQUET_PATH}"','save_as':f'"json", "{GENERIC_OUTPUT_JSON_PATH}"'}[n]
                return base + f'\n\nregistry.{n}({target});'
            if n == 'show': return base + '\n\nregistry.show(std::cout);'
            if n in {'begin','end'}: return base + '\n\nfor (const auto& key : registry) {\n  (void)key;\n}'
        if n == 'from_kindle':
            return catalog_base() + '\n\nauto out = pluss::bundle::Registry::from_kindle(catalog);'
        if n in {'has_all','has_any'}:
            return base + f'\n\nauto out = registry.{n}({{"ids", "city"}});'
        return base + f'\n\nauto out = registry.{n}();'

    if class_name == 'Axel':
        base = axel_base()
        if n in {'defined','unique','depth','empty','registries','labels','to_string','to_json'}:
            return base + f'\n\nauto out = axel.{n}();'
        if n in {'registry','head','tail','slice','map','select','apply_slot','concat_depth','to_kindle','as_kindle','kindelize','show'}:
            if n == 'registry': return base + '\n\nauto out = axel.registry(0);'
            if n == 'head': return base + '\n\nauto out = axel.head(1);'
            if n == 'tail': return base + '\n\nauto out = axel.tail(1);'
            if n == 'slice': return base + '\n\nauto out = axel.slice(0, 1);'
            if n == 'map': return base + '\n\nauto out = axel.map([](const pluss::bundle::Registry& registry) { return registry; });'
            if n == 'select': return base + '\n\nauto out = axel.select({"ids"});'
            if n == 'apply_slot': return base + '\n\nauto out = axel.apply_slot("ids", [](const pluss::bundle::Slot& slot) { return slot; });'
            if n == 'concat_depth': return base + '\n\nauto out = axel.concat_depth(axel);'
            if n in {'to_kindle','as_kindle','kindelize'}: return base + f'\n\nauto out = axel.{n}();'
            if n == 'show': return base + '\n\naxel.show(std::cout);'
        if n == 'from_kindle':
            return archive_base() + '\n\nauto out = pluss::bundle::Axel::from_kindle(archive);'
        return base + f'\n\nauto out = axel.{n}();'

    if class_name in ACCESSOR_STUB_BASE:
        return accessor_stub_example(class_name, method)

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
                '- Larger `n` gives you the full ascending slice without writing `arrange(...).head(...)` yourself.',
                '',
                '#### Equivalent Expanded Form',
                '',
                '```cpp',
                'auto out = df.arrange({pluss::table::DataFrame::ArrangeSpec{"sales", true, true}})',
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
        if class_name == 'DataFrame' and n == 'group':
            return bullets('Execution Model', [
                '`group(...)` opens a builder and defers execution until `by(...).agg(...)`, `by(...).transform(...)`, or `by(...).filter(...)` is chosen.',
                'Typed key combinations are factorized once in stable first-seen order and then reused across grouped reductions.',
                'Count, sum, mean, minimum, maximum, sample standard deviation, and sample variance use vectorized Series scatter reductions.',
                'Numeric payloads and validity masks stay on their source platform; key hashing and arbitrary-value distinct counting require one bulk host materialization.',
            ])
        if class_name == 'DataFrame' and n == 'label().align':
            return bullets('Practical Behavior', [
                '`label().align(new_label)` reorders rows by matching the existing case labels against `new_label`.',
                'Every requested case label must already exist or the call throws.',
                'The output case axis is rewritten to `new_label`, so both the row order and the labels now follow that target order.',
            ])
        if class_name == 'DataFrame' and n == 'match':
            return bullets('Matching Rules', [
                '`match(other)` matches both case labels and feature names in one call.',
                'The default `Unite` rule preserves the left order and appends labels found only on the right.',
                '`Common`, `KeepLeft`, and `KeepRight` select the intersection or one side explicitly.',
                'Repeated cases pair by occurrence order by default; choose `RepeatedCaseRule::Reject` when repeated labels should be an error.',
                'Frame-to-frame arithmetic and comparisons use the same default matching behavior automatically.',
                'Only case-key matching is materialized on the host; feature gathers and arithmetic remain on the source Series platform.',
            ])
        if class_name == 'DataFrame' and n == 'lengthen':
            return bullets('Reshape Rules', [
                '`lengthen(...)` tiles identifier features and stacks each value feature as one contiguous case block.',
                'Value features must share a Thread kind; numeric value features must also share dtype and platform.',
                'Payload concatenation and identifier tiling use Series/Thread bulk operations rather than cell-wise conversion.',
                'Missing-value masks are concatenated with the payload and preserved.',
            ])
        if class_name == 'DataFrame' and n in {'all', 'any'}:
            return bullets('Axis Rules', [
                '`axis=0` reduces down the selected boolean features and returns one result per feature.',
                '`axis=1` reduces across features and returns one result per row.',
                'These methods expect boolean features; they are not general numeric reducers.',
                'Nullable boolean input requires `skipnull=true`.',
            ])
        if class_name == 'DataFrame' and n == 'assign':
            return bullets('Practical Behavior', [
                '`assign(...)` adds new derived features or replaces existing ones without mutating the source frame.',
                'Each derived thread must match the frame case count.',
                'Use the pair form when you already have the computed thread, and the spec form when the value should be computed from the current frame inside the call.',
            ])
        if class_name == 'DataFrame' and n == 'reshuffle':
            return bullets('Practical Behavior', [
                '`reshuffle(axis=0)` permutes rows and keeps the feature order unchanged.',
                '`reshuffle(axis=1)` permutes feature order and keeps the row order unchanged.',
                'Pass `random_state` when you need deterministic shuffling for docs, tests, or reproducible data splits.',
            ])
        if class_name == 'DataFrame' and n == 'shuffle':
            return bullets('Practical Behavior', [
                '`shuffle(random_state)` is the row-order helper for the common case where you want to permute rows and leave the feature order alone.',
                'It is equivalent to `reshuffle(axis=0, ...)`, but reads more directly in train/test split and stochastic preview workflows.',
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
        if class_name == 'DataFrame' and n == 'caselbl':
            return bullets('Practical Behavior', [
                '`caselbl()` returns the current case axis exactly as it is stored on the frame.',
                'The case axis can be numeric, note-based, timeline-based, or time-index based.',
                'Use `case_study(...)` when you want row selection; use `caselbl()` when you want the row labels themselves.',
                'When you need numeric row labels as data, call `caselbl().as_series()` explicitly so the conversion stays visible.',
            ])
        if class_name == 'DataFrame' and n == 'ceil_time':
            return bullets('Practical Behavior', [
                '`ceil_time(...)` rounds selected temporal features upward to the requested unit.',
                'Only timeline/timeindex features are valid targets.',
                'Use this when you want time-bucket labels that always round toward the next boundary.',
            ])
        if class_name == 'DataFrame' and n == 'case_study':
            return []
        if class_name == 'DataFrame' and n == 'catalogue':
            return bullets('Encoding Rules', [
                '`catalogue(feature)` returns explicit `codes`, `levels`, and `ordered` fields instead of hiding codes inside an ordinary numeric feature.',
                'Missing source values keep an invalid mask and use the conventional payload code `-1`.',
                'Levels preserve first-seen order by default; `sort_levels=true` assigns codes from sorted levels.',
                '`encoding.decode()` reconstructs the typed Thread and its validity mask.',
            ])
        if class_name == 'DataFrame' and n == 'observe':
            return bullets('Practical Behavior', [
                '`observe(...)` returns a `DataFrameView`, so it is the preview-oriented row window helper alongside `head(...)` and `tail(...)`.',
                'It uses positional row coordinates, not case-label lookup.',
                'Use `observe(start, stop).show()` when you want a middle slice without materializing a bespoke helper or mutating the source frame.',
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
        if class_name == 'DataFrame' and n == 'conform':
            return bullets('Practical Behavior', [
                '`conform(other)` first reorders features to match `other.features()` and then aligns the case axis to `other.label().values()`.',
                'Both frames must have the same feature count and the same feature name set.',
                'Use this before arithmetic or updates when structure compatibility matters more than preserving the current order.',
            ])
        if class_name == 'DataFrame' and n == 'copy':
            return bullets('Practical Behavior', [
                '`copy()` gives you a detached frame value for downstream edits.',
                'Use it when you want to make the independence of the result explicit in user code or examples.',
            ])
        if class_name == 'DataFrame' and n == 'tail':
            return bullets('Practical Behavior', [
                '`tail(n)` preserves the original feature order and returns the last `n` rows.',
                'If `n` is larger than the frame height, the whole frame is returned.',
                'The returned object is a `DataFrameView`, so it behaves like a lightweight preview slice rather than a deep copy.',
            ])
        if class_name == 'DataFrame' and n == 'tabelize':
            return bullets('Practical Behavior', [
                '`catalog.tabelize()` is the explicit bridge from Kindle stream collections into the DataFrame model.',
                'Aligned streams become DataFrame features, and stream position becomes row position.',
                'Use the policy overload when you want explicit tabelization rules instead of the defaults.',
            ])
        if class_name == 'DataFrame' and n in {'title', 'title_'}:
            return bullets('Practical Behavior', [
                '`title()` with no argument reads the current frame title.',
                '`title("...")` returns a titled copy and leaves the source frame unchanged.',
                '`title_("...")` mutates the current frame title in place.',
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
                '`dropnull` controls whether null should participate as its own distinct value.',
            ])
        if class_name == 'DataFrame' and n == 'count_null_rows':
            return bullets('Practical Behavior', [
                '`count_null_rows()` counts how many rows contain at least one null value anywhere in the frame.',
                'Use this as a quick completeness check before grouping, joining, or export.',
            ])
        if class_name == 'DataFrame' and n == 'bucketize':
            return bullets('Practical Behavior', [
                '`bucketize(...)` delegates to thread-level bucketization only for the selected numeric `Series` features.',
                'Non-selected features pass through unchanged, so this is safe to use on mixed-type frames.',
                'The no-feature overload auto-selects every numeric `Series` feature in the frame.',
                'Bucket labels are returned as Int64-backed `Thread` columns and out-of-range values remain null.',
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
        if class_name == 'DataFrame' and n == 'dropnull':
            return bullets('Practical Behavior', [
                '`dropnull()` removes rows that contain at least one null value anywhere in the frame.',
                'Use `dropna_all(features)` when the rule should apply only to selected features and only when all of them are null.',
            ])
        if class_name == 'DataFrame' and n == 'dropna_all':
            return bullets('Practical Behavior', [
                '`dropna_all(features)` removes rows only when all selected features are null in that row.',
                'Rows with at least one observed value among the selected features are kept.',
            ])
        if class_name == 'DataFrame' and n == 'load':
            return bullets('Practical Behavior', [
                '`load(...)` is the format-switched entry point behind the explicit file loaders.',
                'Use it when the source format is chosen at runtime; otherwise, the format-specific loader is clearer.',
                'CSV-style overloads let you control separator, row limits, feature projection, case-axis promotion, and bad-line policy.',
            ])
        if class_name == 'DataFrame' and n in {'load_csv', 'load_excel', 'load_json', 'load_parquet', 'load_txt'}:
            return bullets('Practical Behavior', [
                f'`{n}(...)` loads one external source into typed PlussMic columns.',
                'Positional overloads are quick for ad-hoc work, while option/config overloads are better when you want repeatable loader settings.',
                'Excel multi-sheet overloads return `pluss::loader::LoadResult` because several frames may be produced at once.',
            ])
        if class_name == 'DataFrame' and n == 'lt':
            return bullets('Comparison Rules', [
                '`lt(...)` returns a boolean-style frame with the same case axis and selected feature layout as the input.',
                'Scalar/native comparisons broadcast the right-hand value across every compatible numeric cell.',
                'Frame-vs-frame comparisons are row-wise and require compatible shape and feature alignment.',
            ])
        if class_name == 'DataFrame' and n == 'matrix':
            return bullets('Practical Behavior', [
                '`matrix()` does not coerce every feature into one mixed block.',
                'Instead, it packs features by payload family so numeric, note, timeline, and timeindex data stay type-stable.',
                'Use `as_series()`, `as_note()`, `as_timeline()`, or `as_timeindex()` only when the frame contributes exactly one block of that family.',
            ])
        if class_name == 'DataFrame' and n in {'max', 'mean', 'median', 'min', 'std', 'sum', 'var'}:
            return bullets('Practical Behavior', [
                f'`{n}(...)` reduces the selected numeric features to one summary row.',
                'Non-numeric features are skipped rather than coerced.',
                'The output stays in DataFrame form, which makes these reducers easy to reuse in downstream frame pipelines.',
            ])
        if class_name == 'DataFrame' and n in {'notna', 'notnull'}:
            return bullets('Practical Behavior', [
                f'`{n}(...)` returns a boolean-style frame marking which cells are observed rather than missing.',
                'Use it to build masks before `filter(...)`, replacement policies, or diagnostic summaries.',
                '`notna(...)` and `notnull(...)` are equivalent.',
            ])
        if class_name == 'DataFrame' and n == 'place':
            return bullets('Practical Behavior', [
                '`place(name, feature, anchor, after)` inserts a brand-new feature relative to an existing anchor.',
                '`place(name, anchor, after)` moves an existing feature without forcing you to restate the full feature order.',
                'When `after=false`, placement happens before the anchor. When `after=true`, placement happens after the anchor.',
            ])
        if class_name == 'DataFrame' and n in {'put', 'put_'}:
            return bullets('Practical Behavior', [
                f'`{n}(feature, mask, value)` rewrites only the rows where the mask evaluates true.',
                'The mask must match the frame case count.',
                'Use `put(...)` when you want a new frame result, and `put_(...)` when you want the same rewrite in place.',
            ])
        if class_name == 'DataFrame' and n == 'prod':
            return bullets('Axis Rules', [
                '`prod(render=false)` returns a compact summary view with one product per numeric feature.',
                '`prod(axis=0)` returns a numeric thread with one product per numeric feature.',
                '`prod(axis=1)` returns a numeric thread with one product per row across numeric features.',
            ])
        if class_name == 'DataFrame' and n == 'query':
            return bullets('Practical Behavior', [
                '`query(expression)` keeps rows where the expression evaluates true.',
                'Feature names are referenced directly inside the expression string.',
                'String comparisons should use quoted values, especially when feature names contain spaces.',
            ])
        if class_name == 'DataFrame' and n == 'top':
            return bullets('Practical Behavior', [
                '`top(feature, n)` sorts descending by the selected feature, keeps nulls last, then returns the first `n` rows.',
                'The output rows stay aligned across every feature; this is not a single-column extraction.',
                'Use `bottom(...)` when you want the smallest rows instead of the largest ones.',
            ])
        if class_name == 'DataFrame' and n == 'reorder':
            return bullets('Practical Behavior', [
                '`reorder({"a", "b"})` now means “move these features together and preserve the relative order of everything else.”',
                'Use `ReorderSpec` when you need to move a subset to the front, back, before an anchor, or after an anchor.',
                'Use the multi-spec overload when you want to apply several moves in one call from left to right.',
                'This avoids listing a 3000-feature schema just to reposition two columns.',
            ])
        if class_name == 'DataFrame' and n == 'sample':
            return bullets('Practical Behavior', [
                '`sample(n, ...)` draws a fixed number of rows by default, or features when `axis=1`.',
                '`sample(frac, ...)` draws a proportion of the selected axis.',
                '`replace=true` enables sampling with replacement. For `frac > 1`, replacement is required.',
            ])
        if class_name == 'DataFrame' and n == 'replace':
            return bullets('Practical Behavior', [
                '`replace(from, to, features)` dispatches by value type, so numeric replacements touch numeric series features while string replacements touch note features.',
                'If `features` is empty, every compatible feature of the relevant kind participates automatically.',
                'Use `pluss::null` as `to` when you want matched cells to become missing.',
            ])
        if class_name == 'DataFrame' and n == 'rolling':
            return bullets('Practical Behavior', [
                '`rolling(...)` is the frame-level window-engineering route, so one call can derive several rolling outputs at once.',
                '`keep_input=true` appends rolling outputs to the original frame, while `keep_input=false` returns only the engineered outputs.',
                'Each `WindowSpec` controls the output name, source feature, operation, window width, and minimum observation rule.',
            ])
        if class_name == 'DataFrame' and n == 'transform':
            return bullets('Practical Behavior', [
                '`transform(fn)` rewrites every feature with the same thread-level function.',
                '`transform(feature, thread_fn)` rewrites one existing feature in place within the returned frame.',
                '`transform(feature, frame_fn)` is the derive-style overload: it can create a new feature from the whole frame, as long as the returned thread matches the frame case count.',
            ])
        if class_name == 'DataFrame' and n == 'select_like':
            return bullets('Practical Behavior', [
                f'`{n}(...)` filters feature names, not row values.',
                'The output keeps the original row order and case axis unchanged.',
                'Use this when you want schema-driven feature selection without listing every matching feature manually.',
            ])
        if class_name == 'DataFrame' and n == 'sql':
            return bullets('Practical Behavior', [
                '`sql()` returns the SQL accessor; it does not execute a statement by itself.',
                'Call `.execute(...)` on the accessor when you want a result frame immediately.',
                'Use `prepare_sql(...)` when the same query will be executed repeatedly with changing bindings.',
            ])
        if class_name == 'DataFrame' and n == 'truediv':
            return bullets('Practical Behavior', [
                'Scalar `truediv(...)` divides every compatible numeric cell by the supplied scalar.',
                'Frame-vs-frame `truediv(...)` divides aligned numeric features row by row.',
                'Non-numeric features pass through unchanged in the scalar form and are excluded from numeric combination work.',
            ])
        if class_name == 'DataFrame' and n == 'ts':
            return bullets('Practical Behavior', [
                '`ts(...)` does not materialize a new DataFrame by itself; it returns the time-series accessor bound to the selected features.',
                'Use the accessor when you want downstream resampling, lag blocks, rolling blocks, or calendar features on those bound features.',
            ])
        if class_name == 'DataFrame' and n == 'unique_cases':
            return bullets('Practical Behavior', [
                '`unique_cases(features)` keeps only the first row for each distinct key combination defined by `features`.',
                'With no feature list, the full row payload is used as the duplicate key.',
                'This is the row-filtering companion to `label().repeated(...)`.',
            ])
        if class_name == 'DataFrame' and n == 'unify':
            return bullets('Join Semantics', [
                '`unify(...)` matches rows by key values, not by current row order.',
                '`common`, `keep_left`, `keep_right`, and `unite` retain the corresponding key set while preserving stable input order.',
                '`left_only` and `right_only` select unmatched keys; `cross` creates the cartesian case product without key features.',
                'Repeated keys expand to every matching pair, and `validate` can enforce one-to-one or one-sided uniqueness.',
                'When right-hand non-key names collide with left-hand names, `right_suffix` is applied only to the right-hand collisions.',
            ])
        if class_name == 'DataFrame' and n == 'extend':
            return bullets('Extension Rules', [
                '`extend(other)` appends cases vertically and matches features by name.',
                'The default `Unite` feature rule preserves left feature order, appends right-only features, and null-fills one-sided payloads.',
                'Matching numeric features must have the same dtype and platform; other matching features must have the same Thread kind.',
                'Use `fresh_cases=true` for contiguous integer cases or `reject_repeated_cases=true` to require a unique preserved case axis.',
            ])
        if class_name == 'DataFrame' and n == 'unsqueeze':
            return bullets('Practical Behavior', [
                '`unsqueeze()` wraps one frame into a depth-1 `Atlas`.',
                'The `depth_labels` overload lets you attach exactly one depth label to that one-frame atlas.',
                'This is useful when downstream APIs expect atlas depth even though you currently have only one frame.',
            ])
        if class_name == 'DataFrame' and n == 'view':
            return bullets('Slice Semantics', [
                '`view(start, stop)` behaves like a positional row slice `[start, stop)`.',
                'Negative `start` or `stop` values count backward from the end of the frame.',
                'Unlike `slice(...)`, `view(...)` returns a `DataFrameView` wrapper intended for lightweight inspection.',
            ])
        if class_name == 'DataFrame' and n == 'where':
            return bullets('Practical Behavior', [
                '`where(mask)` keeps rows where the boolean mask is true and drops the rest.',
                'The mask length must match the frame case count exactly.',
                'Use `put(...)` when you want masked replacement inside one feature instead of masked row filtering.',
            ])
        if class_name == 'DataFrame' and n == 'window':
            return bullets('Practical Behavior', [
                '`window(...)` engineers one or more derived features from rolling/window operations described by `WindowSpec`.',
                '`keep_input=false` returns only the engineered outputs, which is useful for model-matrix style pipelines.',
                '`rolling(...)` is the alias that delegates to the same frame-level implementation.',
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
        if class_name == 'Thread' and n == 'caselbl':
            return bullets('Practical Behavior', [
                '`caselbl()` returns the current thread label axis exactly as stored.',
                'The setter overload returns a relabeled copy while preserving payload values and the validity mask.',
                'Use this when you want to inspect or rewrite row labels on one thread directly.',
            ])
        if class_name == 'Thread' and n in {'clear_caselbl', 'clear_caselbl_'}:
            return bullets('Practical Behavior', [
                f'`{n}()` removes explicit case labels from the thread without rewriting the payload values.',
                'Use the underscore form when the label removal should happen in place.',
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
                '`distinct(dropnull)` keeps the first occurrence of each distinct value in encounter order.',
                'With `dropnull=false`, at most one null entry is retained.',
            ])
        if class_name == 'Thread' and n == 'dropnull':
            return bullets('Practical Behavior', [
                '`dropnull()` removes invalid entries from the thread and compacts the result.',
                'Use this when null rows should disappear rather than be filled.',
            ])
        if class_name == 'Thread' and n == 'dtype':
            return bullets('Practical Behavior', [
                '`dtype()` reports the scalar dtype used by a numeric series payload.',
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
        if class_name == 'Stream' and n == 'dropnull':
            return bullets('Practical Behavior', [
                '`dropnull()` removes invalid entries from the stream while preserving stream semantics for the remaining entries.',
                'Use it when rows with null payload values should be removed entirely.',
            ])
        if class_name == 'Catalog' and n == 'at':
            return bullets('Practical Behavior', [
                '`at(key)` fetches one named stream from the catalog.',
                'Use it when you want stream access without routing through table conversion first.',
            ])
        if class_name == 'Catalog' and n in {'apply', 'map'}:
            return bullets('Practical Behavior', [
                '`apply(...)` rewrites one named stream; `map(...)` rewrites every stream.',
                'The transform must return a defined stream with the same length as the input stream.',
                'Use the underscore form when you want the mutation in place.',
            ])
        if class_name == 'Catalog' and n == 'astype':
            return bullets('Conversion Rules', [
                '`keys({...}).astype(dtype)` is the catalog-level numeric-series conversion helper.',
                'It delegates to stream/thread conversion on the selected keys only.',
            ])
        if class_name == 'Catalog' and n == 'featurize':
            return bullets('Practical Behavior', [
                '`featurize(field)` adds `<field>_char_len` and `<field>_word_count` streams.',
                'The source field is coerced through note/text handling before the text statistics are computed.',
            ])
        if class_name == 'Catalog' and n == 'stats':
            return bullets('Practical Behavior', [
                '`stats(field, stat_type)` collapses one text field into a one-value summary stream.',
                'Supported statistics include common length-based summaries such as mean/min/max/count, along with vocabulary size.',
            ])
        if class_name == 'Catalog' and n == 'cast':
            return bullets('Conversion Rules', [
                'Catalog cast accessors rewrite only the selected keys.',
                'Unknown keys throw immediately.',
            ])
        if class_name == 'Catalog' and n == 'cloc':
            return bullets('Practical Behavior', [
                '`cloc(...)` slices aligned row positions across every stream in the catalog.',
                'Use it when you want row-wise positional access while keeping stream alignment intact.',
            ])
        if class_name == 'Catalog' and n == 'similarity':
            return bullets('Practical Behavior', [
                '`similarity(field1, field2, "cosine")` computes row-wise text similarity and appends the result as a numeric stream.',
                'At the moment the supported similarity method is cosine.',
                'Both fields must have the same length.',
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
        if class_name == 'Catalog' and n == 'dropnull':
            return bullets('Practical Behavior', [
                '`keys({...}).dropnull()` applies stream-level `dropnull()` to the selected keys.',
                'It preserves the surrounding catalog structure while rewriting only those streams.',
                'Unknown keys throw immediately.',
            ])
        if class_name == 'Catalog' and n == 'keywords':
            return bullets('Practical Behavior', [
                '`keywords(field, top_n)` tokenizes the text field, counts tokens, sorts by frequency descending then alphabetically, and returns a small keyword/count catalog.',
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
        if class_name == 'Registry' and n in {'apply', 'map'}:
            return bullets('Practical Behavior', [
                '`apply(...)` rewrites one slot or a selected slot set; `map(...)` rewrites every slot.',
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
        if class_name == 'Registry' and n == 'concat':
            return bullets('Practical Behavior', [
                '`concat(key, other)` appends the values from another slot onto one existing slot.',
                'The target key must exist and the appended slot must be defined.',
            ])
        if class_name == 'Registry' and n == 'drop':
            return bullets('Practical Behavior', [
                '`drop(...)` removes named slots, not individual values.',
                'Unknown keys throw immediately.',
            ])
        if class_name == 'Registry' and n in {'prune', 'prune_'}:
            return bullets('Practical Behavior', [
                'These methods remove slots whose payload is empty.',
                'Non-empty slots are passed through unchanged.',
            ])
        if class_name == 'Axel' and n == 'apply_slot':
            return bullets('Practical Behavior', [
                '`apply_slot(key, fn)` walks every registry in the axel and rewrites that slot only where it exists.',
                'Registries that do not have the requested key are left unchanged.',
            ])
        if class_name == 'Axel' and n == 'as_kindle':
            return bullets('Bridge Behavior', [
                '`as_kindle()` converts the axel into the Archive model.',
                'Use this when the next workflow is archive/catalog based rather than bundle based.',
            ])
        if class_name == 'Axel' and n == 'concat_depth':
            return bullets('Practical Behavior', [
                '`concat_depth(other)` appends the other axel’s registries onto the current depth axis.',
                'If both axels have labels, the labels are concatenated too.',
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
        if class_name == 'DataFramePlotAccessor' and n == 'step':
            return bullets('Practical Behavior', [
                '`step(...)` uses the same feature selection pattern as `line(...)`, but the trace changes by steps instead of straight segments.',
                'This is useful when the y-value should be interpreted as holding until the next x position.',
                'If `x_feature` is omitted, the case axis is used.',
            ])
        if class_name == 'DataFramePlotAccessor' and n == 'scatter':
            return bullets('Practical Behavior', [
                '`scatter(x_feature, y_feature, ...)` plots one point per row.',
                'Use `hue_feature` when rows should be color-grouped by a categorical label.',
                'This method is best when the relationship between two measures matters more than their order.',
            ])
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
        if class_name == 'DataFramePlotAccessor' and n == 'hist':
            return bullets('Practical Behavior', [
                '`hist(...)` bins numeric values into frequency bars.',
                'With several features, the selected features are rendered together in one figure for quick distribution comparison.',
                'Use the options overload when you want explicit visual controls instead of just the bin count.',
            ])
        if class_name == 'DataFramePlotAccessor' and n == 'heatmap':
            return bullets('Practical Behavior', [
                'The direct `heatmap(features)` form renders the selected numeric features as a matrix over the case axis.',
                'The grouped `heatmap(case_feature, feature_feature, value_feature, ...)` form first aggregates long-form rows into a two-dimensional summary grid.',
                'Use `annotate_levels=true` in the options when you want each cell to display its numeric value.',
            ])
        if class_name == 'DataFramePlotAccessor' and n == 'line':
            return bullets('Practical Behavior', [
                '`line(...)` draws one continuous trace per selected y-feature.',
                'Pass `x_feature` when the horizontal axis should come from a real feature instead of the case axis.',
                'Use the options overload when you want to tune line alpha, width, or y-limit policy.',
            ])
        if class_name == 'DataFramePlotAccessor' and n == 'lag':
            return bullets('Practical Behavior', [
                '`lag(feature, options)` compares one numeric feature against a lagged copy of itself.',
                'Increase `LagPlotOptions::lag` when you want to inspect longer-gap autocorrelation structure.',
                'This is useful for quick time-dependence diagnostics before formal modeling.',
            ])
        if class_name == 'DataFramePlotAccessor' and n == 'parallel':
            return bullets('Practical Behavior', [
                '`parallel(...)` expects numeric features and draws one polyline per row across several aligned axes.',
                'Use `class_feature` when rows should be colored by a grouping label.',
                'This is useful for comparing multivariate profiles row by row.',
            ])
        if class_name == 'DataFramePlotAccessor' and n == 'pie':
            return bullets('Practical Behavior', [
                '`pie(value_feature, label_feature, ...)` uses one row per slice.',
                'The value feature determines slice magnitude, and the label feature controls the text shown for each slice.',
                'This is best for small categorical summaries rather than large row counts.',
            ])
        if class_name == 'DataFramePlotAccessor' and n == 'radviz':
            return bullets('Practical Behavior', [
                '`radviz(...)` expects numeric features and projects each row toward feature anchors around a circle.',
                'Use `class_feature` when you want point colors to reflect group labels.',
                'This is useful for quick multivariate separation checks between classes.',
            ])
        if class_name == 'DataFramePlotAccessor' and n == 'scatter_matrix':
            return bullets('Practical Behavior', [
                '`scatter_matrix(...)` creates pairwise scatter panels for the selected numeric features.',
                'Use the `triangle` overload when you want only the upper, lower, or corner half of the matrix.',
                'Use the options overload when you want finer pair-plot rendering control.',
            ])
        if class_name == 'DataFramePlotAccessor' and n == 'violin':
            return bullets('Practical Behavior', [
                'The feature-list overload renders one violin per numeric feature.',
                'The `(value_feature, group_feature)` overload renders one violin per group in the grouping feature.',
                'Use this when you want full distribution shape rather than only quartiles or whiskers.',
            ])
        if class_name == 'ColumnRef' and n == 'thread':
            return bullets('Practical Behavior', [
                '`thread()` gives you the exact referenced column payload without copying it into a new frame.',
                'Use this when you want to switch into `Thread` operations such as comparisons, casting, or text cleanup.',
            ])
        if class_name == 'ColumnRef' and n == 'value':
            return bullets('Practical Behavior', [
                '`value()` returns a `ThreadValue` wrapper over the first element of the referenced column.',
                'Use this when you need typed scalar extraction rather than a whole-column thread.',
            ])
        if class_name == 'ColumnRef' and n == 'values':
            return bullets('Practical Behavior', [
                '`values()` materializes the referenced feature as a standalone `Thread`.',
                'Use this when you want to pass the column into another API that expects a thread rather than a column reference.',
            ])
        if class_name == 'DataFrameSqlAccessor' and n == 'execute':
            return bullets('Practical Behavior', [
                '`execute(statement)` runs the SQL immediately against the current frame.',
                'The current frame is exposed inside the statement as `self`.',
                'Use this for one-off queries. Use `prepare(...)` when the same statement shape will be executed repeatedly with different parameter values.',
            ])
        if class_name == 'DataFrameSqlAccessor' and n == 'prepare':
            return bullets('Practical Behavior', [
                '`prepare(statement)` captures a parameterized SQL template and returns a reusable prepared helper.',
                'Use placeholders such as `{min_sales}` for value parameters you will bind later.',
                'Prepared execution is the better path when you want repeated queries without rebuilding the statement each time.',
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
        cfg['intro'],
        '',
        '## How To Use This Page',
        '',
        '- Methods are grouped alphabetically so you can jump quickly from the left rail.',
        '- Each method page section starts with the public signatures, then explains what the method does, how each parameter changes behavior, and what the call returns.',
        '- Overloaded methods include several examples so you can see the supported call shapes instead of reverse-engineering them from signatures alone.',
        '- Each example is followed immediately by verified output captured from the current compiled examples so the result shape is visible before you move to the next overload.',
        '',
    ]
    lines.extend(class_specific_intro_sections())
    if True:
        names_by_letter: dict[str, list[str]] = {}
        for name in sorted(grouped.keys(), key=str.lower):
            letter = name[0].upper()
            names_by_letter.setdefault(letter, []).append(name)
        ordered_names = [name for letter in sorted(names_by_letter.keys()) for name in names_by_letter[letter]]
        prev_next_by_name: dict[str, tuple[str | None, str | None]] = {}
        for idx, name in enumerate(ordered_names):
            prev_name = ordered_names[idx - 1] if idx > 0 else None
            next_name = ordered_names[idx + 1] if idx < len(ordered_names) - 1 else None
            prev_next_by_name[name] = (prev_name, next_name)

        def merged_params(overloads: list[Method]) -> list[tuple[str, str]]:
            seen: set[str] = set()
            out: list[tuple[str, str]] = []
            for overload in overloads:
                for name, typ in overload.params:
                    if name not in seen:
                        seen.add(name)
                        out.append((name, typ))
            return out

        def merged_returns(overloads: list[Method]) -> str:
            seen: list[str] = []
            for overload in overloads:
                rendered = returns_doc(overload.return_type)
                if rendered not in seen:
                    seen.append(rendered)
            if len(seen) == 1:
                return seen[0]
            return '\n\n'.join(seen)

        for letter in sorted(names_by_letter.keys()):
            lines.append(f'## {letter}')
            lines.append('')
            for name in names_by_letter[letter]:
                overloads = sorted(grouped[name], key=lambda m: user_signature(m, class_name).lower())
                prev_name, next_name = prev_next_by_name[name]
                structured_content = load_method_content(class_name, name)
                if structured_content is not None:
                    lines.extend(render_structured_method_page(class_name, name, overloads, structured_content))
                    lines.extend(render_method_prev_next(prev_name, next_name))
                    lines.append('')
                    continue
                custom_page = render_dataframe_a_page(name, overloads, class_name)
                if custom_page is None:
                    custom_page = render_thread_a_page(name, overloads, class_name)
                if custom_page is None:
                    custom_page = render_dataframe_b_page(name, overloads, class_name)
                if custom_page is None:
                    custom_page = render_thread_b_page(name, overloads, class_name)
                if custom_page is not None:
                    lines.extend(custom_page)
                    lines.extend(render_method_prev_next(prev_name, next_name))
                    lines.append('')
                    continue
                if class_name == 'DataFrame' and name == 'case_study':
                    lines.extend(render_case_study_page(overloads, class_name))
                    lines.extend(render_method_prev_next(prev_name, next_name))
                    lines.append('')
                    continue
                lines.append(f'### `{name}`')
                lines.append('')
                lines.append('#### Signatures' if len(overloads) > 1 else '#### Signature')
                lines.append('')
                lines.append('```cpp')
                for idx, overload in enumerate(overloads):
                    if idx:
                        lines.append('')
                    lines.append(user_signature(overload, class_name))
                lines.append('```')
                lines.append('')
                lines.append('#### What It Does')
                lines.append('')
                note = generic_note(class_name, overloads[0]).strip()
                lines.append(note or 'Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.')
                lines.append('')
                lines.append('#### Parameters')
                lines.append('')
                lines.append(params_doc(merged_params(overloads), class_name, overloads[0].name))
                lines.append('')
                lines.append('#### Returns')
                lines.append('')
                lines.append(merged_returns(overloads))
                lines.append('')
                rendered_any = False
                example_count = 0
                for overload in overloads:
                    code = normalize_user_names(class_name, example_for(class_name, overload)).strip()
                    if not code:
                        continue
                    example_count += 1
                    if not rendered_any:
                        lines.append('#### Examples')
                        lines.append('')
                        rendered_any = True
                    lines.append(f'Example {example_count}. {example_caption(class_name, overload)}')
                    lines.append('')
                    lines.append('```cpp')
                    lines.append(code)
                    lines.append('```')
                    preview = normalize_user_names(class_name, output_for(class_name, overload)).strip()
                    lines.append('')
                    lines.append('##### Output')
                    lines.append('')
                    lines.append('```text')
                    lines.append(preview or 'The call completes and returns the contract described under Output.')
                    lines.append('```')
                extra = extra_sections_for_method(overloads[0])
                if extra:
                    if lines[-1] != '':
                        lines.append('')
                    lines.extend(extra)
                lines.extend(render_method_prev_next(prev_name, next_name))
    return '\n'.join(lines)


def extract_method_chunks(full_page: str) -> dict[str, str]:
    chunks: dict[str, list[str]] = {}
    current_name: str | None = None
    current_lines: list[str] = []
    for line in full_page.splitlines():
        method_match = re.match(r'^### `([^`]+)`$', line)
        if method_match:
            if current_name is not None:
                chunks[current_name] = current_lines
            current_name = method_match.group(1)
            current_lines = [line]
            continue
        if current_name is None:
            continue
        if re.match(r'^## [A-Z]$', line):
            continue
        current_lines.append(line)
    if current_name is not None:
        chunks[current_name] = current_lines
    return {name: '\n'.join(lines).strip() for name, lines in chunks.items()}


def build_method_index_page(class_name: str, methods: list[Method]) -> str:
    cfg = CLASS_CONFIG[class_name]
    ordered_names = ordered_method_names(methods)

    lines = [
        f'# {cfg["title"]}',
        '',
        cfg['intro'],
        '',
        '## Using This Reference',
        '',
        '- Use the left sidebar to open one method page at a time.',
        '- Each method page contains its own signatures, parameter details, notes, verified outputs, and working previous/next navigation.',
        '- This landing page stays intentionally short so the content panel is not one giant scroll of every method.',
        '',
    ]
    lines.extend([
        '## Method Pages',
        '',
        'The sidebar lists methods alphabetically. You can also use the search box to jump directly to a method name.',
        '',
    ])

    children = ACCESSOR_CHILDREN.get(class_name, [])
    if class_name == 'Registry':
        lines.extend([
            '## Accessors',
            '',
            '`Registry` exposes:',
            '',
            '- `fields(keys)` -> `Registry` projection',
            '- `fieldaxis()` -> [`RegistryFieldAxisAccessor`](registry_fieldaxis_accessor_api_reference.md)',
            '- `cast()`, `nt()`, `tl()` -> nested accessors over the current registry',
            '',
        ])
    elif class_name == 'Axel':
        lines.extend([
            '## Accessors',
            '',
            '`Axel` exposes:',
            '',
            '- `fields(keys)` -> `Axel` projection',
            '- `fieldaxis()` -> [`AxelFieldAxisAccessor`](axel_fieldaxis_accessor_api_reference.md)',
            '- `cast()`, `nt()`, `tl()` -> nested accessors over the current axel',
            '',
        ])
    elif children:
        lines.extend(['## Accessors', ''])
        lines.append(f'`{class_name}` exposes nested accessors, each with its own method pages and sidebar:')
        lines.append('')
        for child in children:
            child_cfg = CLASS_CONFIG[child]
            binder = child_cfg.get('binder', '')
            label = f'`{binder}`' if binder else f'`{child}`'
            lines.append(f"- {label} -> [`{child}`]({child_cfg['output'].stem}.md)")
        lines.append('')

    lines.extend([
        '```{toctree}',
        ':hidden:',
        ':maxdepth: 1',
        '',
    ])
    for name in ordered_names:
        lines.append(method_page_rel_from_index(class_name, name))
    lines.extend(['```', ''])
    return '\n'.join(lines)


def build_method_page_markdown(class_name: str,
                               method_name: str,
                               chunk: str,
                               prev_name: str | None,
                               next_name: str | None,
                               known_methods: dict[str, set[str]]) -> str:
    lines = chunk.splitlines()
    if lines and re.match(r'^### `[^`]+`$', lines[0]):
        lines = lines[1:]
    body = '\n'.join(lines).strip()
    body = strip_inline_prev_next(body)
    body = rewrite_method_links(body, current_class=class_name, known_methods=known_methods)
    body = re.sub(r'(?m)^####\s+', '## ', body)
    body = re.sub(r'(?m)^#####\s+', '### ', body)

    prev_link = method_page_html_rel_from_method(class_name, class_name, prev_name) if prev_name else None
    next_link = method_page_html_rel_from_method(class_name, class_name, next_name) if next_name else None

    out = [
        f'# {class_name}::{method_name}',
        '',
        f'[Back to {class_name} API Reference](../../{CLASS_CONFIG[class_name]["output"].stem})',
        '',
        body,
        '',
        '<div class="pluss-inline-prev-next">',
    ]
    if prev_name and prev_link:
        out.extend([
            f'  <a class="prev-link" href="{prev_link}">',
            '    <span class="label">Previous</span>',
            f'    <span class="title">{prev_name}</span>',
            '  </a>',
        ])
    if next_name and next_link:
        out.extend([
            f'  <a class="next-link" href="{next_link}">',
            '    <span class="label">Next</span>',
            f'    <span class="title">{next_name}</span>',
            '  </a>',
        ])
    out.extend(['</div>', ''])
    return '\n'.join(out)


HAND_AUTHORED_MARKER = '<!-- pluss-hand-authored -->'


def is_hand_authored(path: pathlib.Path) -> bool:
    """Return True if an existing per-method doc page has been promoted to
    hand-authored prose and must not be regenerated from the header.

    A page is marked hand-authored by putting HAND_AUTHORED_MARKER on its own
    line anywhere in the first few lines of the file (an HTML comment, so it
    renders as nothing in Sphinx/MyST output). Once a method's page carries
    this marker, `main()` below skips writing to it entirely -- the header
    parser still discovers the method (so it still appears in nav/toctree and
    the index page), but its prose, examples, and output are never touched.
    """
    if not path.exists():
        return False
    try:
        with path.open('r', encoding='utf-8') as fh:
            head = [next(fh, '') for _ in range(5)]
    except OSError:
        return False
    return any(HAND_AUTHORED_MARKER in line for line in head)


SIGNATURE_HASH_PREFIX = '<!-- pluss-signature-hash:'
SIGNATURE_HASH_RE = re.compile(r'<!-- pluss-signature-hash:\s*([0-9a-f]{12}) -->')


def compute_signature_hash(overloads: list[Method]) -> str:
    """Fingerprint every overload's full signature for one method name.

    Order-independent (sorted) so reordering overloads in the header doesn't
    manufacture false drift, but sensitive to any added/removed/changed
    overload, parameter, default value, or return type.
    """
    normalized = sorted(re.sub(r'\s+', ' ', m.signature).strip() for m in overloads)
    digest = hashlib.sha256('\n'.join(normalized).encode('utf-8')).hexdigest()
    return digest[:12]


def read_stored_signature_hash(path: pathlib.Path) -> str | None:
    if not path.exists():
        return None
    try:
        with path.open('r', encoding='utf-8') as fh:
            head = [next(fh, '') for _ in range(5)]
    except OSError:
        return None
    for line in head:
        m = SIGNATURE_HASH_RE.search(line)
        if m:
            return m.group(1)
    return None


def write_stored_signature_hash(path: pathlib.Path, new_hash: str) -> None:
    """Insert or update the `pluss-signature-hash` marker (2nd line, right
    after the `pluss-hand-authored` marker). Never touches anything else in
    the file -- this is the only function allowed to modify a hand-authored
    page, and only ever the hash comment.
    """
    text = path.read_text(encoding='utf-8')
    lines = text.split('\n')
    if any(SIGNATURE_HASH_RE.search(line) for line in lines[:5]):
        lines = [
            (f'{SIGNATURE_HASH_PREFIX} {new_hash} -->' if SIGNATURE_HASH_RE.search(line) else line)
            for line in lines
        ]
    else:
        # Insert right after the hand-authored marker line.
        for i, line in enumerate(lines[:5]):
            if HAND_AUTHORED_MARKER in line:
                lines.insert(i + 1, f'{SIGNATURE_HASH_PREFIX} {new_hash} -->')
                break
    path.write_text('\n'.join(lines), encoding='utf-8')


def group_methods_by_name(methods: list[Method]) -> dict[str, list[Method]]:
    by_name: dict[str, list[Method]] = {}
    for m in methods:
        by_name.setdefault(m.name, []).append(m)
    return by_name


def write_api_nav_manifest(extracted: dict[str, list[Method]]) -> None:
    """Emit one `PLUSS_API_NAV` entry per per-method class, driven entirely by
    `CLASS_CONFIG`/`METHOD_PAGE_DIR`/`PER_METHOD_CLASSES` -- adding a new
    per-method class (e.g. a new DataFrame accessor) only requires adding it
    to those three, never touching this function or layout.js by hand.

    Iterates `CLASS_CONFIG` (an ordinary, insertion-ordered dict) rather than
    `PER_METHOD_CLASSES` (a set) so the emitted JSON key order is stable
    across runs regardless of PYTHONHASHSEED.
    """
    payload = {}
    for class_name, cfg in CLASS_CONFIG.items():
        if class_name not in PER_METHOD_CLASSES:
            continue
        payload[class_name] = {
            'title': cfg.get('nav_title', class_name),
            'slug': METHOD_PAGE_DIR[class_name],
            'landing': f"{cfg['output'].stem}.html",
            'methods': ordered_method_names(extracted.get(class_name, [])),
        }
    for class_name, cfg in MANUAL_API_NAV.items():
        payload[class_name] = {
            'title': cfg['title'],
            'slug': cfg['slug'],
            'landing': cfg['landing'],
            'methods': manual_nav_methods(cfg['index']),
        }
    API_NAV_JS.write_text(f'window.PLUSS_API_NAV = {json.dumps(payload, indent=2)};\n')


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description='Regenerate templated per-method API reference pages, without ever '
                    'overwriting pages that have been promoted to hand-authored prose. '
                    'Detects when a hand-authored page\'s C++ signature has drifted from '
                    'what its prose was last verified against.')
    parser.add_argument(
        '--accept', action='append', default=[], metavar='ClassName::method',
        help='Re-baseline a hand-authored page\'s stored signature hash to its current '
             'header signature, after you have manually verified the prose still matches. '
             'Repeatable. Example: --accept DataFrame::join')
    parser.add_argument(
        '--check', action='store_true',
        help='CI mode: do the normal regen/preserve pass, but exit non-zero if any '
             'hand-authored page has drifted from its stored signature hash. Nothing is '
             'ever auto-fixed by --check; use --accept locally once you have updated the '
             'prose.')
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(sys.argv[1:] if argv is None else argv)
    accept_set = set(args.accept)

    extracted: dict[str, list[Method]] = {}
    skipped_classes: set[str] = set()
    for class_name, cfg in CLASS_CONFIG.items():
        try:
            methods = extract_methods(class_name, cfg['header'])
        except (FileNotFoundError, RuntimeError) as exc:
            print(f'[generate_api_reference] skipping {class_name}: {exc}', file=sys.stderr)
            skipped_classes.add(class_name)
            continue
        methods = [m for m in methods if m.name not in {'begin', 'end'} or class_name in {'Catalog', 'Registry'}]
        if class_name == 'Stream':
            # nt()/tl()/ts()/plot() are entry points into StreamNoteAccessor/
            # StreamTimelineAccessor/StreamTimeSeriesAccessor/StreamPlotAccessor --
            # each too large (or, for plot(), not verifiable via stdout capture) to
            # document in this phase. Deliberately out of scope, same as Catalog's
            # equivalent CatalogNoteAccessor/CatalogTimelineAccessor/
            # CatalogTimeSeriesAccessor entry points never getting pages at all.
            methods = [m for m in methods if m.name not in {'nt', 'tl', 'ts', 'plot'}]
        extracted[class_name] = methods

    write_api_nav_manifest(extracted)

    known_methods = {class_name: set(ordered_method_names(methods)) for class_name, methods in extracted.items()}
    all_drifted: list[str] = []

    for class_name, cfg in CLASS_CONFIG.items():
        if class_name in skipped_classes:
            continue
        methods = extracted[class_name]
        try:
            if class_name in PER_METHOD_CLASSES:
                full_page = build_page(class_name, methods)
                chunks = extract_method_chunks(full_page)
                ordered_names = ordered_method_names(methods)
                by_name = group_methods_by_name(methods)
                page_root = method_page_root(class_name)
                page_root.mkdir(parents=True, exist_ok=True)
                live_pages = {method_page_doc_path(class_name, name).name for name in ordered_names}
                for stale in page_root.glob('*.md'):
                    if stale.name not in live_pages and not is_hand_authored(stale):
                        stale.unlink()
                skipped_hand_authored = []
                written = 0
                for idx, name in enumerate(ordered_names):
                    prev_name = ordered_names[idx - 1] if idx > 0 else None
                    next_name = ordered_names[idx + 1] if idx < len(ordered_names) - 1 else None
                    chunk = chunks.get(name, '')
                    doc_path = method_page_doc_path(class_name, name)
                    if is_hand_authored(doc_path):
                        skipped_hand_authored.append(name)
                        current_hash = compute_signature_hash(by_name[name])
                        stored_hash = read_stored_signature_hash(doc_path)
                        qualified = f'{class_name}::{name}'
                        if qualified in accept_set:
                            write_stored_signature_hash(doc_path, current_hash)
                            print(f'  accepted new signature baseline for {qualified}')
                        elif stored_hash is None:
                            # First time this page carries the marker: adopt the
                            # current header signature as the verified baseline.
                            write_stored_signature_hash(doc_path, current_hash)
                        elif stored_hash != current_hash:
                            all_drifted.append(qualified)
                        continue
                    doc_path.write_text(
                        build_method_page_markdown(
                            class_name,
                            name,
                            chunk,
                            prev_name,
                            next_name,
                            known_methods,
                        )
                    )
                    written += 1
                if skipped_hand_authored:
                    print(
                        f'{class_name}: preserved {len(skipped_hand_authored)} hand-authored page(s): '
                        f'{", ".join(skipped_hand_authored)}'
                    )
                cfg['output'].write_text(build_method_index_page(class_name, methods))
                print(
                    f'{class_name}: wrote {written} method page(s), '
                    f'preserved {len(skipped_hand_authored)} -> {cfg["output"].name}'
                )
            else:
                cfg['output'].write_text(build_page(class_name, methods))
                print(f'{class_name}: wrote {len(methods)} methods -> {cfg["output"].name}')
        except Exception as exc:
            print(f'[generate_api_reference] skipping {class_name} generation: {exc}', file=sys.stderr)
            continue

    if all_drifted:
        print('')
        print('SIGNATURE DRIFT DETECTED -- the C++ signature changed since these')
        print('hand-authored pages were last verified. Their prose may now be stale:')
        for qualified in all_drifted:
            print(f'  - {qualified}')
        print('')
        print('Update the prose/examples for each page above, then re-run with')
        print('  --accept ClassName::method  (repeat per page)')
        print('to re-baseline the stored signature hash once verified.')
        if args.check:
            return 1

    return 0


if __name__ == '__main__':
    raise SystemExit(main())
