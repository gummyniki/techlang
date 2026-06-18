# Basic Syntax

## Variables

### Data types

- int
- float
- double
- char
- string
- boolean
- array
- enum
- struct

---

### Variable declaration

```
Each datatype has a "base" value (used later):
int = 0;
string = "";
float = 0;
double = 0;
char = '';
struct = {};
enum = {};
array = {};


Comments are created with "//"


*datatype* *name* = *value* [*optional params*];

int i = 3;
string s = "hello world";
float f = 3.14 [const];
f = std.CastInt(f); // returns 3 (int)

i = i + 4; i += 4;
s = "hello not world";

ArrayOf(int) a = {3, 4, 10};
a[0] = 5;
a[4] = 7; // normally the array goes up to [2], all elements between [2] and [4] will be set to the type's default value (0)

ArrayOf(float) a2 = {3.14, 4.5, 10.1} [fixed]; // fixed = fixed size, can set elements to other values but cannot add more elements;

bool b = 0; bool b = false; // same thing

struct person = {
    int age;
    float height;
    string name;
}

person p; // p's values will be set to their corresponding base values
p.age = 30;

enum levels = {
    EASY, // 0
    MEDIUM, // 1
    HARD = 9, // can manually set value
}
```


## Statements

### if

```
if (statement) do {
 do stuff;
} else do {
 do other stuff;
}
```

### for

```
for (*declaration*, *statement*, *increment/decrement*) do {
    do stuff;
}
```

### while

```
while (*statement*) do {
    do stuff;
}
```

### functions


```
function x(*params*) returns *type* {
    do stuff;

    return *value*;
}

x(*params*);


For a function to not return anything, the type is "none"
```


## Imports

```
!import(file.tec) as file;


file.foo();
```

## Error handling

All functions should ideally return an error code, which then gets checked after calling the function.

```
function set(int variable, int value) returns int {
    variable = value;

    return 0; // in case something bad happens, return 1;
}

int x = 0;
int result = set(x, 5);

if (result != 0) {
    // exit the program
}
```

## Standard library

imported with: 
```
!import(std.tec) as std;
```

has functions like: ```
std.print();

std.CastInt();
std.CastFloat();
std.CastChar();
std.CastString();

std.exit(int errorCode);

// etc
```
