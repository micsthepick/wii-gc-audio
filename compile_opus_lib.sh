cd opus
./autogen.sh
CC="$DEVKITPPC/bin/powerpc-eabi-gcc" \
AR="$DEVKITPPC/bin/powerpc-eabi-ar" \
RANLIB="$DEVKITPPC/bin/powerpc-eabi-ranlib" \
./configure \
    --host=powerpc-eabi \
    --disable-shared \
    --enable-static \
    --enable-fixed-point \
    --disable-extra-programs
make -j$(nproc)