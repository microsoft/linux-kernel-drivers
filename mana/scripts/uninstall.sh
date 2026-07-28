#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Universal uninstaller for the mana-compat DKMS module. Reverses
# scripts/install.sh: removes the DKMS module for every kernel it was built
# for, deletes the source tree from /usr/src, removes the blacklist and the
# autoload entry, regenerates the initramfs so the in-tree mana can bind
# again at next boot.

set -eu

log()  { printf '%s\n' "  $*"; }
warn() { printf '\033[33mWARN\033[0m: %s\n' "$*" >&2; }
die()  { printf '\033[31mERROR\033[0m: %s\n' "$*" >&2; exit 1; }

[ "$(id -u)" -eq 0 ] || die "must run as root (try: sudo sh uninstall.sh)"

if ! command -v dkms >/dev/null 2>&1; then
    warn "dkms not installed; skipping dkms removal step"
else
    # Find every version of mana-compat DKMS knows about and remove all
    dkms status mana-compat 2>/dev/null | while IFS= read -r line; do
        # Parse "mana-compat/<ver>, <kernel>, ..." or "mana-compat, <ver>, ..."
        VER=$(printf '%s' "$line" | sed -n 's|^mana-compat[/,][[:space:]]*\([^,]*\).*|\1|p')
        [ -n "$VER" ] || continue
        log "dkms remove mana-compat/$VER --all"
        dkms remove -m mana-compat -v "$VER" --all >/dev/null 2>&1 || true
    done

    # If dkms remove couldn't complete (e.g. /usr/src tree already gone from a
    # botched prior install), forcibly clean the state tree so future installs
    # don't hit "Missing the module source directory". Safe -- we only remove
    # our own module's DKMS state.
    if [ -d /var/lib/dkms/mana-compat ]; then
        log "force-removing leftover /var/lib/dkms/mana-compat"
        rm -rf /var/lib/dkms/mana-compat
    fi

    # Delete leftover /usr/src trees (dkms remove --all doesn't remove them)
    for d in /usr/src/mana-compat-*; do
        [ -d "$d" ] || continue
        log "removing source tree $d"
        rm -rf "$d"
    done
fi

# Currently-loaded module goes away too, so the running kernel stops using it.
if lsmod | grep -q "^mana_compat "; then
    log "unloading currently-running mana_compat module"
    rmmod mana_compat 2>&1 || warn "rmmod failed (module in use?)"
fi

# Blacklist + autoload
for f in /etc/modprobe.d/50-mana-compat.conf /etc/modprobe.d/mana-oot.conf \
         /etc/modules-load.d/mana-compat.conf; do
    if [ -f "$f" ]; then
        log "removing $f"
        rm -f "$f"
    fi
done

# Regenerate initramfs so the in-tree mana can auto-load next boot
KVER=$(uname -r)
log "regenerating initramfs for $KVER"
if command -v update-initramfs >/dev/null 2>&1; then
    update-initramfs -u -k "$KVER" || warn "update-initramfs failed (non-fatal)"
elif command -v dracut >/dev/null 2>&1; then
    dracut --force /boot/initramfs-"$KVER".img "$KVER" \
        || warn "dracut failed (non-fatal)"
elif command -v mkinitcpio >/dev/null 2>&1; then
    mkinitcpio -p linux || warn "mkinitcpio failed (non-fatal)"
fi

echo
echo "  mana-compat removed."
echo "  Reboot to let the in-tree mana driver bind the device again:"
echo "    sudo reboot"
