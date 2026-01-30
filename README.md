## i386 Emulator

### Overview
This i386 Emulator implements an execution model of a subset of the Intel IA-32 architecture. It follows 
the standard pipeline: fetch, decode, execution; it also includes memory access, 
interrupt-driven I/O, memory-mapped devices, and protected-mode segmentation to enable interactive programs. 
An emphasis was made on behavior closely reflecting hardware and deterministic execution.

For more information, please view the documentation linked below:

### Dependencies
- C compiler with C11 support
- ncurses library for UI mode
  
### Build and Run
#### To install ncurses:
If you are a Windows User, please install Windows Subsystem for Linux (WSL) in order to install ncurses

**WSL or Linux:**
```bash
sudo apt install libncurses-dev
```
**MAC:**
```bash
brew install ncurses
```
#### Build
make

#### Run
./c --program example/program.txt --ui
