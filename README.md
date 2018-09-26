# Algorithm
Performance comparison (C#, C++, Go)

Quicksort is a sorting algorithm that is well suited to parallelization. It has three steps:
1. Pick an element, called a pivot, from the data to be sorted
2. Reorder the data so that all of the elements that are less than the pivot come
before the pivot in the data and all the elements that are greater than the pivot
come after the pivot.
3. Recursively process the subset of lesser elements and the subset of greater
elements.


```csharp
using Kobdik.Common;
using Kobdik.DataModule;
```
