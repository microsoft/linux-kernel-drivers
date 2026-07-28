#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Universal DKMS installer for the mana-compat out-of-tree driver.
#
# One artifact for every supported LTS kernel from 5.15 through 6.18+, on
# every distro that has (or can install) dkms -- Debian, Ubuntu, RHEL,
# Fedora, Azure Linux, SUSE, Arch. DKMS itself is what handles the per-
# kernel build; this script is just the outer wiring.
#
# Layout of the tarball this script lives in:
#   install.sh                    # this file
#   uninstall.sh                  # symmetric removal path
#   mana-compat-<version>/        # dkms source tree, ready for /usr/src/
#     dkms.conf, Kbuild, Makefile, configure, source-version,
#     package-revision, compat/, driver/, scripts/, README.md
#
# Usage:
#   sudo sh install.sh            # install
#   sudo sh uninstall.sh          # remove
#
# The script is idempotent: re-running it upgrades in place if the tarball
# version is newer than what dkms already tracks, or is a no-op otherwise.

set -eu

log()  { printf '%s\n' "  $*"; }
warn() { printf '\033[33mWARN\033[0m: %s\n' "$*" >&2; }
die()  { printf '\033[31mERROR\033[0m: %s\n' "$*" >&2; exit 1; }

[ "$(id -u)" -eq 0 ] || die "must run as root (try: sudo sh install.sh)"

HERE="$(cd "$(dirname "$0")" && pwd)"
SRCDIR=$(find "$HERE" -maxdepth 1 -type d -name 'mana-compat-*' -print -quit)
[ -n "$SRCDIR" ] && [ -d "$SRCDIR" ] || die "source tree not found; is this the extracted tarball?"

PKGVER=$(basename "$SRCDIR" | sed 's/^mana-compat-//')
[ -n "$PKGVER" ] || die "could not derive package version from $SRCDIR"
log "mana-compat installer, version $PKGVER"

# ---- 1. distro detection -------------------------------------------------

if [ -r /etc/os-release ]; then
    . /etc/os-release
    DISTRO_ID="${ID:-unknown}"
    DISTRO_LIKE="${ID_LIKE:-}"
else
    DISTRO_ID=unknown
    DISTRO_LIKE=
fi

case "$DISTRO_ID $DISTRO_LIKE" in
    *debian*|*ubuntu*)  FAMILY=debian ;;
    *rhel*|*fedora*|*centos*|*mariner*|*azurelinux*) FAMILY=rpm ;;
    *suse*|*opensuse*)  FAMILY=suse ;;
    *) FAMILY=unknown ;;
esac
log "distro: $DISTRO_ID  (family: $FAMILY)"

# ---- 2. install dkms + kernel-devel if missing --------------------------

_pkg_install() {
    case "$FAMILY" in
        debian)
            DEBIAN_FRONTEND=noninteractive apt-get update -qq
            DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends "$@"
            ;;
        rpm)
            if   command -v dnf  >/dev/null 2>&1; then dnf  install -y "$@"
            elif command -v tdnf >/dev/null 2>&1; then tdnf install -y "$@"
            elif command -v yum  >/dev/null 2>&1; then yum  install -y "$@"
            else die "no rpm package manager (dnf/tdnf/yum) found"
            fi
            ;;
        suse)
            zypper --non-interactive install "$@"
            ;;
        *)
            die "cannot auto-install packages on unknown distro; please install manually: $*"
            ;;
    esac
}

if ! command -v dkms >/dev/null 2>&1; then
    log "dkms not found, installing..."
    _pkg_install dkms
fi

# Some distros (notably Azure Linux 3) install the dkms binary but do not
# create /var/lib/dkms; dkms then bails out with "No write access to DKMS
# tree at /var/lib/dkms". Ensure the state directory exists.
mkdir -p /var/lib/dkms

# Kernel build tree for the currently-running kernel is required by DKMS.
KVER=$(uname -r)
if [ ! -d "/lib/modules/$KVER/build" ] && [ ! -e "/lib/modules/$KVER/build" ]; then
    log "kernel headers for $KVER missing, installing..."
    case "$FAMILY" in
        debian) _pkg_install "linux-headers-$KVER" ;;
        rpm)    _pkg_install "kernel-devel-$KVER"  || _pkg_install kernel-devel ;;
        suse)   _pkg_install "kernel-devel"        ;;
    esac
fi
[ -d "/lib/modules/$KVER/build" ] || [ -e "/lib/modules/$KVER/build" ] || \
    die "no kernel build tree at /lib/modules/$KVER/build after headers install"

# gcc / make / binutils are used by dkms build. Missing binutils in particular
# is a real trap on Azure Linux 3 (documented in README).
_ensure() {
    command -v "$1" >/dev/null 2>&1 || _pkg_install "$2"
}
_ensure gcc  gcc
_ensure make make
_ensure as   binutils

# ---- 3. dkms add / build / install -------------------------------------

DKMS_SRC="/usr/src/mana-compat-$PKGVER"
if [ -d "$DKMS_SRC" ]; then
    log "removing prior source tree at $DKMS_SRC (will re-add)"
    dkms remove -m mana-compat -v "$PKGVER" --all >/dev/null 2>&1 || true
    rm -rf "$DKMS_SRC"
fi

log "staging source under $DKMS_SRC"
cp -a "$SRCDIR" "$DKMS_SRC"

log "dkms add"
dkms add -m mana-compat -v "$PKGVER"

log "dkms build --kernelver $KVER"
dkms build -m mana-compat -v "$PKGVER" -k "$KVER"

log "dkms install --kernelver $KVER"
dkms install -m mana-compat -v "$PKGVER" -k "$KVER" --force

# ---- 4. blacklist in-tree mana + regenerate initramfs -------------------

BLACKLIST=/etc/modprobe.d/50-mana-compat.conf
if [ ! -f "$BLACKLIST" ]; then
    log "installing modprobe blacklist at $BLACKLIST"
    cat > "$BLACKLIST" <<'EOF'
# Installed by mana-compat DKMS installer.
# Keep the in-tree mana driver from claiming the device so the out-of-tree
# mana-compat can bind at boot.
blacklist mana
blacklist mana_ib
EOF
fi

log "regenerating initramfs so the blacklist takes effect at early boot"
if command -v update-initramfs >/dev/null 2>&1; then
    update-initramfs -u -k "$KVER" || warn "update-initramfs -u failed (non-fatal)"
elif command -v dracut >/dev/null 2>&1; then
    dracut --force /boot/initramfs-"$KVER".img "$KVER" \
        || warn "dracut regeneration failed (non-fatal)"
elif command -v mkinitcpio >/dev/null 2>&1; then
    mkinitcpio -p linux || warn "mkinitcpio regeneration failed (non-fatal)"
else
    warn "no initramfs tool detected -- blacklist will only take effect after next module load, not early boot"
fi

# ---- 5. autoload at boot -----------------------------------------------

LOAD_CONF=/etc/modules-load.d/mana-compat.conf
if [ ! -f "$LOAD_CONF" ]; then
    echo "mana_compat" > "$LOAD_CONF"
    log "installed autoload at $LOAD_CONF"
fi

# ---- 6. summary --------------------------------------------------------

echo
echo "  mana-compat $PKGVER installed via DKMS"
echo "  ================================================"
dkms status | grep mana-compat || true
echo
echo "  next steps:"
echo "    1. Reboot so the blacklist takes effect and mana-compat binds"
echo "       cleanly at boot (recommended for any production install):"
echo "         sudo reboot"
echo
echo "    2. Or, if you need it live NOW without reboot (may fail if in-tree"
echo "       mana is currently holding the device):"
echo "         sudo modprobe -r mana_ib mana 2>/dev/null || true"
echo "         sudo modprobe mana-compat"
echo
echo "  verification after boot / modprobe:"
echo "    lsmod | grep mana        # -> mana_compat"
echo "    cat /sys/module/mana_compat/version"
echo
echo "  removal:"
echo "    sudo sh $HERE/uninstall.sh"
