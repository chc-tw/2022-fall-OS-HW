# 2022-fall-OS-HW

In this course, I need to do 6 homeworks about operation system practice.
1. Simple shell
2. Multi-Process Matrix Multiplication using Shared Memory
3. Parallel Merge Sort with Threads
4. malloc() Replacement
5. Page Replacement Simulation: LRU and LFU
6. A User-Space File System Implementation

## Hw1 Simple Shell
Basic requirements:
1. Display the prompt sign “>” and take a string from user
2. Parse the string into a program name and arguments
3. Fork a child process
4. Have the child process execute the program
5. Wait until the child terminates
6. Go to the first step

### Bonus
- I/O redirection
`ls –l > a.txt`
- pipe
`ls | more`

## Hw2 Multi-Process Matrix Multiplication using Shared Memory
Basic requirements:
1. Matrix multiplication using multiple processes
2. Read the dimension of two square matrices A & B
3. Calculate the matrx multiplication using 1~16 processes repsepctively
4. Print all execution time and a checksum

## Hw3 Parallel Merge Sort with Threads
Basic requirements:
1. Divide original array into eight equal pieces
2. Calculate merge sort using 1~8 threads respectively
3. Print all execution time
Ex. merge sort with 4 work threads
![](https://i.imgur.com/byE22HE.png)

## Hw4 malloc() Replacement
Basic requirements:
1. Implement details about malloc() and free() using best fit and firt fit respectively.

## Hw5 Page Replacement Simulation: LRU and LFU
Basic requirements:
1. read the test case which contains page number of referenced pages
2. do the page replacement using LRU and LFU

## Hw6 A User-Space File System Implementation
1. Implementing a user-space file system that mounts a tar file onto a specified directory
2. Files in the tar files can be accessed through the system directory tree
3. The program will run as a FUSE server
