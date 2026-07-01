# BMap

Bernardo's port scanner.

## Features

- TCP port scanning
- ARP-based host discovery
- Subnet scanning
- Configurable target lists and port lists (via `assets/ports.csv` or CLI args)
- Fast, minimal dependencies

## Structure

```
├── assets/
│   └── ports.csv           # Default list of ports to scan
└── src/
    ├── main.c
    ├── data/
    │   ├── args.c/h        # CLI argument parsing
    │   └── csv.c/h         # CSV parsing for ports/targets
    ├── model/
    │   ├── network_info.h  # Network metadata structs
    │   ├── port_status.h   # Port result structs
    │   └── string_list.h   # String list utility
    ├── net/
    │   ├── arp.c/h         # ARP host discovery
    │   ├── subnet.c/h      # Subnet calculation
    │   └── tcp.c/h         # TCP connection logic
    └── scanning/
        ├── ports.c/h       # Port scan orchestration
        └── targets.c/h     # Target resolution
```

## Requirements

- GCC or Clang
- Linux or macOS (raw socket support required)
- Root/sudo privileges (needed for ARP and raw TCP operations)

## Build

```bash
make
```

The compiled binary will be placed in the project root (or as configured in the `Makefile`).

## Usage

```bash
sudo ./scanner [options]
```

Common options (update based on your actual CLI):

| Flag | Description |
|------|-------------|
| `-t <target>` | Target IP, hostname, or subnet (e.g. `192.168.1.0/24`) |
| `-p <ports>` | Ports to scan (e.g. `80,443,8080`) |

## Notes

- Raw socket operations require root. Run with `sudo`.
- `assets/ports.csv` contains a default set of well-known ports scanned when no `-p` flag is given.
- This tool is intended for use on networks you own or have permission to scan.
