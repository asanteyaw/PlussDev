#!/usr/bin/env python3
"""Pack the public documentation into one file for the download.

    python3 bundle_docs.py <source root> <output docs.pack>

pluss::tils::doc reads pages from this file when there is no network. Layout, little-endian:

    8 bytes   "PLDOCPK1"
    u32       number of pages
    per page  u16 path length, the path (as in the source tree), u64 offset, u32 packed size, u32 size
    then      each page as a raw DEFLATE stream, at its offset from the start of the file
"""
import struct
import sys
import zlib
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import public_docs as pd  # noqa: E402


def main(source, output):
    source = Path(source).resolve()
    entries = []
    for space, folder, path in pd.public_pages(source):
        raw = pd.public_text(path, source).encode("utf-8")
        packer = zlib.compressobj(9, zlib.DEFLATED, -15)
        entries.append((path.relative_to(source).as_posix().encode(), packer.compress(raw) + packer.flush(), len(raw)))
    head = 8 + 4 + sum(2 + len(name) + 8 + 4 + 4 for name, _, _ in entries)
    index, blobs, offset = bytearray(), bytearray(), head
    for name, packed, size in entries:
        index += struct.pack("<H", len(name)) + name + struct.pack("<QII", offset, len(packed), size)
        blobs += packed
        offset += len(packed)
    out = Path(output)
    out.parent.mkdir(parents=True, exist_ok=True)
    data = b"PLDOCPK1" + struct.pack("<I", len(entries)) + bytes(index) + bytes(blobs)
    if not out.exists() or out.read_bytes() != data:
        out.write_bytes(data)
    print(f"docs pack: {len(entries)} pages, {len(data) // 1024} KB")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    main(sys.argv[1], sys.argv[2])
