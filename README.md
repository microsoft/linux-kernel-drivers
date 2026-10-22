# Microsoft MANA out-of-tree driver

| Directory | Package | What it is |
|-----------|---------|------------|
| [`mana/`](mana/README.md) | `mana-compat` | MANA (Microsoft Azure Network Adapter) ethernet driver — one verbatim upstream copy that builds across Linux LTS kernels via a configure-driven glue layer |

See the README in subdir for what it provides, how it works, and how to build,
install, and maintain it.

## Support plan
* Supported Linux Distributions: The driver is supported only on Linux distributions listed here, https://learn.microsoft.com/en-us/azure/virtual-network/accelerated-networking-overview?tabs=NetworkManager#for-virtual-machines-vms-with-the-microsoft-mana-adapter, which running LTS kernels (currently 5.15, 6.1, 6.6, 6.12, 6.18).
* Kernel Requirements: Support extends only to clean, non-End-of-Life (EOL) Linux Long-Term Support (LTS) kernels.
* If you are running a modified custom kernel or an unsupported/EOL Linux operating system, Microsoft will require you to reproduce any bugs on a clean, supported LTS kernel & distro before we will troubleshoot the driver for you.

## Code of Conduct
This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct).
