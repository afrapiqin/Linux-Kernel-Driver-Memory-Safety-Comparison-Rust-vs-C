# Linux Kernel Driver Memory Safety Comparison: Rust vs C

This project examines memory safety trade-offs in Linux device drivers by implementing a simple $O(1)$ problem: a stack data structure in both Rust and C. It demonstrates Rust’s ability to enforce memory safety by triggering a controlled panic on out-of-bounds (OOB) accesses while allowing the module to unload gracefully. In contrast, the equivalent C implementation lacks these safeguards and crashes the entire system upon an OOB access. This comparison highlights Rust’s advantages in preventing memory-related failures without compromising system stability.

## System
- Linux Kernel 6.13 with Rust support enabled.
- Raspberry Pi 4 Model B.
- Required kernel headers and toolchains:
  - Rust nightly toolchain with Rust kernel modules enabled.
  - Clang toolchain for compiling the C module.

## Installation and Usage
### 1. Building and Loading the C Module
```sh
cd CModuleSrc
make
sudo insmod scull_misc_c.ko
```

### 2. Triggering an OOB Crash in the C Module
```sh
clang -o c_test c_test.c
./c_test
```
*Expected outcome:* System crash requiring reboot.

### 3. Building and Loading the Rust Module
```sh
cd RustModuleSrc
make
sudo insmod scull_misc_rust.ko
```

### 4. Triggering an OOB Panic in the Rust Module
```sh
clang -o rust_test rust_test.c
./rust_test
```
*Expected outcome:* Kernel panic log with detailed trace, followed by safe module unloading.

## `dmesg` Log Output (Rust Module)
```
[   81.466389] rust_kernel: panicked at scull_misc_rust.rs:109:9:
               index out of bounds: the len is 127 but the index is 127
[   81.478873] ------------[ cut here ]------------
[   81.478881] kernel BUG at rust/helpers/bug.c:7!
[   81.483483] Internal error: Oops - BUG: 00000000f2000800 [#1] PREEMPT SMP
...
[   81.764970] scull_rust: release was successful
```

## Comparison Summary
| Feature          | Rust Module | C Module |
|----------------|------------|---------|
| OOB Handling | Kernel Panic (Safe Unload) | System Crash (Requires Reboot) |
| Memory Safety | Enforced by Rust's Bounds Checking | No Protection, Possible Kernel Corruption |
| System Stability | Module Automatically Unloaded | Full System Failure |

## Performance
Two key operations were measured: stack insert and pop operations. The results compare execution time for both Rust and C implementations. The data shows that C outperforms Rust in both insert and pop operations, consistently exhibiting lower execution times. While Rust introduces some overhead due to its safety guarantees, the performance difference remains moderate. This suggests that Rust can still be a viable option for memory-safe implementations, though with a slight trade-off in execution speed compared to C.

![Insert Performance Graph](/data/graph.png)

## To-Do
- Implement an $O(log n)$ problem using a binary tree data structure to analyze increasing problem complexity and evaluate the cost effect of Rust's memory allocation as complexity grows.
- Develop a Rust driver for the DHT11 sensor to evaluate its practical performance in real-life applications.

## Acknowledgments
- Rust for Linux initiative for enabling safe kernel programming.

