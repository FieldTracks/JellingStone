#!/bin/bash

PWD=$(pwd)

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


if [ ! -d "$DIR" ]; then
  # try to add .. - if the script is run from contrib
  DIR=../build
  if [[ ! -d "$DIR" ]]; then
        echo "build directory not accessible - exiting"
        exit 1
  fi
fi

ARCHIVE="${NAME}-${VERSION}+${BUILD}.zip"
mkdir -p dist/JellingStone
cp $DIR/bootloader/bootloader.bin dist/JellingStone
cp $DIR/partition_table/partition-table.bin dist/JellingStone
cp $DIR/JellingStone.bin dist/JellingStone
cd dist || exit 1
zip "$ARCHIVE" JellingStone/*
sha1sum "$ARCHIVE"
mv "$ARCHIVE" "$PWD"
cd "$PWD" || exit 1