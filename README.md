# Techlang 🔧

A compiled, statically typed programming language with familiar C-like syntax
and modern features. Techlang compiles directly to native binaries via LLVM.

> ⚠️ Techlang is currently in **alpha**. Expect bugs and missing features.
> Contributions and feedback are welcome!

## Why Techlang?

- **Familiar syntax** — if you know C, C++, or Java you'll feel right at home
- **Statically typed** — catch errors at compile time, not at runtime
- **Fast** — compiles to native binaries via LLVM with full optimization support
- **Modern features** — structs, enums, pointers, imports, and more
- **Simple module system** — split code across files with `!import`
- **Coming soon** — a GPU compute companion language for parallel workloads

## Quick Example

```techlang
!import(std.tec) as std;

struct person = {
    int age;
    string name;
}

function greet(person p) returns none {
    std.print_string("Hello, ");
    std.print_string(p.name);
}

function main() returns none {
    person p;
    p.age = 25;
    p.name = "Alice";
    greet(p);
}
```

Output:
   Hello, Alice



## Installation

### Requirements
- LLVM 17+
- GCC
- CMake 3.20+
- A Linux or macOS system

### Build from Source

```bash
git clone https://github.com/gummyniki/techlang
cd techlang
mkdir build && cd build
cmake ..
make
```

The compiler binary will be at `build/techlang`.

Optionally add it to your PATH:
```bash
export PATH=$PATH:/path/to/techlang/build
```

### Compile a Techlang File

```bash
./build/techlang hello.tec
./hello
```

## Examples

The `examples/` directory contains several programs demonstrating Techlang's
features:

| Example | Description |
|---|---|
| `hello.tec` | Hello World |
| `fibonacci.tec` | Recursive fibonacci sequence |
| `structs.tec` | Structs and methods |
| `enums.tec` | Enums and control flow |
| `pointers.tec` | Pointer usage and pass by reference |
| `arrays.tec` | Arrays and iteration |
| `imports/` | Multi-file programs with imports |

Run all examples:
```bash
chmod +x run_tests.sh
./run_tests.sh
```

## Language Reference

See [LANGUAGE.md](docs/LANGUAGE.md) for the full syntax reference.

## Standard Library

Techlang comes with a standard library imported with:
```
import(std.tec) as std; // the std file has to actually exist in your current directory, you can copy it from examples/
```

| Function | Description |
|---|---|
| `std.print_int(int x)` | Print an integer |
| `std.print_float(float x)` | Print a float |
| `std.print_string(string s)` | Print a string |
| `std.print_char(char c)` | Print a character |
| `std.print_bool(bool b)` | Print a boolean |
| `std.print_newline()` | Print a newline |
| `std.read_int()` | Read an integer from stdin |
| `std.read_float()` | Read a float from stdin |
| `std.sqrt(float x)` | Square root |
| `std.cast_int_float(int x)` | Cast int to float |
| `std.cast_float_int(float x)` | Cast float to int |
| `std.cast_int_char(int x)` | Cast int to char |
| `std.exit(int code)` | Exit the program |


## Editor Support

| Editor | Plugin | Status |
|---|---|---|
| VS Code | [techlang-vscode](https://open-vsx.org/extension/gummyniki/techlang) | ✅ Available |
| Neovim | [techlang-nvim](https://github.com/gummyniki/techlang-nvim) | ✅ Available |

## Roadmap

- [x] Basic types (int, float, double, char, string, bool)
- [x] Arrays
- [x] Pointers
- [x] Structs
- [x] Enums
- [x] Functions
- [x] If/else, while, for loops
- [x] File imports
- [x] Standard library
- [x] Compiles to native binary
- [x] Helpful error messages
- [ ] `[fixed]` arrays
- [ ] More standard library functions
- [ ] VS Code syntax highlighting
- [ ] Language Server Protocol (LSP)
- [ ] Package manager
- [ ] GPU compute companion language *(coming soon)*

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
