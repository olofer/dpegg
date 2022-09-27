# dpegg
Unusually detailed egg drop programming

## Overview
Basic `C++` code with `Python` parser/plotter that explicitly generates optimal execution paths for the egg drop problem. In general the exact strategy is not unique. There is often wiggle room to improve the average number of drops required, for the same maximum number of drops. The average is defined as the mean number of drops taken across all possible (unknown) limit floors.

## Example 1
With $127$ floors there are a total of $128$ outcomes (egg breaks at any of the 127 floors, or not at all). The figure below shows the concentration of the floor access patterns (which floors the eggs are dropped) as the egg budget is increased. With $1$ egg, the search is linear from the bottom floor. For $7$ eggs or more, the search settles at a perfectly balanced binary search.

![Access patterns, 128 outcomes](/readme-figures/dpegg-out-127-access.png)

For this case, the minimum maximum number of drops is equal to the average number of drops when the egg budget is saturated. 

![Drops required, 128 outcomes](/readme-figures/dpegg-out-127-drops.png)

## Example 2
TBA.