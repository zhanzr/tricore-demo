# AURIX Multi-Board Demo Workspace

Using AURIX Development Studio.

## Boards

| Folder          | Board                              |
|-----------------|------------------------------------|
| `appkit-tc275/` | Application Kit TC2X5 V2.0 (TC275) |
| `appkit-tc234/` | Application Kit TC2X4 (TC234)      |

Each board folder contains Eclipse projects. See the per-board READMEs for
build/flash details.

## Build Options

### TASKING (AURIX Studio IDE)

The projects are configured for the **TASKING VX-toolset** shipped with ADS.
The free edition only runs the compiler when launched **by the IDE** —
building from a plain command line fails with
`License does not support running as standalone`. Use **Project → Build** in
the IDE.

### GCC (standalone, no license restriction)

The AURIX GCC toolchains (`tricore-elf-gcc`) run standalone. Each board has
build scripts that compile all `Libraries/` sources plus the project sources
and produce a `.hex` for flashing:

```
powershell -ExecutionPolicy Bypass -File appkit-tc234\blink_hello\build_blink_hello.ps1
```

See [appkit-tc234/README.md](appkit-tc234/README.md) for the full build/flash
workflow (GCC linker script, defines, flash via `AURIXFlasher.exe`).

## Board Readmes

- [appkit-tc275/README.md](appkit-tc275/README.md)
- [appkit-tc234/README.md](appkit-tc234/README.md)
