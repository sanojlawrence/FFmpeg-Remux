#!/bin/bash

set -e

NDK=/c/Users/Home/AppData/Local/Android/Sdk/ndk/25.2.9519653
SYSROOT=$NDK/toolchains/llvm/prebuilt/windows-x86_64/sysroot
STRIP=$NDK/toolchains/llvm/prebuilt/windows-x86_64/bin/llvm-strip

build_ffmpeg() {
  ABI=$1
  ARCH=$2
  CPU=$3
  CC=$4
  PREFIX=../generated/$ABI

  echo "===================="
  echo "Building FFmpeg for $ABI"
  echo "===================="

  CONFIGURE_OPTS="\
      --prefix=$PREFIX \
      --target-os=android \
      --arch=$ARCH \
      --cpu=$CPU \
      --cc=$CC \
      --enable-cross-compile \
      --sysroot=$SYSROOT \
      --strip=$STRIP \
      --disable-static \
      --enable-shared \
      --disable-programs \
      --disable-doc \
      --disable-avdevice \
      --disable-avfilter \
      --disable-swscale \
      --disable-network \
      --enable-avcodec \
      --enable-avformat \
      --enable-avutil \
      --enable-swresample \
      --enable-muxer=mp4,matroska \
      --enable-demuxer=mov,matroska \
      --enable-parser=aac,h264 \
      --enable-protocol=file \
      --enable-zlib \
      --disable-bzlib \
      --disable-encoders \
      --disable-decoders \
      --disable-gpl \
      --disable-version3 \
      --enable-pic \
      --enable-small"

  # Disable ASM for x86/x86_64
  if [ "$ABI" == "x86" ] || [ "$ABI" == "x86_64" ]; then
    CONFIGURE_OPTS+=" --disable-x86asm --disable-inline-asm"
  fi

  ./configure $CONFIGURE_OPTS

  make clean
  # shellcheck disable=SC2046
  make -j$(nproc)
  make install

  echo "Stripping shared libraries for $ABI using llvm-strip..."
  $STRIP --strip-unneeded $PREFIX/lib/*.so
}

# Build for armeabi-v7a
build_ffmpeg "armeabi-v7a" "arm" "armv7-a" \
  "$NDK/toolchains/llvm/prebuilt/windows-x86_64/bin/armv7a-linux-androideabi21-clang"

# Build for arm64-v8a
build_ffmpeg "arm64-v8a" "aarch64" "armv8-a" \
  "$NDK/toolchains/llvm/prebuilt/windows-x86_64/bin/aarch64-linux-android21-clang"

# Build for x86
build_ffmpeg "x86" "x86" "i686" \
  "$NDK/toolchains/llvm/prebuilt/windows-x86_64/bin/i686-linux-android21-clang"

# Build for x86_64
build_ffmpeg "x86_64" "x86_64" "x86-64" \
  "$NDK/toolchains/llvm/prebuilt/windows-x86_64/bin/x86_64-linux-android21-clang"
