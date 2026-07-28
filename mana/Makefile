# SPDX-License-Identifier: GPL-2.0-or-later
#
# Checked build wrapper used by both humans and DKMS. Every build re-runs the
# compile-test probes before handing off to kbuild, so the graft always reflects
# the target kernel's current headers and configuration.

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

AUTOCONF := compat/autogen/mana_compat_autoconf.h

# --- Universal DKMS source set ------------------------------------------
# The files shipped to /usr/src/<module>-<version>/ on the target so DKMS
# can build the module in place. Consumed by the `tarball` target below.
DKMS_SRC := Kbuild Makefile configure source-version package-revision \
            README.md dkms.conf compat driver scripts

.PHONY: all
all: config
	$(MAKE) -C $(KDIR) M=$(PWD) MANA_COMPAT_PROBED=1 modules

.PHONY: config
config:
	sh $(PWD)/configure --kerneldir $(KDIR)

# Print the auto-derived package version
# (<MANA internal>+linux<source>+compat<rev>).
.PHONY: version
version:
	@sh $(PWD)/scripts/gen-version.sh

# Sync dkms.conf PACKAGE_VERSION to the auto-derived version. Run this
# after a driver re-import bumps mana.h or source-version, or after
# bumping ./package-revision (see bump-revision) for a compat-only or
# packaging-only change.
.PHONY: set-version
set-version:
	@v=$$(sh $(PWD)/scripts/gen-version.sh); \
	sed 's|^PACKAGE_VERSION=.*|PACKAGE_VERSION="'"$$v"'"|' dkms.conf > dkms.conf.tmp \
		&& mv dkms.conf.tmp dkms.conf; \
	echo "dkms.conf: PACKAGE_VERSION=\"$$v\""

# Increment the out-of-tree packaging revision (PACKAGE_REVISION in
# ./package-revision) and sync the version everywhere via set-version. Use this
# for a compat-only or packaging-only change that does NOT move the
# upstream-derived halves of the version (mana.h or source-version) -- the bump
# is what makes DKMS treat the build as a new version instead of reusing the
# previously built, now-stale module.
.PHONY: bump-revision
bump-revision:
	@cur=$$(sed -n 's/^PACKAGE_REVISION=\(.*\)/\1/p' package-revision | head -n1 \
		| sed -e 's/^"//' -e 's/"$$//'); \
	case "$$cur" in \
		''|*[!0-9]*) echo "ERROR: PACKAGE_REVISION in package-revision is not an integer ('$$cur')" >&2; exit 1;; \
	esac; \
	next=$$((cur + 1)); \
	sed 's|^PACKAGE_REVISION=.*|PACKAGE_REVISION='"$$next"'|' package-revision > package-revision.tmp \
		&& mv package-revision.tmp package-revision; \
	echo "package-revision: PACKAGE_REVISION $$cur -> $$next"; \
	$(MAKE) set-version

# --- Universal DKMS tarball installer ------------------------------------
# The single distribution artifact. Works on any Linux distro with (or that
# can install) DKMS -- Debian, Ubuntu, RHEL, Fedora, Azure Linux, SUSE,
# Arch. DKMS handles the per-kernel build on the target, so a single tarball
# built once covers every supported LTS kernel (5.15 through 6.18+) and
# every future kernel that ships on a target distro.
#
# The tarball contains:
#   install.sh                  # distro-aware installer (POSIX sh)
#   uninstall.sh                # symmetric removal path
#   mana-compat-<version>/      # DKMS source tree (deployed to /usr/src/)
TARBALL := $(PWD)/mana-compat-installer-$(shell sed -n 's/^PACKAGE_VERSION="\([^"]*\)".*/\1/p' dkms.conf | head -n1).tar.gz

.PHONY: tarball
tarball:
	@v=$$(sed -n 's/^PACKAGE_VERSION="\([^"]*\)".*/\1/p' dkms.conf | head -n1); \
	[ -n "$$v" ] || { echo "ERROR: cannot read PACKAGE_VERSION from dkms.conf" >&2; exit 1; }; \
	stage=$$(mktemp -d); trap 'rm -rf "$$stage"' EXIT; \
	mkdir -p "$$stage/mana-compat-$$v"; \
	cp -a $(DKMS_SRC) "$$stage/mana-compat-$$v"/; \
	find "$$stage/mana-compat-$$v/compat/autogen" -type f ! -name '.gitkeep' -delete 2>/dev/null || true; \
	find "$$stage/mana-compat-$$v" \( -name '*.o' -o -name '*.ko' -o -name '*.mod' \
		-o -name '*.mod.c' -o -name '*.cmd' -o -name '.*.cmd' -o -name '*.d' \
		-o -name '*.symvers' -o -name 'modules.order' \) -delete 2>/dev/null || true; \
	install -m 0755 scripts/install.sh   "$$stage/install.sh"; \
	install -m 0755 scripts/uninstall.sh "$$stage/uninstall.sh"; \
	tar -C "$$stage" -czf $(TARBALL) install.sh uninstall.sh mana-compat-$$v; \
	echo "  tarball: $(TARBALL)"; \
	echo "  install on any Linux target with:"; \
	echo "    tar xzf $$(basename $(TARBALL))"; \
	echo "    sudo sh install.sh"

.PHONY: clean
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) MANA_COMPAT_PROBED=1 clean
	rm -f $(AUTOCONF)

.PHONY: distclean
distclean: clean
	rm -f $(AUTOCONF)
	rm -f mana-compat-installer-*.tar.gz
