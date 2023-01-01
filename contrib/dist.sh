#!/bin/bash

DIR=./build

if [ -z "$NAME" ]; then
  NAME="JellingStone"
fi
if [ -z "$VERSION" ]; then
  VERSION="$(git rev-parse HEAD)"
fi
if [ -z "$BUILD" ]; then
  BUILD="$(date +%s)"
fi


if [[ ! -d "$DIR" ]]; then
  # try to add .. - if the script is run from contrib
  DIR=../build
  if [[ ! -d "$DIR" ]]; then
        echo "build directory not accessible - exiting"
        exit 1
  fi
fi

ARCHIVE="${NAME}_${VERSION}_${BUILD}.zip"
zip "$ARCHIVE" $DIR/bootloader/bootloader.bin $DIR/partition_table/partition-table.bin $DIR/JellingStone.bin
sha1sum "$ARCHIVE"