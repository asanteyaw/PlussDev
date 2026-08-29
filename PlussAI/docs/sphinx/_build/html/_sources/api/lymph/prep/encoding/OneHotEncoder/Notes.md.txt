# `lymph::prep::OneHotEncoder` Notes

## Layout Notes

The main value of this type is not "turn strings into numbers." It is "learn a
stable indicator layout once, then reuse it later."

That distinction matters whenever training data and inference data do not share
the exact same observed category set.

## Surface Notes

Use `prep::OneHotEncoder` when you want a reusable object. Use
`prep::stage::one_hot_encoder(...)` when the encoder should only live as one
descriptor inside a larger prep graph.

## Related Pages

- [Introduction](Introduction.md)
- [map](map.md)
- [summary](summary.md)
