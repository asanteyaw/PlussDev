# `lymph::prep::Route` Introduction

## What This Type Represents

`Route` is a declarative feature-selection object.

It is not the resolved answer itself. It is the reusable rule that later turns
into concrete feature positions when you call `resolve(...)` against a real
frame and an ordered feature list.

## Why It Exists

This type exists to separate three concerns cleanly:

- construction of the selection rule
- later resolution against a specific frame
- remainder policy attached to that rule

Without a real route object, those concerns collapse into repeated ad-hoc
column lists and one-off filtering logic.

## Related Pages

- [resolve](resolve.md)
- [with_remainder](with_remainder.md)
- [Notes](Notes.md)
