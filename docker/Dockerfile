# Pokered Save Editor 2 — Linux build/test container
# -------------------------------------------------------------------------
# Bakes the whole toolchain ONCE (clang/llvm + CMake/Ninja + Qt 6.11 + the
# headless-GUI libs + Xvfb + ccache) so every test run is just compile + ctest.
#
# The repo is NOT copied into the image — it is bind-mounted read-only at
# runtime (/host) and rsynced into a persistent ext4 volume (/build) by
# run-tests.sh, so builds happen on the container's own fast filesystem rather
# than over the slow Windows/WSL bind mount. See docker/README.md.
#
# Mirrors Twilight's local kit: Qt 6.11 + clang. (CI uses gcc/6.8.3; this is
# deliberately the kit toolchain so it reproduces the runtime the app ships on,
# AND it is where ASan/UBSan actually work — they are broken on the Windows
# llvm-mingw kit.)
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

# --- Toolchain + headless-GUI runtime deps --------------------------------
# clang/lld/llvm: compiler + linker + llvm-cov/llvm-profdata for coverage.
# xvfb: virtual X server for the real-X GUI test path (complements offscreen).
# The libxcb-*/libgl/libxkbcommon/fontconfig set is what Qt's xcb + offscreen
# platform plugins need to load headless.
RUN apt-get update && apt-get install -y --no-install-recommends \
        clang lld llvm \
        cmake ninja-build git ccache rsync \
        python3 python3-pip \
        xvfb \
        ca-certificates \
        libgl1 libegl1 libglx-mesa0 libopengl0 \
        mesa-common-dev libgl-dev libegl-dev libgles-dev libglx-dev libopengl-dev \
        libxkbcommon0 libxkbcommon-x11-0 libxkbcommon-dev \
        libfontconfig1 libfreetype6 libdbus-1-3 libglib2.0-0 \
        libx11-6 libx11-xcb1 libxcb1 libxcb-cursor0 libxcb-glx0 \
        libxcb-icccm4 libxcb-image0 libxcb-keysyms1 libxcb-randr0 \
        libxcb-render0 libxcb-render-util0 libxcb-shape0 libxcb-shm0 \
        libxcb-sync1 libxcb-util1 libxcb-xfixes0 libxcb-xinerama0 libxcb-xkb1 \
        libxrender1 libxi6 libsm6 libice6 \
    && rm -rf /var/lib/apt/lists/*

# --- Qt via aqtinstall -----------------------------------------------------
# linux_gcc_64 is the only desktop Qt arch shipped for Linux; its prebuilt libs
# are gcc-built but link cleanly against clang-compiled app code (both use
# libstdc++, matching ABI). qtdeclarative (Qt Quick) + qtsvg are base components;
# the genuine add-ons go in -m.
#
# KEEP THIS LIST == the `modules:` line in .github/workflows/tests.yml. It was
# `-m qtcharts` alone, which was right until pse-audio arrived and pulled in Qt
# Multimedia: every build in this image then died at configure with "Could NOT
# find Qt6Multimedia", so `dtest.ps1 asan` -- the ONLY place AddressSanitizer
# runs locally, since it does not work on the llvm-mingw kit -- silently stopped
# working. That is how a heap-use-after-free in the music parser survived to CI
# (2026-07-16). If you add a Qt module to the build, add it here too.
ARG QT_VERSION=6.11.0
RUN python3 -m pip install --no-cache-dir --break-system-packages aqtinstall \
    && aqt install-qt linux desktop ${QT_VERSION} linux_gcc_64 \
       -m qtcharts qtmultimedia qtshadertools -O /opt/Qt

ENV QT_ROOT=/opt/Qt/${QT_VERSION}/gcc_64
ENV CMAKE_PREFIX_PATH=${QT_ROOT}
ENV PATH=${QT_ROOT}/bin:${PATH}
ENV LD_LIBRARY_PATH=${QT_ROOT}/lib
ENV CC=clang
ENV CXX=clang++
# Qt wants a UTF-8 locale; without this it prints a benign switch-to-C.UTF-8
# warning on every Qt process (noisy in test logs).
ENV LANG=C.UTF-8
ENV LC_ALL=C.UTF-8

# --- Sanitizer runtime (for the `asan` variant) ----------------------------
# clang's compiler-rt sanitizer archives (libclang_rt.asan*) are NOT in the
# base `clang` package. Separate layer (after Qt) so it doesn't invalidate the
# cached Qt download. Version matches the clang the `clang` metapackage pulls
# on Ubuntu 24.04 (clang 18).
RUN apt-get update && apt-get install -y --no-install-recommends \
        libclang-rt-18-dev \
    && rm -rf /var/lib/apt/lists/*

# --- Entrypoint ------------------------------------------------------------
COPY run-tests.sh /usr/local/bin/run-tests.sh
# Strip any CRLF (file may be authored on Windows) and make executable.
RUN sed -i 's/\r$//' /usr/local/bin/run-tests.sh && chmod +x /usr/local/bin/run-tests.sh

ENTRYPOINT ["/usr/local/bin/run-tests.sh"]
CMD ["standard"]
