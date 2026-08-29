# `lymph::prep::Route` Notes

## Order Notes

Routes do not own a universal column order. They resolve against one.

That is why the explicit `resolve(frame, ordered_names)` overload matters: the
meaning of position `2` comes from the feature order you told the route to
respect.

## Complement Notes

When one route is the complement of another, prefer
`route::remaining_of(other_route)` over manually duplicating the complement
name list. It is harder to drift and easier to audit.

## Related Pages

- [Introduction](Introduction.md)
- [resolve](resolve.md)
- [with_remainder](with_remainder.md)
