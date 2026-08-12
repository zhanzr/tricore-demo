# AURIX Multi-Board Demo Workspace

Using AURIX Development Studio (ADS).

## Boards

| Folder          | Board                              |
|-----------------|------------------------------------|
| `appkit-tc275/` | Application Kit TC2X5 V2.0 (TC275) |
| `appkit-tc234/` | Application Kit TC2X4 (TC234)      |
| `tc212-kit/`    | Application Kit TC212 (TC21x)      |

Each board folder contains per-project build setups. See the per-board
READMEs for board-specific details.

## Build Options

### TASKING (AURIX Studio IDE)

The `appkit-tc275` projects are configured for the **TASKING VX-toolset**
shipped with ADS. The free edition only runs the compiler when launched
**by the IDE** — building from a plain command line fails with
`License does not support running as standalone`. Use **Project -> Build** in
the IDE.

### GCC (standalone, no license restriction)

The AURIX GCC toolchains (`tricore-elf-gcc`) run standalone. Every project
builds the shared `Libraries/` sources plus the project sources into a `.hex`
for flashing:

- **`tc212-kit`**: each project has a `Makefile`. From a bash shell
  (MSYS2 `C:\msys64\usr\bin\bash.exe` preferred, else Git for Windows bash),
  from any project folder:

  ```
  make hex      # build build/<proj>.hex
  make flash    # program build/<proj>.hex via AURIXFlasher
  make size     # print section sizes
  make clean    # remove build/
  ```

  See [tc212-kit/README.md](tc212-kit/README.md).
- **`appkit-tc234`**: each project has a `build_<proj>.sh` script. Run the
  project's script from a bash shell (or the older `build_<proj>.ps1` from
  PowerShell). See [appkit-tc234/README.md](appkit-tc234/README.md).

## Board Readmes

- [appkit-tc275/README.md](appkit-tc275/README.md)
- [appkit-tc234/README.md](appkit-tc234/README.md)
- [tc212-kit/README.md](tc212-kit/README.md)
