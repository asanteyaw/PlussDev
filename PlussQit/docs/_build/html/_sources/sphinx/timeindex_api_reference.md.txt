:orphan:

<!-- pluss-hand-authored -->

# TimeIndex Compatibility Reference

[Back to PlussQit API Reference](../api_reference.md)

`TimeIndex` remains public during the staged SDK migration so existing
PlussMic and application code continues to compile. Its ordered-axis methods
are now taught through the canonical
[Timeline Index Accessor API Reference](timeline_index_accessor_api_reference.md).

New code should construct a `Timeline`, call `timeline.index()`, and use the
returned accessor. The compatibility type and its factories forward to the
same implementation and will be removed only after downstream public
signatures no longer expose them.
