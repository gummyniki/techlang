# Techlang 🔧

[![Release](https://img.shields.io/github/v/release/gummyniki/techlang?include_prereleases)](https://github.com/gummyniki/techlang/releases)

### [Website](https://gummyniki.github.io/techlang-website/)

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
   Hello, Alice



## Installation

**Quick install (Linux x64):**
```bash
wget https://github.com/gummyniki/techlang/releases/download/v0.1.0-alpha/techlang-linux-x64.tar.gz
tar -xzf techlang-linux-x64.tar.gz
cd techlang-linux-x64 && ./install.sh
```

### Requirements
- LLVM 17+
- GCC
- CMake 3.20+
- A Linux or macOS system
- If you're using VecTec, the CUDA toolkit

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
| `casting.tec` | Casting a variable's type to another |
| `file_editing/` | File I/O |
| `gpu/` | Examples showcasing GPU computing using VecTec |

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
| `std.print()` | Print to the standard output |
| `std.print_newline()` | Print a newline |
| `std.read_int()` | Read an integer from stdin |
| `std.read_float()` | Read a float from stdin |
| `std.sqrt(float x)` | Square root |
| `std.exit(int code)` | Exit the program |
| `std.concat(string a, string b)` | Concatenate two strings |
| `std.string_length(string s)` | Get string length |
| `std.string_equals(string a, string b)` | Compare strings (returns 1 if equal) |
| `std.string_substring(string s, int start, int end)` | Extract substring |



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
- [x] More standard library functions
- [x] VS Code syntax highlighting
- [ ] Language Server Protocol (LSP)
- [ ] Package manager
- [x] GPU compute companion language

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
