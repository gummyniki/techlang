# Techlang Language Reference

A complete reference for the Techlang programming language.

## Table of Contents

- [Comments](#comments)
- [Types](#types)
- [Variables](#variables)
- [Operators](#operators)
- [Control Flow](#control-flow)
- [Functions](#functions)
- [Arrays](#arrays)
- [Pointers](#pointers)
- [Structs](#structs)
- [Enums](#enums)
- [Imports](#imports)
- [Error Handling](#error-handling)
- [Standard Library](#standard-library)

---

## Comments

```techlang
// this is a single line comment
```

---

## Types

| Type | Description | Example |
|---|---|---|
| `int` | 32-bit signed integer | `42` |
| `float` | 32-bit floating point | `3.14` |
| `double` | 64-bit floating point | `3.14159265` |
| `char` | Single character | `'a'` |
| `string` | String of characters | `"hello"` |
| `bool` | Boolean value | `true` or `false` |
| `T[]` | Array of type T | `{1, 2, 3}` |
| `T*` | Pointer to type T | `x.address` |
| `any` | Pointer to an integer, can cast to any type | `42, 42.1, 'a', "hello", etc.` |

### Base Values

Every type has a default base value used when declaring struct instances:

| Type | Base Value |
|---|---|
| `int` | `0` |
| `float` | `0.0` |
| `double` | `0.0` |
| `char` | `'\0'` |
| `string` | `""` |
| `bool` | `false` |
| `struct` | all fields set to their base values |

---

## Variables

### Declaration

```techlang
int x = 5;
float f = 3.14;
double d = 3.14159265;
char c = 'a';
string s = "hello world";
bool b = true;
bool b2 = 0; // same as false
```

### Modifiers

```techlang
float pi = 3.14 [const]; // cannot be reassigned
```

### Assignment

```techlang
x = 10;
x += 5;
x -= 3;
x *= 2;
x /= 4;
```

---

## Operators

### Arithmetic

```techlang
int a = 10 + 3;  // 13
int b = 10 - 3;  // 7
int c = 10 * 3;  // 30
int d = 10 / 3;  // 3
int e = 10 % 3;  // 1
```

### Comparison

```techlang
bool a = 5 == 5;  // true
bool b = 5 != 3;  // true
bool c = 5 > 3;   // true
bool d = 5 < 3;   // false
bool e = 5 >= 5;  // true
bool f = 5 <= 3;  // false
```

### Logical

```techlang
bool a = true && false; // false
bool b = true || false; // true
bool c = !true;         // false
```

### Unary

```techlang
int x = -5;   // negation
bool b = !true; // logical not
```

## Casting
```
 float x = 3.14;
 int y = x as int; // 3

 // variable of type 'any' can cast to all types
```

---

## Control Flow

### If / Else

```techlang
if (x > 0) {
    std.print("positive");
} else {
    std.print("not positive");
}
```

### While

```techlang
while (x > 0) {
    x -= 1;
}
```

### For

```techlang
for (int i = 0, i < 10, i += 1) {
    std.print(i);
}
```

The for loop has three parts separated by commas:
1. **Declaration** — `int i = 0`
2. **Condition** — `i < 10`
3. **Increment** — `i += 1`

---

## Functions

### Declaration

```techlang
function add(int a, int b) returns int {
    return a + b;
}
```

### Calling

```techlang
int result = add(3, 4);
```

### Void Functions

Use `none` as the return type for functions that don't return a value:

```techlang
function greet(string name) returns none {
    std.print(name);
}
```

### Recursion

```techlang
function fibonacci(int n) returns int {
    if (n <= 1) {
        return n;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}
```

### External Functions

Functions can be linked to C implementations using the `extern` keyword:

```techlang
function my_func(int x) returns int extern "c_function_name" {}
```

---

## Arrays

### Declaration

```techlang
int[] nums = {1, 2, 3, 4, 5};
float[] floats = {1.1, 2.2, 3.3};
string[] words = {"hello", "world"};
```

### Access

```techlang
int first = nums[0];
nums[0] = 10;
```

### Methods
```
int[] x = {1, 2, 3, 4};

print(x.length); // 4
```

### Passing to Functions

```techlang
function sum(int[] arr, int size) returns int {
    int total = 0;
    for (int i = 0, i < size, i += 1) {
        total += arr[i];
    }
    return total;
}

int result = sum(nums, 5);
```

---

## Strings

### Decalration

```techlang
string s = "Hello world";
string empty = "";
```

### Concatenation
Strings can be concatenated with the `+` operator:

```techlang
string first = "Hello";

string second = " World";

string result = first + second; // "Hello World"
```

Or by using `std.concat()`:

```techlang
string result = std.concat("foo", "bar"); // "foobar"
```


### length

```techlang
string s = "hello";

std.print(s.length); // 5
```

### Standard Library String Functions

| Function | Description | Example |
|---|---|---|
| `std.concat(string a, string b)` | Concatenate two strings | `std.concat("hello", " world")` |
| `std.string_length(string s)` | Get string length | `std.string_length("hello")` |
| `std.string_equals(string a, string b)` | Compare two strings | `std.string_equals("a", "a")` |
| `std.string_substring(string s, int start, int end)` | Get substring | `std.string_substring("hello", 0, 3)` |

---

## Pointers

### Getting a Pointer

```techlang
int x = 5;
int *p = x.address;
```

### Dereferencing

```techlang
int value = p.value;
```

### Writing Through a Pointer

```techlang
p.value = 10;
```

### Pass by Reference

```techlang
function increment(int *p) returns none {
    p.value += 1;
}

int x = 5;
increment(x.address());
// x is now 6
```

---

## Structs

### Declaration

```techlang
struct person = {
    int age;
    float height;
    string name;
}
```

### Instantiation

```techlang
person p; // all fields set to base values
```

### Field Access

```techlang
p.age = 30;
p.height = 1.75;
p.name = "Alice";
```

### Passing to Functions

```techlang
function greet(person p) returns none {
    std.print(p.name);
}
```

---

## Enums

### Declaration

```techlang
enum direction = {
    NORTH,      // 0
    SOUTH,      // 1
    EAST,       // 2
    WEST        // 3
}
```

### Manual Values

```techlang
enum levels = {
    EASY,       // 0
    MEDIUM,     // 1
    HARD = 9,   // 9
    EXTREME     // 10
}
```

### Usage

Enum values are integers and can be used anywhere an int is expected:

```techlang
int d = NORTH;

if (d == NORTH) {
    std.print("Going north!");
}
```

---

## Imports

Split code across multiple files using `!import`:

```techlang
!import(math.tec) as math;

int result = math.add(3, 4);
```

The alias (`math`) is used to prefix all functions from that file.
Imports are resolved relative to the current file's directory.

### Standard Library

```techlang
!import(std.tec) as std;
```

---

## Error Handling

The recommended pattern for error handling is returning error codes:

```techlang
function divide(int a, int b) returns int {
    if (b == 0) {
        std.exit(1); // exit with error code
    }
    return a / b;
}
```

## I/O
```
any f = std.file_open("hello.txt", "w"); // files are always of type 'any'

std.file_write(f, "Hello from Techlang!\n");
std.file_write(f, "File I/O works!\n");

std.file_close(f);


any f2 = std.file_open("hello.txt", "r");
string line = std.file_read_line(f2);
print(line); // "Hello from Techlang!"

std.file_close(f2);

std.file_delete("hello.txt");
```


---

## Standard Library

Import with `!import(std.tec) as std;`

### Printing

```techlang
std.print(x);
std.print_newline();
```

### Input

```techlang
int x = std.read_int();
float f = std.read_float();
```

### Math

```techlang
float s = std.sqrt(16.0); // 4.0
```

### I/O
```
any f = std.file_open("hello.txt", "w");
std.file_write(f, "Hello from Techlang!\n");
std.file_write(f, "File I/O works!\n");
std.file_close(f);

any f2 = std.file_open("hello.txt", "r");
string line = std.file_read_line(f2);

int eof = std.file_eof(f); // returns 1 if the file ended

std.file_close(f2);
```


### Program Control

```techlang
std.exit(0);  // exit with code 0 (success)
std.exit(1);  // exit with code 1 (error)
```





## VecTec — GPU Compute

VecTec is a companion language to Techlang that runs code on the GPU.
VecTec kernels are written in `.vtec` files and called from Techlang
with zero boilerplate — the compiler handles all CUDA memory
management automatically.

### Requirements

VecTec requires an NVIDIA GPU and the CUDA toolkit installed.

```bash
# Arch Linux
sudo pacman -S cuda

# Ubuntu
sudo apt install nvidia-cuda-toolkit
```

### Writing a Kernel

Kernels are functions that run on the GPU. Every thread runs the
kernel simultaneously — use `threadId()` to know which element to
process.

```vtec
kernel addArrays(int[] a, int[] b) returns int[] {
    int id = threadId();
    return a[id] + b[id];
}
```

### Calling from Techlang

Import a `.vtec` file just like any other Techlang module. The
compiler automatically compiles the kernel to PTX, generates the
CUDA runtime wrapper, and links everything together.

```techlang
!import(std.tec) as std;
!import(arrays.vtec) as gpu;

function main() returns none {
    int[] a = {1, 2, 3, 4};
    int[] b = {5, 6, 7, 8};

    int[] result = gpu.addArrays(a, b);

    std.print(result[0]); // 6
    std.print(result[1]); // 8
}
```

### Built-in Functions

| Function | Description |
|---|---|
| `threadId()` | Returns the current thread's ID within its block (0 to N-1) |
| `threadCount()` | Returns the number of threads per block |
| `blockId()` | Returns the current block's ID within the grid |
| `gridDim()` | Returns the total number of blocks in the grid |
| `syncThreads()` | Blocks until every thread in the block reaches this point |
| `atomicAdd(arr, index, value)` | Atomically adds `value` into `arr[index]`, safe across threads/blocks |

### Shared Memory

Shared memory is fast, per-block memory used to share data between
threads in the same block. Declare it with the `shared` keyword,
with a fixed compile-time size:

```vtec
shared int[256] tile;
```

Shared memory must always be synchronized with `syncThreads()`
before being read by other threads, to ensure every thread has
finished writing first:

```vtec
kernel sumReduce(int[] data, int size, int[] result) returns none {
    shared int[256] tile;
    int tid = threadId();
    int id = blockId() * threadCount() + tid;

    if (id < size) {
        tile[tid] = data[id];
    } else {
        tile[tid] = 0;
    }
    syncThreads();

    int stride = threadCount() / 2;
    while (stride > 0) {
        if (tid < stride) {
            tile[tid] += tile[tid + stride];
        }
        syncThreads();
        stride = stride / 2;
    }

    if (tid == 0) {
        atomicAdd(result, 0, tile[0]);
    }
}
```

This pattern — load into shared memory, sync, reduce in a tree
pattern, sync between each step — is the standard approach for
parallel reductions (sums, mins, maxes) across a block.

### Multi-Block Reductions

A single kernel call only reduces within each block — with
multiple blocks, you get one partial result per block. To combine
partial results across the whole grid, use `atomicAdd` to safely
accumulate each block's result into a single shared output:

```techlang
int[] data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
int[] result = {0}; // must be pre-zeroed

gpu.sumReduce(data, 10, result);

std.print(result[0]); // 55
```

### Supported Types in Kernels

| Type | Notes |
|---|---|
| `int` | 32-bit integer |
| `float` | 32-bit float |
| `double` | 64-bit float |
| `bool` | 1-bit integer |
| `int[]`, `float[]`, etc | Passed as a pointer — one element per thread is typical |


### Kernel Rules

A few things to keep in mind when writing VecTec kernels:

- One thread typically processes one element — use `threadId()` and `blockId()` to compute a global index
- The number of threads/blocks launched is automatically computed from the size of the first array parameter
- Kernels cannot call Techlang functions
- No string or file I/O inside kernels
- Use `atomicAdd` rather than plain writes when multiple threads or blocks might write to the same memory location
