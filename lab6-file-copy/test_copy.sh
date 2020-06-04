#!/bin/sh
# Shell script for simple testing of copy program
# Made by Jan Radziminski

name="Copy"
command="./copy"

echo "Running $name testing program...\n ";

echo "Displaying help:\n./copy -h"
./copy -h

echo "\nCopying small file with read/write method:\n./copy input/file_small output/file_small_copy_rw"
./copy input/file_small output/file_small_copy_rw
echo "File copied\n"

echo "Copying small file with memorymap method:\n./copy -m input/file_small output/file_small_copy_mmap"
./copy -m input/file_small output/file_small_copy_mmap
echo "File copied\n"

echo "Copying medium file with read/write method:\n./copy input/file_mid output/file_mid_copy_rw"
./copy input/file_mid output/file_mid_copy_rw
echo "File copied\n"

echo "Copying medium file with memorymap method:\n./copy -m input/file_mid output/file_mid_copy_mmap"
./copy -m input/file_mid output/file_mid_copy_mmap
echo "File copied\n"

echo "Copying file that doesnt exist:\n./copy input/abc output/abc_copy"
./copy -m input/abc output/abc_copy

echo "\nCopying file to directory that doesnt exist:\n./copy input/file_mid test/abc_copy"
./copy -m input/file_mid test/abc_copy