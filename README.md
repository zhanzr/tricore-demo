# AURIX Multi-Board Demo Workspace
Using AURIX Development Studio.

## Boards

| Folder          | Board                                   |
|-----------------|-----------------------------------------|
| `appkit-tc275/` | Application Kit TC2X5 V2.0 (TC275)      |
| `appkit-tc234/` | Application Kit TC234 (placeholder)     |

Each board folder contains Eclipse projects. Projects build inside AURIX
Development Studio (TASKING compiler), not from the command line.

## Build System Notes

### TASKING Compiler License Limitation
This project uses the **TASKING VX-toolset for AURIX Development Studio (non-commercial)** shipped with ADS at:
`tools/Compilers/Tasking_1.1r8/ctc/bin/`

The free edition enforces a runtime check via `tlm.dll` (Tasking License Manager) loaded by the ADS Eclipse process. The compiler only executes when spawned **by the ADS IDE** — standalone use from command line (`make`, shell scripts, etc.) is blocked with:

> `License does not support running as standalone. Please use the integrated development environment.`

**Workarounds:**
- Build via ADS GUI (**Project → Build Project** or **Ctrl+B**)
- Use headless Eclipse (`AURIX-studioc.exe`) — requires fixing the `-perspective` argument injection and resolving CDT manifest conflicts
- Switch to a GCC-based Tricore toolchain (no license restrictions, but requires updating compiler flags, linker scripts, and some TASKING-specific builtins/attributes in the codebase)

## Board Readmes

- [appkit-tc275/README.md](appkit-tc275/README.md)
- [appkit-tc234/README.md](appkit-tc234/README.md)
