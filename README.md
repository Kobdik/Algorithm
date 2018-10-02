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

Пример кода из книги показался мне недостаточно эффективным в случае наличия большого числа повторяющихся элементов. Не выполнялся 2-й шаг алгоритма, среди "меньших" элементов могли встречаться равные pivot, а вычленять из цепи только по одному элементу - неэффективно, в итоге, работает более чем 2 раза медленнее. Переписав всё по-своему, получил код, который в синхронном варианте сортирует 5 000 000 псевдослучайных чисел (со значениями от 0 до 255) за ~206 ms (при использовании библиотечного `Array.Sort` результат ~570 ms). При параллельном выполнении ~85 ms, ускорение данного алгоритма всего в 2.5 раза при использовании 8 ядер.

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
Большое спасибо dot.net за ConcurrentQueue, который позволяет обходиться нам без мьютексов. Скорее всего, он написан с использованием неблокирующей технологии \<atomic\>.
  
# C++

Аналогичный код на C++ (см. CppSort.cpp) в синхронном исполнении сортирует 5 000 000 псевдослучайных чисел (со значениями от 0 до 255) за ~205 ms, против ~190 ms при использовании `std::sort(vec.begin(), vec.end());`. Параллельная сортировка на C++ в моем исполнении только сравнялась с C# и составила ~85 ms. Сколько я ни бился, быстрее не получилось. Сортировка производится на 8-ми потоках, когда нет блоков для обработки - поток засыпает. Работающий поток посылает уведомления о новых блоках. Подсчет "уснувших" потоков дает возможность разбудить их все и завершить программу.
```cpp
	void Worker()
	{
		//std::cout << std::this_thread::get_id() << " started " << std::endl;
		std::unique_lock<std::mutex> uni(mux);
		while (true)
		{
			//entered workers
			if (!works.empty())
			{
				ShortState *work = works.front();
				works.pop();
				uni.unlock();
				//work obtained
				DoSort(work);
				delete work;
				uni.lock();
			}
			else
			{
				++counter;
				if (counter > 7)
				{
					exit = true;
					worker_cnd.notify_all();
					main_cnd.notify_one();
				}
				else
				{
					worker_cnd.wait(uni, [this] { return UnLock();	});
					--counter;
				}
				if (exit) break;
			}
		}
		//std::cout << std::this_thread::get_id() << " done " << std::endl;
	}
```
В С++17 STL я не нашел готовых к применению неблокирующих контейнеров. Работа с библиотекой \<atomic\> для меня остаётся чёрной магией. Лучшая книга по данной теме - в книге издательства Manning "C++ Concurrency in Action" by Antony Williams. 

# Go

Большие надежды возлагаю на горутины и каналы для синхронизации потоков при параллельном исполнении алгоритма. Настораживает то факт, что стандартный шаблон `sort.Sort(Int16Slice(data[:]))` отрабатывает аналогичный массив за 620 ms, однако, параллельный вариант программы сортирует за 78 ms. Также как в C# не пришлось извращаться, чтобы подсчитать, когда мои горутины все завершились. Чтобы канал works не заблокировал все горутины, когда закончатся неотсортированные блоки, применил неблокирующий код, позоляющий подсчитывать количество действующих горутин. Очень интересный инструмент на основе каналов и горутин, логика работы с потоками стала нагляднее, а количество строк кода уменьшилось почти вдвое!

```go
func (data Int16Slice) workLoop(works chan *Work, counter *sync.WaitGroup) {
	counter.Add(1)
	hasWork := true
	for hasWork {
		select {
		case work := <-works:
			//add goroutine
			if data.doSort(works, work) {
				go data.workLoop(works, counter)
			}
		default:
			hasWork = false
		}
	}
	//fmt.Printf("Goroutine done\n")
	counter.Done()
}
```
