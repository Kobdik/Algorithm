# Algorithm
Performance comparison (C#, C++, Go)

Разбирая примеры из книги Адама Фримана Pro .NET 4 Parallel Programming in C#, нашел там следующее описание алгоритма быстрой сортировки:

Quicksort is a sorting algorithm that is well suited to parallelization. It has three steps:
1. Pick an element, called a pivot, from the data to be sorted
2. Reorder the data so that all of the elements that are less than the pivot come before the pivot in the data and all the elements that are greater than the pivot come after the pivot.
3. Recursively process the subset of lesser elements and the subset of greater elements.
```csharp
internal static int partitionBlock(T[] data, int startIndex, int endIndex, IComparer<T> comparer) {
// get the pivot value - we will be comparing all of the other items against this value
T pivot = data[startIndex];
// put the pivot value at the end of block
swapValues(data, startIndex, endIndex);
// index used to store values smaller than the pivot
int storeIndex = startIndex;
// iterate through the items in the block
for (int i = startIndex; i < endIndex; i++) {
  // look for items that are smaller or equal to the pivot
  if (comparer.Compare(data[i], pivot) <= 0) {
    // move the value and increment the index
    swapValues(data, i, storeIndex);
    storeIndex++;
  }
}
swapValues(data, storeIndex, endIndex);
return storeIndex;
}```

Пример кода из книги показался мне недостаточно эффективным в случае наличия большого числа повторяющихся элементов. Не выполнялся 2-й шаг алгоритма, среди "меньших" элементов могли встречаться равные pivot, а вычленять из цепи только по одному элементу - неэффективно. Переписав всё по-своему, получил код, который работает с быстродействием, сопоставимым с `Array.Sort`.



```csharp
        private int DoPartByPivot(ShortState work)
        {
            int eqlCount = 0, fstIndex = work.fstIndex, lstIndex = work.lstIndex;
            short value, pivot = data[fstIndex++];
            // iterate through comparing with pivot
            while (fstIndex <= lstIndex)
            {
                value = data[fstIndex];
                // look for smaller or equal to the pivot
                if (value <= pivot)
                {
                    // increment the index
                    fstIndex++;
                }
                else
                {
                    // move greater to the right side
                    SwapValues(fstIndex, lstIndex);
                    lstIndex--;
                }
            }
            fstIndex = work.fstIndex;
            // iterate through comparing with pivot
            while (fstIndex <= lstIndex)
            {
                value = data[fstIndex];
                // look for equal to the pivot
                if (value == pivot)
                {
                    // move equal to the right side
                    SwapValues(fstIndex, lstIndex);
                    lstIndex--;
                    eqlCount++;
                }
                else
                {
                    // increment the index
                    fstIndex++;
                }
            }
            //count of repeated values
            work.eqlCount = eqlCount;
            //values greater or equal to the pivot
            return fstIndex;
        }
```
