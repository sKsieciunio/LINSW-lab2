SEQUENCER_VERSION = 1.0
SEQUENCER_SITE    = ~/Desktop/LINSW-lab2-git
SEQUENCER_SITE_METHOD = local

SEQUENCER_LICENSE = MIT          # adjust to your license
SEQUENCER_LICENSE_FILES = LICENSE

SEQUENCER_DEPENDENCIES = c-periphery

# Pass cross-compiler and flags from Buildroot to your Makefile
define SEQUENCER_BUILD_CMDS
    $(MAKE) CC="$(TARGET_CC)" \
            CFLAGS="$(TARGET_CFLAGS)" \
            LDFLAGS="$(TARGET_LDFLAGS)" \
            -C $(@D)
endef

# Install the binary to /usr/bin on the target
define SEQUENCER_INSTALL_TARGET_CMDS
    $(INSTALL) -D -m 0755 $(@D)/sequencer \
        $(TARGET_DIR)/usr/bin/sequencer
endef

$(eval $(generic-package))