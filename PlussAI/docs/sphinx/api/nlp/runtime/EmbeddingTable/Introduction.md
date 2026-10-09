# How The Table Turns Ids Into Vectors

The table is a pair of lookups. The token lookup fetches, for each id in a batch, the row of weights that
id owns. The position lookup fetches, for each position in a row, the row of position weights for that
position. The two are added, and the result is masked so padding contributes nothing.

Because the lookup is by id, two different words never share a row, and the vector for a word does not
depend on the document it appears in, only on its position. Position is what lets a model tell "dog bites
man" from "man bites dog".

Reset the table with the vocabulary size and pad id the indexer reported, and reset it again if the
vocabulary changes, since the rows are sized by the vocabulary.

## See Also

- [`reset`](reset.md) -- allocates the weights.
- [`forward`](forward.md) -- performs the lookup.

## Changelog

- **0.1** -- initial documented surface.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="dump_state_lines.html">
    <span class="label">Next</span>
    <span class="title">dump_state_lines</span>
  </a>
</div>
<!-- apiref:nav:end -->
