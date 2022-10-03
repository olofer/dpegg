# dpegg
Unusually detailed egg drop programming.

## Overview
Basic `C++` code (with `Python` parser/plotter demo) that explicitly generates optimal execution paths for the egg drop problem. In general the exact strategy is not unique. There may be wiggle room to improve the average number of drops required, for the same (minimized) maximum number of drops. The average is defined as the mean number of drops taken across all possible (unknown) limit floors.

## Basics

### Example 1
With $127$ floors there are a total of $128$ outcomes (egg breaks at any of the 127 floors, or not at all). The figure below shows the concentration of the floor access patterns (which floors the eggs are being dropped from) as the egg budget is increased. With $1$ egg, the search is linear from the bottom floor. With $2$ eggs, smaller linear searches are visible in-between certain spaced key floors. And so on. For $7$ eggs or more, the search settles at a perfectly balanced binary search.

![Access patterns, 128 outcomes](/readme-figures/dpegg-out-127-access.png)

For this case, the minimum maximum number of drops is equal to the average number of drops when the egg budget is saturated. Notice that $2^7=128$. 

![Drops required, 128 outcomes](/readme-figures/dpegg-out-127-drops.png)

### Example 2
With $100$ floors (i.e. $101$ outcomes) it is possible to find a dynamic programming solution which is skewed (the saturated binary search is not balanced). Maybe this allows a lower number of drops on average for the same number of maximum drops? At least it illustrates non-uniqueness of the exact search policy. The difference in average performance (compared to the symmetric case) might be of no real significance.

![Access patterns, 101 outcomes](/readme-figures/dpegg-out-100-access.png)

There is now a gap between the worst case and the average number of drops at saturation.

![Drops required, 101 outcomes](/readme-figures/dpegg-out-100-drops.png)

### Example 3
With $64$ floors, $7$ drops are still needed in worst case (note that $65 > 2^6$), but the average is almost $6$ drops. And the access pattern looks symmetric again.

![Access patterns 65](/readme-figures/dpegg-out-64-access.png) 

![Drops required 65](/readme-figures/dpegg-out-64-drops.png)

## Diagnosis
It turns out that there can be a wide range of equally optimal decisions. Here the decision cost surface is visualized for $F=100$ with a basket of $E=7$ unused eggs, assuming that the optimal decision is followed after this (first) drop is made.

![Decision surface example](/readme-figures/dpegg-dump-100-decision-7.png) 

This is to be explored further.

## Task status

- [x] Basic DP solver with tiebreak option
- [ ] Is search policy skewness maximum when right between powers of two?
- [ ] Can skewness be kicked the other way if tiebreaking searches from the end index? **sure looks so**
- [x] Visualize the "tiebreak function", it may look crazy
- [ ] More efficient DP solver, scale to larger number of floors

