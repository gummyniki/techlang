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
| `ArrayOf(T)` | Array of type T | `{1, 2, 3}` |
| `PointerOf(T)` | Pointer to type T | `std.addressOf(x)` |
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
 int y = x as int // 3

 // variable of type 'any' can cast to all types
```

---

## Control Flow

### If / Else

```techlang
if (x > 0) {
    std.print_string("positive");
} else {
    std.print_string("not positive");
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
    std.print_int(i);
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
    std.print_string(name);
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
ArrayOf(int) nums = {1, 2, 3, 4, 5};
ArrayOf(float) floats = {1.1, 2.2, 3.3};
ArrayOf(string) words = {"hello", "world"};
```

### Access

```techlang
int first = nums[0];
nums[0] = 10;
```

### Methods
```
ArrayOf(int) x = {1, 2, 3, 4};

print(x.length); // 4
```

### Passing to Functions

```techlang
function sum(ArrayOf(int) arr, int size) returns int {
    int total = 0;
    for (int i = 0, i < size, i += 1) {
        total += arr[i];
    }
    return total;
}

int result = sum(nums, 5);
```

---

## Pointers

### Getting a Pointer

```techlang
int x = 5;
PointerOf(int) p = std.addressOf(x);
```

### Dereferencing

```techlang
int value = std.deref(p);
```

### Writing Through a Pointer

```techlang
std.storeAt(p, 10); // x is now 10
```

### Pass by Reference

```techlang
function increment(PointerOf(int) p) returns none {
    int current = std.deref(p);
    std.storeAt(p, current + 1);
}

int x = 5;
increment(std.addressOf(x));
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
    std.print_string(p.name);
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
    std.print_string("Going north!");
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
std.print_int(42);
std.print_float(3.14);
std.print_string("hello");
std.print_char('a');
std.print_bool(true);
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


### Pointers

```techlang
PointerOf(int) p = std.addressOf(x);
int val = std.deref(p);
std.storeAt(p, 10);
```

### Program Control

```techlang
std.exit(0);  // exit with code 0 (success)
std.exit(1);  // exit with code 1 (error)
```
