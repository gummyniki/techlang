# Techlang 🔧

**A compiled, C-like systems language where GPU kernels are just functions.**

Techlang is actually two languages that work as one:

- **Techlang** — a statically typed, C-like language that compiles to native binaries via LLVM
- **VecTec** — its GPU compute companion: write CUDA kernels with the same clean syntax, import them like any other module, and call them like normal functions

No host/device boilerplate. No build-system glue. No Python runtime. Just functions, some run on the CPU, some run on the GPU.

> ⚠️ Techlang is currently in **alpha**. Expect bugs and missing features.
> Contributions and feedback are welcome!

### [Website](https://gummyniki.github.io/techlang-website/) · [Language Reference](docs/LANGUAGE.md)

## The main idea

Here's a GPU kernel in VecTec:

```
// arrays.vtec
kernel addArrays(int[] a, int[] b, int size) returns int[] {
    int id = blockId() * threadCount() + threadId();
    if (id < size) {
        return a[id] + b[id];
    }
    return 0;
}

```

And here's calling it from Techlang, like it's any other function:

```
// arrays.tec
!import(std.tec) as std;
!import(arrays.vtec) as gpu;

function main() returns none {
    int[] a = {1, 2, 3, 4};
    int[] b = {5, 6, 7, 8};
    int[] result = gpu.addArrays(a, b, 4);
    std.print(result[0]); // 6
}
```

Under the hood, VecTec compiles your kernel to PTX and generates all the CUDA
driver calls (module loading, launches, transfers) automatically. The
equivalent CUDA C++ program is dozens of lines of `cudaMalloc`, `cudaMemcpy`,
kernel launch syntax, and error checking.



## Why Techlang?

- **GPU compute as a first-class citizen** — kernels are functions, not a separate toolchain
- **Familiar syntax** — if you know C, C++, or Java, you'll feel right at home
- **Statically typed** — catch errors at compile time, not at runtime
- **Fast** — compiles to native binaries via LLVM with full optimization support
- **Painless C interop** — declare any C function with `extern` and link against existing libraries. The Vulkan + GLFW bindings for the demo renderer were written this way
- **Modern features** — structs, enums, pointers, and a simple `!import` module system

## CPU example

```
!import(std.tec) as std;

struct person = {
    int age;
    string name;
}

function greet(person p) returns none {
    std.print("Hello, ");
    std.print(p.name);
}

function main() returns none {
    person p;
    p.age = 25;
    p.name = "Alice";
    greet(p);
}
```

Output:

```
Hello, Alice
```

## Installation

**Quick install (Linux x64):**

```
wget https://github.com/gummyniki/techlang/releases/download/v0.3.0-alpha/techlang-linux-x64.tar.gz
tar -xzf techlang-linux-x64.tar.gz
cd techlang-linux-x64 && ./install.sh
```

### Requirements

- LLVM 17+
- GCC
- CMake 3.20+
- Linux or macOS
- CUDA toolkit (only if using VecTec)

### Build from source

```
git clone https://github.com/gummyniki/techlang
cd techlang
mkdir build && cd build
cmake ..
make
```

The compiler binary will be at `build/techlang`. Optionally add it to your PATH:

> The release versions of techlang arrive around once every 2-3 weeks, but the language is constantly getting updates this early in development, it's recommended to manually build from source once every couple of days so you are always up to date

```
export PATH=$PATH:/path/to/techlang/build
```

### Compile and run

```
./build/techlang hello.tec
./hello
```

## Examples

The `examples/` directory contains programs demonstrating the language:

| Example         | Description                                    |
| --------------- | ---------------------------------------------- |
| `gpu/`          | **GPU computing with VecTec**                  |
| `hello.tec`     | Hello World                                    |
| `fibonacci.tec` | Recursive fibonacci                            |
| `structs.tec`   | Structs and methods                            |
| `enums.tec`     | Enums and control flow                         |
| `pointers.tec`  | Pointers and pass by reference                 |
| `arrays.tec`    | Arrays and iteration                           |
| `imports/`      | Multi-file programs with imports               |
| `casting.tec`   | Type casting                                   |
| `file_editing/` | File I/O                                       |

Run all examples:

```
chmod +x run_tests.sh
./run_tests.sh
```

## Standard library

Imported with:

```
!import(std.tec) as std;
```

| Function                                             | Description                          |
| ---------------------------------------------------- | ------------------------------------ |
| `std.print()`                                        | Print to standard output             |
| `std.print_newline()`                                | Print a newline                      |
| `std.read_int()`                                     | Read an integer from stdin           |
| `std.read_float()`                                   | Read a float from stdin              |
| `std.sqrt(float x)`                                  | Square root                          |
| `std.exit(int code)`                                 | Exit the program                     |
| `std.concat(string a, string b)`                     | Concatenate two strings              |
| `std.string_length(string s)`                        | Get string length                    |
| `std.string_equals(string a, string b)`              | Compare strings (returns 1 if equal) |
| `std.string_substring(string s, int start, int end)` | Extract substring                    |

## Editor support

| Editor  | Plugin                                                                | Status      |
| ------- | --------------------------------------------------------------------- | ----------- |
| VS Code | [techlang-vscode](https://open-vsx.org/extension/gummyniki/techlang)  | ✅ Available |
| Neovim  | [techlang-nvim](https://github.com/gummyniki/techlang-nvim)           | ✅ Available |

## Roadmap

- [x] Core language: types, arrays, pointers, structs, enums, control flow
- [x] Compiles to native binaries via LLVM
- [x] File imports and standard library
- [x] Helpful error messages
- [x] **VecTec: GPU compute companion language (CUDA/PTX)**
- [x] Editor plugins (VS Code, Neovim)
- [ ] AMD and Intel GPU support (SPIR-V)
- [ ] Language Server Protocol (LSP)
- [ ] Package manager


## Examples
Examples are available in the examples/ directory, for more complex ones, using different libraries check out *[this repo](github.com/gummyniki/techlang-libs)*

## Contributing

Contributions are very welcome! If you find a bug or want to add a feature:

1. Fork the repository
2. Create a branch: `git checkout -b my-feature`
3. Make your changes
4. Run the test suite: `./run_tests.sh`
5. Open a pull request

Please open an issue first for large changes so we can discuss the approach.

## License

MIT — see [LICENSE](LICENSE) for details.
