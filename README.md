# Algorithm
Performance comparison (C#, C++, Go)

# C#
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
}
```

Пример кода из книги показался мне недостаточно эффективным в случае наличия большого числа повторяющихся элементов. Не выполнялся 2-й шаг алгоритма, среди "меньших" элементов могли встречаться равные pivot, а вычленять из цепи только по одному элементу - неэффективно. Переписав всё по-своему, получил код, который в синхронном варианте сортирует 5 000 000 псевдослучайных чисел (со значениями от 0 до 255) за ~206 ms, против ~570 ms при использовании библиотечного `Array.Sort`. При параллельном выполнении ~ 85 ms.

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
# C++

Аналогичный код на C++ (см. CppSort.cpp) в синхронном исполнении сортирует 5 000 000 псевдослучайных чисел (со значениями от 0 до 255) за ~205 ms, против ~200 ms при использовании `std::sort(vec.begin(), vec.end());`. Параллельную сортировку на C++ пока не реализовал, надо разобраться с работой `std::future<>, std::async() etc...`, выложу коды, как только заработает.

# Go

Большие надежды возлагаю на горутины и каналы для синхронизации потоков при параллельном исполнении алгоритма. Однако, настораживает то факт, что стандартный шаблон `sort.Sort(ByteSlice(data[:]))` отрабатывает аналогичный массив за 620 ms. Посмотрим, что покажет мой вариант параллельной сортировки, в течение 3-х дней постараюсь решить задачу. 
