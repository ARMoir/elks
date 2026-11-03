#!/usr/bin/env python3
"""Wrap the populated FAT16 filesystem in an exactly 16 MiB CF disk image."""
from pathlib import Path
import hashlib
import struct
import sys

SECTOR = 512
DISK_SECTORS = 32768
START = 63
LENGTH = 32 * 16 * 63 - START


def chs(lba):
    cylinder, offset = divmod(lba, 16 * 63)
    head, sector = divmod(offset, 63)
    return bytes([head, sector + 1 | (cylinder >> 2 & 0xC0), cylinder & 255])


def verify(data):
    assert len(data) == DISK_SECTORS * SECTOR
    assert data[510:512] == b"\x55\xaa"
    entry = data[446:462]
    assert entry[0] == 0x80 and entry[4] == 0x04
    assert entry[1:4] == chs(START)
    assert entry[5:8] == chs(START + LENGTH - 1)
    assert struct.unpack_from("<II", entry, 8) == (START, LENGTH)
    assert data[462:510] == bytes(48)
    boot = data[START * SECTOR:(START + 1) * SECTOR]
    assert boot[510:512] == b"\x55\xaa"
    assert struct.unpack_from("<H", boot, 11)[0] == SECTOR
    assert boot[13] == 2 and boot[16] == 2
    total = struct.unpack_from("<H", boot, 19)[0] or struct.unpack_from("<I", boot, 32)[0]
    assert total == LENGTH
    assert struct.unpack_from("<HHI", boot, 24) == (63, 16, START)
    reserved = struct.unpack_from("<H", boot, 14)[0]
    fat_sectors = struct.unpack_from("<H", boot, 22)[0]
    root_sectors = (struct.unpack_from("<H", boot, 17)[0] * 32 + 511) // 512
    clusters = (total - reserved - 2 * fat_sectors - root_sectors) // 2
    assert 4085 <= clusters < 65525, clusters
    fat = (START + reserved) * SECTOR
    assert data[fat:fat + fat_sectors * SECTOR] == data[fat + fat_sectors * SECTOR:fat + 2 * fat_sectors * SECTOR]
    assert START + LENGTH <= 32 * 16 * 63 <= DISK_SECTORS
    print(f"Verified: 16 MiB disk, active FAT16 partition LBA {START}+{LENGTH}, {clusters} clusters")


def main():
    source, bootfile, dest = map(Path, sys.argv[1:])
    volume = bytearray(source.read_bytes()[:LENGTH * SECTOR])
    assert len(volume) == LENGTH * SECTOR
    struct.pack_into("<I", volume, 28, START)
    volume[36] = 0x80
    mbr = bytearray(bootfile.read_bytes())
    assert len(mbr) == SECTOR and mbr[510:] == b"\x55\xaa"
    entry = bytes([0x80]) + chs(START) + bytes([0x04]) + chs(START + LENGTH - 1)
    mbr[446:462] = entry + struct.pack("<II", START, LENGTH)
    disk = bytearray(DISK_SECTORS * SECTOR)
    disk[:SECTOR] = mbr
    disk[START * SECTOR:(START + LENGTH) * SECTOR] = volume
    verify(disk)
    dest.write_bytes(disk)
    digest = hashlib.sha256(disk).hexdigest()
    dest.with_suffix(".sha256").write_text(f"{digest}  {dest.name}\n")
    print(digest, dest.name)


if __name__ == "__main__":
    main()
