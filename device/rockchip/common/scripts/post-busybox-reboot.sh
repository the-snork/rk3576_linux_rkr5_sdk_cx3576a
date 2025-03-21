#!/bin/bash -e

source "${RK_POST_HELPER:-$(dirname "$(realpath "$0")")/post-helper}"

REBOOT_WRAPPER=busybox-reboot

[ "$(readlink sbin/reboot)" = busybox ] || exit 0

message "Fixing up busybox reboot commands..."

install -D -m 0755 "$RK_DATA_DIR/$REBOOT_WRAPPER" sbin/$REBOOT_WRAPPER

for cmd in halt reboot poweroff shutdown; do
	ln -sf $REBOOT_WRAPPER sbin/$cmd
done
