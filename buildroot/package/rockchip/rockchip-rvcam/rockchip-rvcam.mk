################################################################################
#
# rockchip-rvcam project
#
################################################################################

ROCKCHIP_RVCAM_VERSION = main
ROCKCHIP_RVCAM_SITE = $(TOPDIR)/../external/rvcam
ROCKCHIP_RVCAM_SITE_METHOD = local
ROCKCHIP_RVCAM_LICENSE = proprietary, GPL-2.0, Apache-2.0
ROCKCHIP_RVCAM_LICENSE_FILES = licenses/LICENSE licenses/GPL-2.0 licenses/Apache-2.0
ROCKCHIP_RVCAM_INSTALL_STAGING = YES

ROCKCHIP_RVCAM_DEPENDENCIES += tinyxml2
ROCKCHIP_RVCAM_DEPENDENCIES += rockchip-erpc

# if rvcam/prebuilts/Linux does not exist, build RVCAM source version
ifeq ($(wildcard $(ROCKCHIP_RVCAM_SITE)/prebuilts/Linux),)

$(eval $(cmake-package))

# otherwise, build RVCAM release version
else

define ROCKCHIP_RVCAM_INSTALL_TARGET_CMDS
	cp -r $(@D)/prebuilts/Linux/usr/bin/* $(TARGET_DIR)/usr/bin; \
	cp -r $(@D)/prebuilts/Linux/usr/lib/*.so* $(TARGET_DIR)/usr/lib
endef

define ROCKCHIP_RVCAM_INSTALL_INIT_SYSV
	$(INSTALL) -D -m 0644 $(@D)/components/config/xml/rvcam_config.xml \
		$(TARGET_DIR)/etc/
	$(INSTALL) -D -m 0755 $(@D)/rvcam_remote/S60rvcam \
		$(TARGET_DIR)/etc/init.d/
endef

define ROCKCHIP_RVCAM_INSTALL_STAGING_CMDS
	cp -r $(@D)/prebuilts/Linux/usr/bin/* $(STAGING_DIR)/usr/bin; \
	cp -r $(@D)/prebuilts/Linux/usr/lib/* $(STAGING_DIR)/usr/lib
endef

$(eval $(generic-package))

endif
