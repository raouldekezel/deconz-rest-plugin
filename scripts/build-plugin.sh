#!/usr/bin/env bash
# Build libde_rest_plugin.so from this checkout in a disposable container.
#
#   scripts/build-plugin.sh [output-dir]      (default: a new temporary directory)
#
# deconz-lib is pinned to the commit matching the deCONZ release `deploy`
# tracks, instead of upstream's floating `GIT_TAG main`. Nothing is
# installed on the host; the source tree is copied, never modified.
set -euo pipefail

DECONZ_LIB_COMMIT="${DECONZ_LIB_COMMIT:-6c69a3c1612f8d18a97f4c4ee239072e84338cde}"   # last deconz-lib commit before v2.33.2
IMAGE="deconz-rest-plugin-build:bookworm"
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT="${1:-$(mktemp -d)}"
mkdir -p "$OUT" && OUT="$(cd "$OUT" && pwd)"

docker build -q -t "$IMAGE" -f "$ROOT/scripts/Dockerfile.build" "$ROOT/scripts" >/dev/null

docker run --rm -v "$ROOT":/src:ro -v "$OUT":/out -e DECONZ_LIB_COMMIT="$DECONZ_LIB_COMMIT" \
    -e HOST_UID="$(id -u)" -e HOST_GID="$(id -g)" "$IMAGE" bash -euo pipefail -c '
    cp -a /src /work/src
    git config --global --add safe.directory "*"   # the copy belongs to the host user
    # pin deconz-lib in the copy only (upstream fetches the floating "main")
    sed -i "s/GIT_TAG        main/GIT_TAG        $DECONZ_LIB_COMMIT/" /work/src/CMakeLists.txt
    grep -q "GIT_TAG        $DECONZ_LIB_COMMIT" /work/src/CMakeLists.txt
    # link against the SQLite library shipped by deCONZ (libdeconzsqlite3.so), like the packaged plugin
    # with symbols exported: the root CMakeLists hides C symbols, and Debian links --as-needed
    echo "set_target_properties(sqlite3 PROPERTIES OUTPUT_NAME deconzsqlite3 C_VISIBILITY_PRESET default)" >> /work/src/sqlite3/CMakeLists.txt
    cmake -S /work/src -B /work/build -DCMAKE_BUILD_TYPE=Release -DQT_VERSION_MAJOR=5 \
          -DCMAKE_BUILD_WITH_INSTALL_RPATH=ON -DCMAKE_INSTALL_RPATH=/usr/local/lib >/work/cmake.log 2>&1 || { tail -30 /work/cmake.log; exit 1; }
    cmake --build /work/build -j"$(nproc)" >/work/build.log 2>&1 || { tail -40 /work/build.log; exit 1; }
    so=$(find /work/build -name "libde_rest_plugin.so" -type f | head -1)
    cp "$so" /out/libde_rest_plugin.so
    chown "$HOST_UID:$HOST_GID" /out/libde_rest_plugin.so
    echo "deconz-lib $(git -C /work/build/deconzlib-src rev-parse HEAD)"
'
sha256sum "$OUT/libde_rest_plugin.so"
