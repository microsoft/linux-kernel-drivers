# MANA out-of-tree / DKMS driver (`mana-compat`)

An out-of-tree, DKMS-installable build of Microsoft's MANA (Microsoft Azure
Network Adapter) ethernet driver that builds and loads across a range of kernels
**from a single, verbatim copy of the upstream MANA driver** (currently vendored
from tip, **v7.1-rc7**; see `source-version`).

The design goal is a **captive, naive driver** paired with a **rich glue layer**:

* `driver/` is a byte-for-byte snapshot of the in-tree MANA driver. It is *never*
  edited for compatibility; newer upstream code arrives by re-vendoring that
  snapshot wholesale from a pinned Linux tree.
* `compat/` is the glue. It is force-included into every translation unit and
  shims the kernel-version differences so the captive body compiles unchanged.
* `configure` pushes the complexity to **build time**: it compile-tests the
  target kernel before every build and records exactly which kernel features are
  present, so the glue adapts to the *actual* kernel rather than guessing from a
  version number.

The approach follows the kernel-backports / ZFS-autoconf / Amazon-ENA model:
detect features by compiling against the target kernel, fill only the gaps, and
never reimplement driver logic in the glue.

## Supported kernels

Explicitly supported: **5.15, 6.1, 6.6, 6.12, 7.0**. Intermediate releases
(6.8, 6.11, …) work without extra effort because detection is feature-based, not
version-pinned — as do distro kernels that backport features ahead of mainline
(Azure/Canonical/RHEL), since `configure` compile-tests the real headers.

Supported architectures follow upstream MANA: **x86_64** and little-endian
**arm64**. The package is source-only (`all`/`noarch`); DKMS compiles a distinct
module for each target kernel and architecture.

Runtime-validated on every current upstream Linux LTS from **5.15 through
6.18** — five kernel.org LTS tarballs (5.15.212, 6.1.178, 6.6.145, 6.12.97,
6.18.40) built from source with `CONFIG_MICROSOFT_MANA=m` and driven end-to-end
on an Azure VM with MANA hardware. The ethtool conformance suite reports
**PASS 36 / SKIP 1 / FAIL 0** on all five. Also validated on distro derivatives
of two of those LTS lines: **Debian 13** (6.12), **Azure Linux 3** (6.6). Also
builds cleanly on **Ubuntu Jammy** (Azure 6.8) and **Ubuntu Noble** (Azure
6.17), but those distro kernels ship `mana=y` (compiled-in) so the OOT module
cannot replace the in-tree driver at runtime without a custom kernel image.

## Layout

```
mana/
├─ driver/                         VERBATIM upstream MANA driver (do not edit)
│  ├─ *.c                          gdma_main, hw_channel, mana_bpf, mana_en,
│  │                               mana_ethtool, shm_channel
│  └─ include/net/mana/*.h         gdma.h, hw_channel.h, mana.h,
│                                  mana_auxiliary.h, shm_channel.h
├─ compat/
│  ├─ mana_compat.h                umbrella, force-included via Kbuild -include
│  ├─ autogen/                     ./configure writes mana_compat_autoconf.h here
│  ├─ mana_compat_features.h       rejects missing/incomplete generated maps
│  ├─ mana_compat_export.h         EXPORT_SYMBOL_NS string-namespace shim
│  ├─ mana_compat_info.c           MODULE_VERSION stamp (DKMS package version)
│  ├─ mana_compat_privflags.{c,h}  out-of-tree ethtool private-flags control plane
│  ├─ mana_compat_{alloc,kernel,cleanup,cpumask,pci,netdev,ethtool,xdp}.h
│  └─ backport-include/net/        header OVERLAYS resolved before LINUXINCLUDE
│     ├─ net_shaper.h              (so verbatim `#include <net/...>` keeps
│     ├─ netdev_lock.h              working on kernels that lack the header)
│     └─ page_pool/helpers.h
├─ configure                       POSIX sh: compile-test probes -> autoconf.h
├─ scripts/
│  ├─ gen-version.sh               derive <internal>+linux<source>+compat<rev>
│  ├─ install.sh                   universal DKMS installer (POSIX sh, distro-aware)
│  └─ uninstall.sh                 symmetric removal path
├─ source-version                  provenance: the Linux source driver/ came from
├─ Kbuild                          kbuild entry (include order, object list)
├─ Makefile                        host wrapper: runs configure, then kbuild
└─ dkms.conf                       single source of truth for PACKAGE_VERSION
```

## How the glue works

1. **Force-include, never edit the body.** `Kbuild` adds
   `-include compat/mana_compat.h` to every TU. The umbrella pulls in the
   feature map, the export shim, and the per-area shims. The driver sources stay
   pristine.

2. **Feature map: mandatory and regenerated.** The checked top-level `Makefile`
   removes any previous map, runs `configure` against the target kernel, and
   publishes a complete `compat/autogen/mana_compat_autoconf.h` before invoking
   kbuild. The umbrella includes that file directly, and
   `mana_compat_features.h` rejects an incomplete map. There is deliberately no
   version fallback: guessing "absent" can redefine an API that a distro
   backported into an older kernel release. Direct kbuild invocations are
   rejected so staging cannot bypass the checked wrapper.

3. **Vendored headers and overlays shadow the kernel's.** Two dirs sit on
   `NOSTDINC_FLAGS`, ahead of `LINUXINCLUDE`:
   - `driver/include/net/mana/*.h` — the vendored driver headers. The captive
     driver must compile against its own `<net/mana/*.h>`, never the kernel's:
     mainline carries `include/net/mana/` since v6.6 (and some backport trees
     earlier, e.g. Debian's 6.1 cloud kernel), so without this the verbatim body
     would build against the kernel's mismatched copy. They go on
     `NOSTDINC_FLAGS`, not `ccflags-y`, which lands *after* `LINUXINCLUDE`.
   - `backport-include/` — overlays for headers some `#include <net/...>` lines
     name but that older kernels lack (`net/net_shaper.h`, `net/netdev_lock.h`,
     `net/page_pool/helpers.h`). Each uses `#include_next` to reach the real
     header when present, else supplies the missing surface. They gate on
     `MANA_HAVE_*`, not `__has_include` (always true once the overlay dir is on
     the path).

4. **What headers can't express** lives as small, additive, marked hunks in
   `mana_ethtool.c`: the ethtool ops-table signatures and private-flags wiring,
   the legacy `get_rxnfc` ring-count fallback, and the `rx_cqe_frames`
   coalesce entries. Each is tagged `MANA-COMPAT(...)` so it stays easy to spot
   in the otherwise verbatim source, and is re-applied on every re-vendor.
   They add lines only. `mana_en.c` is byte-identical to upstream on every
   supported kernel — the `.net_shaper_ops` registration and the
   `page_pool_params` version-varying fields are hidden by shims in the
   `<net/net_shaper.h>` and `<net/page_pool/helpers.h>` overlays. The
   net_shaper shim uses GCC's `-fplan9-extensions` (enabled in `Kbuild`) so
   the outer shim struct implicitly converts to the kernel-visible
   `struct net_device_ops` at the assignment site.

### RDMA / InfiniBand (`mana_ib`) — not yet in scope

`configure` enforces upstream's target constraints (x86_64 or little-endian
arm64, `CONFIG_PCI_MSI`, and `CONFIG_PCI_HYPERV`) and hard-requires the selected
`CONFIG_AUXILIARY_BUS` and `CONFIG_PAGE_POOL` facilities. It fails early with a
clear message if the target kernel lacks any of them.

The captive `mana_en.c` keeps its auxiliary-bus RDMA plumbing and registers an
`rdma` auxiliary device on every supported kernel. But the InfiniBand driver that
binds it, `mana_ib`, is **not part of this package**, and the install blacklists
it. The auxiliary device is named after the module: built as `mana-compat.ko` it
is `mana_compat.rdma`, while in-tree `mana_ib` matches only `mana.rdma`, so it
would not bind even if loaded. RDMA is therefore unavailable with the out-of-tree
module today.

> **Action item — pull `mana_ib` into scope.** Either vendor an out-of-tree
> `mana_ib` whose match table targets `mana_compat.rdma`, or build the Ethernet
> module under the `mana` name (which trades away the blacklist-based coexistence
> that lets `mana-compat` take over the device). Tracked as future work.

## Detected features (`MANA_HAVE_*`)

`configure` emits one `#define MANA_HAVE_x 0|1` per feature below. The first
mainline release is shown only for historical reference; detection always uses
the target kernel.

| Flag | First mainline | What it gates |
|------|------|---------------|
| `BPF_WARN_XDP_3ARG`            | 5.17 | 3-arg `bpf_warn_invalid_xdp_action()` |
| `NETIF_NAPI_ADD_WEIGHT`        | 6.1  | `netif_napi_add_weight()` |
| `PAGE_POOL_HELPERS`            | 6.6  | `net/page_pool/helpers.h` split out |
| `PAGE_POOL_PARAMS_NAPI`        | 6.6  | `page_pool_params.napi` (fed via the helpers.h overlay shim) |
| `PP_FLAG_PAGE_FRAG`            | ≤6.6 | page-pool fragment opt-in flag (present until removed in 6.7; the wrapper in the helpers.h overlay sets it when `.max_len` is non-zero, so `mana_create_page_pool()` doesn't mention the macro) |
| `PAGE_POOL_PARAMS_NETDEV`      | 6.12 | `page_pool_params.netdev`/`.queue_idx` (fed via the helpers.h overlay shim) |
| `PAGE_POOL_FREE_VA`            | 6.12 | `page_pool_free_va()` |
| `EXPORT_SYMBOL_NS_STRING`      | 6.13 | string-literal namespace form |

`configure` and `mana_compat_features.h` carry the same set of feature flags and
must stay in lockstep.

## Versioning

The package version ties the driver's own version to the upstream source it was
vendored from, plus an out-of-tree packaging revision:

```
<MANA internal>+linux<source>+compat<REV>     e.g.  0.1.1+linux7.1.0~rc7+compat3
```

* **`<MANA internal>`** — `MANA_{MAJOR,MINOR,MICRO}_VERSION` from the captive
  `driver/include/net/mana/mana.h`: the version the driver reports to the device,
  verbatim upstream.
* **`<source>`** — `LINUX_SOURCE_VERSION` from `source-version`, recorded when
  `driver/` was re-vendored, taken from that kernel tree
  (`VERSION.PATCHLEVEL.SUBLEVEL` plus any `EXTRAVERSION`). A pre-release such as
  `-rc7` maps to `~rc7` so the candidate sorts *below* the eventual release under
  `dpkg`/`rpm`; a released source is unaffected.
* **`+compat<REV>`** — `PACKAGE_REVISION` from `package-revision`, the one
  component the packaging owns. The upstream-derived halves do not move for a
  change touching only `compat/` or packaging, but DKMS keys its build tree,
  cached `.ko` and upgrade decisions on `PACKAGE_VERSION` — so without a moving
  component the **stale module stays loaded** on upgrade. Bump `+compat<REV>`
  (`make bump-revision`) for those changes. It is placed last so upstream
  identifiers dominate ordering: a re-vendor to a newer kernel always outranks
  any `+compat` revision of an older base.

All three components come from authoritative sources, so the version can never
silently drift. `scripts/gen-version.sh` computes it; `dkms.conf`'s
`PACKAGE_VERSION` is the single stored copy. `Kbuild` stamps that same string
into the module via `compat/mana_compat_info.c` (the captive body carries no
`MODULE_VERSION` of its own), so `modinfo mana-compat | grep ^version` is a quick
post-upgrade staleness check.

```sh
make version          # print the derived version (0.1.1+linux7.1.0~rc7+compat3)
make set-version      # write it into dkms.conf (run after a re-import)
make bump-revision    # ++PACKAGE_REVISION then set-version (compat/packaging-only change)
```

When to bump which:

* **Re-vendor** a newer driver/kernel: `source-version` is re-stamped (and
  `mana.h` carries any driver bump), so the version moves on its own. Reset
  `package-revision` to `1` manually if the source advanced.
* **Compat-only or packaging-only change** (a new shim, a marked-hunk tweak,
  `Kbuild`/`configure` edits): run `make bump-revision`.

## Building and installing

### Universal DKMS installer (recommended)

One artifact that works on every supported LTS kernel and every distro with
(or that can install) DKMS — Debian, Ubuntu, RHEL, Fedora, Azure Linux, SUSE,
Arch. DKMS is source-only so a single tarball built once covers every current
LTS from 5.15 through 6.18+ plus any future kernel that ships on the target.

Build on any Linux host with `make` and `tar`:

```sh
cd mana
make tarball
# -> mana-compat-installer-<version>.tar.gz (~124 KB)
```

Install on the target:

```sh
tar xzf mana-compat-installer-*.tar.gz
sudo sh install.sh
sudo reboot
```

`install.sh` (a POSIX-sh script) does everything the DKMS workflow needs:

1. Detects the distro family via `/etc/os-release`
   (`debian` / `rpm` / `suse` / other).
2. Installs `dkms`, `kernel-devel` (or `linux-headers-$(uname -r)`),
   `gcc`, `make`, and `binutils` if any are missing — using the local
   package manager (`apt` / `dnf` / `tdnf` / `yum` / `zypper`).
3. Stages the source tree under `/usr/src/mana-compat-<version>/` and
   runs `dkms add / build / install` for the running kernel. The build
   goes through the checked top-level `Makefile`, which re-runs
   `configure` against `${kernel_source_dir}` so probe failures are
   build failures even on older DKMS releases that ignore failed
   `PRE_BUILD` hooks.
4. Installs `/etc/modprobe.d/50-mana-compat.conf` to blacklist the
   in-tree `mana` and `mana_ib`.
5. Installs `/etc/modules-load.d/mana-compat.conf` so `mana-compat`
   autoloads at boot.
6. Regenerates the initramfs (`update-initramfs -u`, `dracut --force`,
   or `mkinitcpio -p linux`, whichever the target uses) so the blacklist
   also holds at early boot — on the Debian default `MODULES=most` the
   in-tree `mana.ko` ships in the initramfs and would otherwise bind
   before the rootfs `/etc/modprobe.d` is ever read.

Reboot is required so the blacklist takes effect and `mana-compat` binds
first, before the in-tree driver can claim the device. To remove:

```sh
sudo sh uninstall.sh
sudo reboot
```

`uninstall.sh` reverses every step: `dkms remove` for each installed kernel,
deletes `/usr/src/mana-compat-*`, removes the blacklist and autoload files,
regenerates the initramfs.

Verify a successful install:

```sh
dkms status mana-compat        # -> mana-compat/<ver>, <kernel>: installed
lsmod | grep mana              # -> mana_compat
cat /sys/module/mana_compat/version
```

### Manual (on the target machine)

Skip the installer and drive the build yourself:

```sh
cd mana
make                # runs ./configure against /lib/modules/$(uname -r)/build,
                    # then builds mana-compat.ko
sudo modprobe -r mana_ib mana 2>/dev/null || true
sudo insmod mana-compat.ko
```

Every `make` re-probes. Use `make config KDIR=...` to generate only the feature
map for diagnostics, `make clean` to remove build artifacts and that map, or
`make distclean` to also drop the built installer tarball.

For a persistent install (blacklist + initramfs regen + autoload) via this
route, replicate what `install.sh` does — but really, use `install.sh`.

### Target-side prerequisites

DKMS builds the module on the target, so the target needs a compiler
(`gcc`), `make`, `binutils`, `dkms`, an initramfs tool
(`dracut` on RPM distros, `initramfs-tools` on Debian/Ubuntu, `mkinitcpio`
on Arch), and the **kernel-devel / linux-headers package that matches the
running kernel**. `install.sh` installs them all automatically if missing;
if you install by hand, install them by hand first.

### Distro-specific pitfalls to know about

> **Azure Linux 3: disable kernel lockdown once**
>
> Azure Linux 3 ships with `lockdown=integrity` on the default kernel command
> line, which blocks unsigned modules — including DKMS-built ones — at
> `insmod`/`modprobe` with `module verification failed: signature and/or
> required key missing`. You will see `Lockdown: insmod: unsigned module
> loading is restricted` in `dmesg`. Choose one of:
>
> ```sh
> # Option A — disable lockdown (simplest, one-time change):
> sudo sed -i 's/lockdown=[a-z]*/lockdown=none/' /etc/default/grub
> sudo grub2-mkconfig -o /boot/grub2/grub.cfg
> sudo reboot
> # Option B — enroll a MOK and sign the module (per-build, more work).
> ```
>
> Once `/proc/cmdline` shows `lockdown=none`, `modprobe mana-compat` (or the
> auto-load path via `/etc/modules-load.d/`) works normally.

> **In-tree `mana` rmmod pitfall on Azure Linux 3 (kernel 6.6.x)**
>
> The Azure Linux 3 in-tree `mana` module (as of `6.6.144.1-1.azl3`) has an
> unload-path bug: after the VF has been active, `rmmod mana` NULL-derefs in
> `mana_destroy_rxq → debugfs_remove → down_write` and takes the CPU down. Do
> **not** hot-swap with `modprobe -r mana` on Azure Linux 3. Use the
> blacklist + reboot path — `install.sh` already sets that up for you.

> **Experimental prototype.** This distribution and the driver itself are
> an unsupported prototype, for development and testing only.

If `dkms build` failed during install, the kernel build tree was probably
missing. Install it for the running kernel and retry (build log lives at
`/var/lib/dkms/mana-compat/<ver>/build/make.log`):

```sh
# Debian / Ubuntu:
sudo apt-get install linux-headers-$(uname -r)
# RHEL / Fedora / Azure Linux:
sudo tdnf install kernel-devel-$(uname -r)   # or: dnf install kernel-devel-$(uname -r)
sudo dkms autoinstall
```
