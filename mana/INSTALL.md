# Installing and running mana-compat

Zero-to-one steps to install the pre-built `mana-compat` DKMS installer (an
experimental prototype) on a clean target and confirm the driver is running.
To build the installer tarball yourself, see [README.md](README.md) →
*Building and installing*.

`mana-compat` is the out-of-tree MANA Ethernet driver. Distribution is
**source-only**: a single universal `.tar.gz` ships the DKMS source tree plus
a distro-aware `install.sh`, and DKMS compiles the module on the target
against the running kernel. `install.sh` also blacklists the in-tree
`mana` / `mana_ib`, registers the module with DKMS, and rebuilds the
initramfs — so a **reboot is required** for the out-of-tree driver to claim
the device (the in-tree `mana` is bundled in the initramfs and binds at
early boot until the rebuilt initramfs carries the blacklist).

## Any Linux distro with (or that can install) DKMS

```sh
# 1. Extract the installer tarball you were shipped.
tar xzf mana-compat-installer-*.tar.gz

# 2. Run the installer as root. It will:
#     - detect the distro family (debian / rpm / suse),
#     - install dkms + kernel-devel + gcc + make + binutils if any are missing,
#     - dkms add / build / install for the running kernel,
#     - install /etc/modprobe.d/50-mana-compat.conf (blacklist),
#     - install /etc/modules-load.d/mana-compat.conf (autoload),
#     - regenerate the initramfs.
sudo sh install.sh

# 3. Reboot.
sudo reboot
```

If the kernel build tree for the running kernel is not installed and the
installer's auto-install of headers fails, `dkms build` will error out.
Install the headers by hand and rerun `install.sh` — it is idempotent:

```sh
# Debian / Ubuntu:
sudo apt-get install linux-headers-$(uname -r)
# RHEL / Fedora / Azure Linux:
sudo tdnf install kernel-devel-$(uname -r)   # or: dnf install ...
sudo sh install.sh
```

## Distro-specific gotcha to know

**Azure Linux 3** ships with `lockdown=integrity` on the default kernel
command line, which refuses unsigned modules. DKMS-built modules are unsigned
by default, so `modprobe mana-compat` will fail with
`module verification failed: signature and/or required key missing`. Disable
lockdown once:

```sh
sudo sed -i 's/lockdown=[a-z]*/lockdown=none/' /etc/default/grub
sudo grub2-mkconfig -o /boot/grub2/grub.cfg
sudo reboot
```

Then rerun `install.sh` if it errored before. The alternative is MOK signing
(harder — see [README.md](README.md)).

## Confirm it is running

```sh
dkms status mana-compat        # installed, for the running kernel
lsmod | grep mana_compat       # module is loaded (mana_compat, not mana)
cat /sys/module/mana_compat/version
```

Identify the MANA interface and inspect it:

```sh
ip -br link                    # list interfaces; the MANA data NIC is one of them
ethtool -i <iface>             # shows the bound driver and PCI bus-info
```

Bring it up as usual (`sudo ip link set <iface> up`, then DHCP or a static
address) and run your traffic test.

## Optional: RX CQE coalescing

MANA supports a device-level option that batches 4 RX completions into a
single CQE, at a little added latency but with fewer interrupts under heavy
RX. It is **off** by default. The OOT driver exposes it two ways so a single
procedure works across every supported kernel:

**Recommended, portable across every kernel from 5.15 through 6.18+ (and 7.1+):**

```sh
ethtool --show-priv-flags <iface>                            # show current state
sudo ethtool --set-priv-flags <iface> rx_cqe_coalesce_4 on   # enable
sudo ethtool --set-priv-flags <iface> rx_cqe_coalesce_4 off  # disable
```

**Native ethtool coalesce interface (kernels < v7.1 use the legacy field,
kernels ≥ v7.1 use the new `rx-cqe-frames` field):**

```sh
# On <v7.1:
sudo ethtool -C <iface> rx-frames 4          # enable  (4-deep CQE coalescing)
sudo ethtool -C <iface> rx-frames 1          # disable (1-deep, no coalescing)
# On ≥v7.1 (if we want native):
sudo ethtool -C <iface> rx-cqe-frames 4      # enable
sudo ethtool -C <iface> rx-cqe-frames 1      # disable
# Either kernel: read current state
ethtool -c <iface>
```

Both interfaces toggle the same underlying device state
(`mana_port_context::cqe_coalescing_enable`) — you can read it back through
either route regardless of which one set it. The device only supports 1 or 4,
so any other value returns `netlink error: Invalid argument`. Setting applies
live (the port must be up) and is per-port runtime state — it resets to *off*
on module reload or reboot, so set it per test run.

## Removal

```sh
tar xzf mana-compat-installer-*.tar.gz   # if you no longer have the extracted tree
sudo sh uninstall.sh                     # dkms remove, blacklist off, initramfs regen
sudo reboot                              # restores the in-tree driver
```

## Notes

- The loaded module is **`mana_compat`** (a distinct `.ko`), intentionally not
  named `mana`, so the in-tree blacklist does not catch it. It binds the same
  PCI device the in-tree driver would.
- **Ethernet only** — RDMA / InfiniBand (`mana_ib`) is not included.
