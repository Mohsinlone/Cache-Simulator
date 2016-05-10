This project implements a flexible cache and memory hierarchy simulator and uses it to compare the performance, area, and energy of different memory hierarchy configurations. For more details, read the project specification. Developed as course requirement for ECE521 (Fall 2015) at NC State Univeristy. 

How to run code - 
make clean
make
sim_cache <BLOCKSIZE> <L1_SIZE> <L1_ASSOC> <VC_NUM_BLOCKS> <L2_SIZE> <L2_ASSOC> <trace_file>

BLOCKSIZE: Positive integer. Block size in bytes. (Same block size for all caches in the memory hierarchy.)

L1_SIZE: Positive integer. L1 cache size in bytes.

L1_ASSOC: Positive integer. L1 set-associativity (1 is direct-mapped).

VC_NUM_BLOCKS: Positive integer. Number of blocks in the Victim Cache. VC_NUM_BLOCKS = 0 signifies that there is no Victim Cache.

L2_SIZE: Positive integer. L2 cache size in bytes. L2_SIZE = 0 signifies that there is no L2 cache.

L2_ASSOC: Positive integer. L2 set-associativity (1 is direct-mapped).
trace_file: Character string. Full name of trace file including any extensions.

Example: 8KB 4-way set-associative L1 cache with 32B block size, 7-block victim cache affiliated with L1, 256KB 8-way set-associative L2 cache with 32B block size, gcc trace:

sim_cache 32 8192 4 7 262144 8 gcc_trace.txt