# Project Workload Distribution

## (1) Image loader and buffering (AFFAN)
  - Read images continously from a directory
  - Break images into chunks
    - Multithreaded chunking process
  - (Optional) Use thread pooling to avoid system overload
    - Implement an algorithm for best thread pool based on workload 
    - Make use of queuing until threads are free
  - Write the chunks into a shared memory buffer (shared between #1 & #2)
    - Ensure Synchronization
      - Mutexes
      - Semaphores
  - Implement atleast one function in the filtering procedure 
## (2) Image reading and filtering procedure - prototype only (ALIYAN)
  - Read from the shared buffer
    - Ensure synchronization
      - Mutexes
      - Semaphores
    - Create threads for each chunk read
  - Send them to the filtering pipeline (Sequence filter applying functions)
      - Create the prototypes of the filtering functions
      - Create the basic flow of filtering procedure
  - After the filtering procedure, write the chunks into another shared buffer (shared between #2 & #3)
    - Ensure Synchronization
      - Mutexes
      - Semaphores
  - Implement atleast one function in the filtering procedure
  - (Optional) Thread pooling
## (3) Image reconstruction (FAROOQ)
  - Read the chunks from the 2nd shared buffer
    - Ensure Synchronization
      - Mutexes
      - Semaphores
  - Reconstruct the images based on image IDs and coordinates
  - Write the images back into a directory
  - Implement atleast one function in the filtering procedure
  - (Optional) Thread pooling

## Why not create a thread for each chunk, filter chunk in that thread, and reconstruct chunks sequentially? 

#### **1. Modularity & Separation of Concerns**
- Each process handles a clear stage:
  - Reader → Chunker → Filter → Reconstructor.
- Easier to debug and test each module.
- You can scale or even replace one stage (like the filter pipeline) without affecting others.

#### **2. Fault Isolation**
- If one process crashes (e.g., filter segmentation fault), others continue or can recover.
- Threads share memory space — one bad pointer or race condition could crash everything.

#### **3. Parallelism (True Concurrency)**
- Processes run truly concurrently on multiple cores.
- Threads are lightweight but limited by the **Global Interpreter Lock** in some environments (like Python) or by shared CPU time if not tuned properly.

#### **4. Buffering Enables Pipelining**
- Producer-consumer behavior: `reader` fills buffer while `filter` is working.
- Prevents bottlenecks — reader doesn't need to wait for the filter or reconstructor.

---

### **Why NOT Just Use Threads Per Chunk?**

#### **1. Threads Add Complexity (Race Conditions, Locks)**
- Shared memory in threads means you need mutexes, condition variables.
- Synchronization gets hard fast, especially when writing to/from buffers and reconstructing images.

#### **2. Not Modular**
- Filter pipeline lives inside the same process/thread context.
- Harder to test, extend, or swap filters (say, trying different filtering backends).

#### **3. Reconstruction Becomes Trickier**
- Need to synchronize all threads processing chunks of the same image.
- Wait for all chunks → merge → reconstruct. That’s a mini orchestration problem on its own.

---

### **So When Would Threads Be Better?**

- If you're dealing with **small-scale data**, or **latency is more important than throughput**.
- If you want **lower memory footprint** — threads consume less memory than processes.
- When all operations are **tightly coupled and need shared memory anyway**.

---

### A Hybrid Approach?

You can **combine both**:
- Use **processes** for pipeline stages (reader, filter, reconstructor).
- Inside the filter stage, use **threads per chunk** to parallelize filter operations within that module.

This gives you:
- Isolation + modularity
- Thread-level parallelism where it matters (e.g., GPU filters, image transforms)
