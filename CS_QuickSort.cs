using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.IO;
using System.Diagnostics;
using System.Threading;
using System.Threading.Tasks;

namespace QuickSort
{
    public class ShortState
    {
        public int fstIndex, lstIndex, eqlCount;

        public ShortState(int fstIndex, int lstIndex)
        {
            this.fstIndex = fstIndex;
            this.lstIndex = lstIndex;
        }
    }

    public class ShortSorter
    {
        protected short[] data;
        protected ConcurrentQueue<ShortState> works;
        public CountdownEvent cde;

        public ShortSorter(short[] source)
        {
            data = source;
        }

        public void Randomize(short max_exclusive)
        {
            Random rnd = new Random();
            for (int i = 0; i < data.Length; i++)
            {
                data[i] = (short)rnd.Next(max_exclusive);
            }
        }

        public bool CheckSorted()
        {
            short curr = data[0];
            bool done = true;
            for (int i = 1; i < data.Length; i++)
            {
                if (data[i] < curr)
                {
                    Console.WriteLine($"Break on i={i}, val={data[i]}, curr={curr}");
                    done = false;
                    break;
                }
                else
                if (curr < data[i]) curr = data[i];
            }
            Console.WriteLine($"Curr is {curr}, sort is {done}. Length={data.Length}");
            return done;
        }
        //entry
        public void LoopSort()
        {
            works = new ConcurrentQueue<ShortState>();
            works.Enqueue(new ShortState(0, data.Length - 1));
            cde = new CountdownEvent(1);
            ThreadPool.QueueUserWorkItem(DoWorkLoop);
        }
        //splitting data
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
        //worker`s loop
        private void DoWorkLoop(Object o)
        {
            try
            {
                while (works.TryDequeue(out ShortState work))
                {
                    DoLoopSort(work);
                }
            }
            finally
            {
                // workers done 
                cde.Signal();
            }
        }
        //try add new worker
        private void TryAddLoop()
        {
            if (cde.CurrentCount < 7)
            {
                ThreadPool.QueueUserWorkItem(DoWorkLoop);
                cde.AddCount();
                // workers added 
                //Console.WriteLine("worker added.");
            }
        }
        //quick sort algorithm
        private void DoLoopSort(ShortState work)
        {
            //work begin
            int pivotIndex = DoPartByPivot(work);
            int fstIndex = work.fstIndex;
            int lstIndex = work.lstIndex;
            int eqlCount = work.eqlCount;
            // recurse on the left block
            if (fstIndex < pivotIndex - 1)
            {
                works.Enqueue(new ShortState(fstIndex, pivotIndex - 1));
                //do in current loop
            }
            // recurse on the right block
            if (pivotIndex + eqlCount < lstIndex)
            {
                works.Enqueue(new ShortState(pivotIndex + eqlCount, lstIndex));
                TryAddLoop();
            }
            //work done
        }

        internal void SwapValues(int firstIndex, int secondIndex)
        {
            //if (firstIndex == secondIndex) return;
            short holder = data[firstIndex];
            data[firstIndex] = data[secondIndex];
            data[secondIndex] = holder;
        }
    }

    class Program
    {
        static void Main(string[] args)
        {
            Console.WriteLine("Work 4096 * 1024 started");
            short[] data = new short[4096 * 1024];
            ShortSorter shortSorter = new ShortSorter(data);
            shortSorter.Randomize(256);
            Stopwatch stopWatch = new Stopwatch();
            stopWatch.Start();
            //71 - 78 ms
            shortSorter.LoopSort();
            shortSorter.cde.Wait(600);
            stopWatch.Stop();
            TimeSpan ts = stopWatch.Elapsed;
            Console.WriteLine("Done: Время {0} ms", ts.Milliseconds);
            //Thread.Sleep(400);
            shortSorter.CheckSorted();
            Console.Read();
        }

    }
}
