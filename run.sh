#!/usr/bin/env bash
set -e
cd "$(dirname "$0")"

# Limit parallel build jobs safely on low-RAM machines to avoid OOM (exit code 137)
if [ -z "${JOBS:-}" ]; then
    AVAIL_KB=$(awk '/MemAvailable/ {print $2}' /proc/meminfo 2>/dev/null || echo 2000000)
    if [ "$AVAIL_KB" -lt 4500000 ]; then
        JOBS=1
    elif [ "$AVAIL_KB" -lt 9000000 ]; then
        JOBS=2
    else
        JOBS=3
    fi
fi

flatpak-builder --force-clean --jobs="$JOBS" --repo=repo build-dir org.deepcomplex.App.json
flatpak kill org.deepcomplex.App 2>/dev/null || true
flatpak --user uninstall -y org.deepcomplex.App 2>/dev/null || true
flatpak --user remote-delete --force deepcomplex-local 2>/dev/null || true
flatpak --user remote-add --no-gpg-verify deepcomplex-local "$PWD/repo"
flatpak --user install -y --no-deps deepcomplex-local org.deepcomplex.App
update-desktop-database ~/.local/share/flatpak/exports/share/applications 2>/dev/null || true
gtk-update-icon-cache -f ~/.local/share/flatpak/exports/share/icons/hicolor 2>/dev/null || true
rm -f ~/.cache/icon-cache.kcache
kbuildsycoca6 --noincremental 2>/dev/null || true
flatpak run org.deepcomplex.App
