## 100 general SWE trivia questions
# What is the difference between a process and a thread?
- A process is an independent program with its own memory, variables, etc
- A thread is a a small unit of a process which shares memory
- Ex: Open google chrome for the first time is a process, but every tab you open is independent from another tab, but still uses the overall chrome process

# What is a race condition?
- 2 operations that are happening at the same time
- common in multi threaded applications but can show up in memory and software
- if 2 or more threads happen at the same time affect a part, 
- Expand on diff types of ways to counter race codnition

# What is a deadlock?
- first thread holds a lock, waits for the second one to finish its thread
-2 nd thread holds a 
# What is the difference between a mutex and a semaphore?
- single thread can access 1 critical code at a time
- semaphore allows set number of threadsa at once. like a counter

# What is the difference between stack memory and heap memory?
- everytime a function runs, it is pushed on the stack frame as a stack (contains function param, local variables and fuction addresses) LIFO works. Last function call is the first one to finish. If fast an predicatble, no free memory as everythin in cleaned up autonomckally
- varilanes taht are outside the functions are in the heap, which is where memory that is needed to be stored dinamically is stored. for custom datastructures, needs to be deallcoated manually. Relay of a grabage collector

# what is seg faulting
- When the memory access in is invalid, can  be refernceing something after freeing up memory

# What causes a stack overflow?
- when a proccess uses more than its allocated stack space. Happens when a proccess makes too many recursive of function calls. 

# What is virtual memory?
- Virtual memory is the illusion that each process has its own continuous memory space, even though the OS is mapping it behind the scenes to real memory.
- Virtual memory is the address space a process sees, which the OS maps to physical RAM and sometimes disk.

# What is paging?
- how virtual memory is split up. When a proccess is very large, it is broken up into parts instead of using all the ram at once. Ex: If you have 8gb ram and 16gb process, it is going to be paged.

# What is a page fault?
- when a process tries to access a virtual memory page that is not in the physical ram  

# What is context switching, and why is it expensive?
- Context switching is when the CPU stops running one thread or process, saves its execution state, and loads the state of another so that one can run instead.
- expensive bc the CPU spends time saving and restoring state instead of doing useful work and switching between processes is often more expensive than between threads because processes have separate address spaces

# What is the difference between user mode and kernel mode?
- kernel mode is where the OS lives has unrestricted access to memory and what not. 
- user mode si where prgrams live, they are delegated memory from kernal mode and deal with the abstracted simpilar world

- User mode is the restricted mode where normal programs run. They cannot directly access hardware or protected memory, and must ask the OS to do privileged operations.

- Kernel mode is the privileged mode where the operating system runs. It has full access to memory, hardware, and CPU instructions.

# What happens during a system call?
- when a user-mode program asks the kernal to do something privledged

# What is the difference between preemptive and cooperative multitasking?
- Preemptive multitasking means the OS can interrupt a running process or thread and switch to another one.

- Cooperative multitasking means a running process or thread keeps control of the CPU until it voluntarily gives it up.

# What is an interrupt?
- A signal that inturpts the CPU by pausing and doing something urgent 

# What is DMA and why is it useful?
- Direct Memory Access It is a way for a hardware device to read from or write to RAM directly, without making the CPU move every byte itself.

# What is cache locality?
- cache that is stored nearby incase it needs to be reused instead of needing it to come from the RAM

# What is the difference between temporal and spatial locality?
- Temporal locality means a program is likely to access the same data again soon.
- Spatial locality means a program is likely to access nearby memory locations soon.

# What is cache coherence?
- when different CPU cores keep stale copies. If x = 5 in core 1 and then x = 9 in core 2 then the next time core 1 is read, jts changed to x = 9


# What is the difference between latency and throughput?
- latency is how long the operation takes
- throughput is the rate of something

# What is big-O time complexity?
- Big-O time complexity describes how an algorithm’s running time grows as the input size grows.

# What is the difference between amortized and worst-case complexity?
- worst case looks at the most expensive or longest time it can take
- the average over multiple many operations

# What is the average lookup complexity of a hash table?
- O(1) lookup

# What causes hash collisions?
- Hash collisions happen when two different keys produce the same hash value or map to the same bucket/index.

# What is the difference between an array and a linked list?
- arrays are stored in contiguous memory and a collection of data that are accessed via indexing, 
- linked lists are a data strucutre which are linking in memory, so where each node as an address in memory, data and with some data that links

# When would you use a heap instead of a balanced BST?
- A heap is used when you want fast access to the smallest or largest element min or max heap.
- A balanced BST is used when you want ordered data and fast search for arbitrary values too.

# What is the difference between DFS and BFS?
- dfs goes all the way deep then backtracks, at the node split, bfs goes level order so explores all nneighboring nodes then goes to teh next level

# What is topological sort used for?
- Topological sort is used to order tasks that have dependencies.

# What is Dijkstra’s algorithm used for?
- Dijkstra’s algorithm is used to find the shortest paths from one source node to all other reachable nodes in a weighted graph with non-negative edge weights.

# Why does Dijkstra fail on graphs with negative edge weights?
- Negative edges can make a previously finalized path become shorter later, which breaks Dijkstra’s greedy logic.

# What is dynamic programming?
- Dynamic programming is a method for solving problems by breaking them into overlapping smaller subproblems, storing their results, and reusing those results instead of recomputing them.

# What is memoization?
- Memoization is saving the result of a subproblem the first time you compute it, so later calls can reuse it instead of recomputing it.

# What is tabulirization
- Tabulation is solving the subproblems in a planned order, usually from smallest to largest, and filling in a table iteratively.

# What is the difference between greedy algorithms and dynamic programming?
- Greedy algorithms do not try all possibilities. They make the best local choice at each step and move on, hoping that leads to a global optimum.

- Dynamic programming solves overlapping subproblems, stores their results, and combines them to get the best overall answer.

# What is a trie useful for?
- A trie is useful for storing and searching strings efficiently when many strings share common prefixes.

# What is the difference between stable and unstable sorting?
Stable vs unstable sorting

A stable sort keeps equal elements in the same relative order as before sorting.
An unstable sort may change the order of equal elements.

# What is quicksort’s average and worst-case runtime?
Average case: O(nlogn)
Worst case: O(n2)

# Why is mergesort preferred for linked lists?
- Mergesort is preferred for linked lists because linked lists can be split and merged efficiently using pointers, and mergesort does not require fast random access.

# What is the difference between a min-heap and a max-heap?
- Min-heap: the smallest element is at the root.
- Max-heap: the largest element is at the root.

# What is a bloom filter?
- A Bloom filter is a space-efficient probabilistic data structure used to test whether an item is possibly in a set or definitely not in a set.

# When would you use a ring buffer?
- When you only caare about a fixed amount of data and when data comes and goes. Example if there is a data stream and you only care about 10, then you would write the first 10 and when you get to the 11th one, you would start overwriding the first one. 


# What is the difference between TCP and UDP?
- TCP is secure/slow and requires a handshake, over some IP address. reliable: guarantees ordered delivery and retransmits lost data
- UDP is fast for data streaming and less secure. Good for videos, and media
- TCP is a reliable, connection-oriented protocol that uses a handshake and ensures ordered delivery. UDP is a connectionless protocol with lower overhead that sends data without guaranteeing delivery or order, which makes it useful for real-time applications.


# What is a socket?
- A socket is an endpoint for sending and receiving data over a network.


# What is the TCP three-way handshake?
SYN
The client says: “I want to connect.”
SYN-ACK
The server replies: “I got your request, and I agree.”
ACK
The client responds: “Got it.”

# What is flow control?
- A way of sending data that prevents data from being sent from the sender faster than the reciever can recriept it


# What is congestion control?
- Congestion control is the mechanism that prevents too much data from being sent into the network too quickly.


# What is the difference between reliable delivery and ordered delivery?
- Reliable delivery means the data eventually arrives and missing data is retransmitted.
- Ordered delivery means the data is given to the receiver in the same order it was sent.



# What is packet jitter?
- Jitter is variation in packet arrival time.
- Example: packets should arrive every 20 ms, but instead arrive after 10 ms, 35 ms, 18 ms.
- High jitter is bad for real-time audio, video, and games because playback becomes uneven.

# What is the difference between HTTP and HTTPS?
- HTTP sends web data without encryption.
- HTTPS is HTTP plus TLS, which encrypts the connection.

# What role does TLS play?

- TLS is what provides the security in HTTPS.
- Its role is to:

- encrypt data so others cannot read it
= authenticate the server so you know you are talking to the real site
- protect integrity so data is not changed in transit

# What is DNS?
- DNS stands for Domain Name System. It translates human-readable names like google.com into IP addresses that computers use to find each other.

# What is NAT?
- NAT stands for Network Address Translation. It lets multiple devices on a private local network share one public IP address by translating internal addresses to the external one.

# What is the difference between a router and a switch?


# What is the difference between symmetric and asymmetric encryption?


# What is a certificate authority?


# What is the difference between authentication and authorization?


# What is a replay attack?


# What is a man-in-the-middle attack?


# What is an idempotent API?


# What does REST mean in practice?


# What is the CAP theorem?


# What is undefined behavior in C or C++?


# What is the difference between compiled and interpreted languages?


# What is the difference between static and dynamic linking?


# What is name mangling in C++?


# What is RAII?


# What is a memory leak?


# What is a dangling pointer?


# What is the difference between shallow copy and deep copy?


# What is move semantics in C++?


# What is the Rule of Three, Five, or Zero?


# What is a vtable?


# What is the difference between compile-time polymorphism and runtime polymorphism?


# What is const correctness?


# What is the difference between signed and unsigned integer overflow in C++?


# What is endianness?


# What is alignment, and why does it matter?


# What is a segmentation fault?


# What is UB from reading uninitialized memory?


# What is the difference between new/delete and malloc/free?


# What problem do smart pointers solve?


# What is a transaction in a database?


# What is ACID?


# What is the difference between serializable and read committed isolation?


# What is an index and when can it hurt performance?


# What is normalization?


# What is denormalization?


# What is the N+1 query problem?


# What is eventual consistency?


# What is replication?


# What is sharding?


# What is the difference between vertical and horizontal scaling?


# What is load balancing?


# What is a message queue?


# What is backpressure in distributed systems?


# What is an idempotency key?


# What is a circuit breaker in service design?


# What is observability?


# What is the difference between logs, metrics, and traces?


# What makes a system fault tolerant?


# What is the difference between availability and reliability?
