## i386 Emulator

### Overview
This project is an i386 Emulator that models a subset of the Intel IA-32 architecture. It follows 
the standard pipeline--fetch, decode, execution-- and it also includes:
- memory access
- interrupt-driven I/O
- memory-mapped devices
- protected-mode segmentation
- a terminal-based UI for live visualization

An emphasis was made on behavior closely reflecting hardware and deterministic execution, allowing interactive programs and experimentation at a low level.

For more information, please view the documentation linked below:
[i386 Documentation](https://www.dropbox.com/scl/fi/vqe0mvkwmbr20an8nkyhh/i386-Documentation.pdf?rlkey=yp1v6flpy0obzvwiv8hspm0et&st=vm8mrros&dl=0)

### Dependencies
- C compiler with C11 support
  - macOS: Apple Clang via Xcode Command Line Tools
- ncurses (wide-character support is required). The UI uses wide-character drawing via ncurses; ensure that your terminal supports Unicode.
  
### Build and Run
#### To install ncurses:
> Native Windows is not supported. Windows users should use **WSL**

**WSL or Linux:**
```bash
sudo apt install libncursesw5-dev
```

**MAC:**
```bash
brew install ncurses
```

#### Build
From project root:
```bash
make
```

#### Run
```
./c --program example/program.txt --ui
```

### Optional: Prebuilt Binaries
Binaries for both Linux/WSL and macOS are provided for convenience and can be found under GitHub Releases.
