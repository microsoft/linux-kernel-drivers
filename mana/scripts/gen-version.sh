#!/bin/sh
# gen-version.sh - derive the package version for the MANA OOT/DKMS module.
#
# The version is built from three sources so it always tracks reality and never
# drifts:
#
#   <MANA internal>+linux<source>+compat<REV>
#
#   MANA internal : MANA_{MAJOR,MINOR,MICRO}_VERSION from the captive driver
#                   header driver/include/net/mana/mana.h. This is the version
#                   the driver itself reports to the device, i.e. the upstream
#                   driver's own notion of its version. Verbatim upstream.
#   source        : LINUX_SOURCE_VERSION from ./source-version, recording the
#                   kernel tree the driver was vendored out of
#                   (VERSION.PATCHLEVEL.SUBLEVEL). Verbatim.
#   REV           : PACKAGE_REVISION from ./package-revision -- the one component
#                   the out-of-tree packaging owns. The two upstream-derived
#                   halves above do not move for a compat-only or packaging-only
#                   change, so this trailing revision is what makes such a build
#                   a distinct, newer version. DKMS keys its build tree and its
#                   upgrade decisions on the version, so without it a changed
#                   build would reuse the previously cached .ko (stale driver).
#
# It is appended LAST so the upstream identifiers dominate ordering and the
# revision is only a tiebreaker between builds off the same upstream base: a
# re-vendor to a newer kernel sorts above any revision of an older base.
#
# Example: MANA 0.1.1 vendored from Linux 7.0.0, packaging rev 1 -> 0.1.1+linux7.0.0+compat1
#
# POSIX sh. No GNU-isms. Parses with sed (never sources files) so a source
# name containing spaces ("Baby Opossum Posse") can never be executed.
set -eu

self=$(basename "$0")
root=$(CDPATH= cd "$(dirname "$0")/.." && pwd)

mana_h="$root/driver/include/net/mana/mana.h"
srcver="$root/source-version"
pkgrev="$root/package-revision"

die() { echo "$self: error: $*" >&2; exit 1; }

# Extract a #define'd integer from mana.h, e.g. MANA_MAJOR_VERSION -> 0
mana_def() {
	_v=$(sed -n "s/^#define[[:space:]]\{1,\}$1[[:space:]]\{1,\}\([0-9]\{1,\}\).*/\1/p" \
	     "$mana_h" | head -n1)
	[ -n "$_v" ] || die "could not read $1 from $mana_h"
	printf '%s' "$_v"
}

# Extract KEY=VALUE from source-version, stripping optional surrounding quotes.
src_val() {
	sed -n "s/^$1=\(.*\)/\1/p" "$srcver" | head -n1 | sed -e 's/^"//' -e 's/"$//'
}

[ -f "$mana_h" ] || die "missing $mana_h"
[ -f "$srcver" ] || die "missing $srcver (driver provenance for this package)"
[ -f "$pkgrev" ] || die "missing $pkgrev (the out-of-tree packaging revision)"

maj=$(mana_def MANA_MAJOR_VERSION)
min=$(mana_def MANA_MINOR_VERSION)
mic=$(mana_def MANA_MICRO_VERSION)

src=$(src_val LINUX_SOURCE_VERSION)
[ -n "$src" ] || die "LINUX_SOURCE_VERSION empty in $srcver"

# Debian-correct pre-release ordering: the kernel encodes pre-releases in
# EXTRAVERSION as "-rcN" (e.g. 7.1.0-rc7). A literal '-' is the Debian
# upstream/revision separator and, worse, sorts a pre-release ABOVE the final
# release. Map '-' to '~' so 7.1.0~rc7 sorts BEFORE 7.1.0, which is what a
# release candidate should do. A released source (empty EXTRAVERSION, e.g.
# 7.0.0) has no '-', so this is a no-op there.
src=$(printf '%s' "$src" | tr '-' '~')

# Out-of-tree packaging revision (PACKAGE_REVISION from ./package-revision).
# Required and validated as a bare integer -- the same rigor as source-version,
# so a missing or malformed file fails loudly instead of silently producing a
# version that sorts wrong and reintroduces the stale-driver trap.
rev=$(sed -n 's/^PACKAGE_REVISION=\(.*\)/\1/p' "$pkgrev" | head -n1 | sed -e 's/^"//' -e 's/"$//')
[ -n "$rev" ] || die "PACKAGE_REVISION empty in $pkgrev"
case "$rev" in
	*[!0-9]*) die "PACKAGE_REVISION must be a non-negative integer (got '$rev')" ;;
esac

printf '%s.%s.%s+linux%s+compat%s\n' "$maj" "$min" "$mic" "$src" "$rev"
