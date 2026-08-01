#!/usr/bin/env python3
"""Pure-Python UDF 2.50 image writer (metadata partition layout).

Reproduces the layout of the reference "Casino Royale" image byte-for-byte
when given the same source tree, file data, and timestamps. Generalizes to
arbitrary source trees for BD authoring (replaces the ImgBurn path).

Geometry (partition 0, relative sectors):
  sector 0            = metadata file entry (ftype 0xFA)
  sectors 1..31       = zero
  sectors 32..32+N-1  = metadata content (FSD, TD, EFEs, FID blocks); N = meta blocks
  next                = file data, packed in DFS order of file EFEs
  files_end           = 32 + N + total file blocks
  mirror_efe          = roundup(files_end, 32)   (ftype 0xFB)
  mirror content      = mirror_efe + 32          (byte copy of metadata content)
  PART_LEN            = mirror_efe + 32 + N
  total sectors       = 288 + PART_LEN + 288
"""
import os
import struct
import binascii
from collections import OrderedDict

SECTOR = 2048
PART_START = 288
VDS_MAIN = 32
LVID_LOC = 64
AVDP_MAIN = 256
META_ANCHOR = 32            # metadata content starts at partition sector 32
ALLOC_UNIT = 32             # part_map_meta alloc_unit_size (blocks)
CHUNK = 0x3FFFF800          # max 32-bit extent length (top 2 bits reserved)

TID_PVD, TID_AVDP, TID_IUVD, TID_PD, TID_LVD, TID_USD, TID_TD, TID_LVID = 1, 2, 4, 5, 6, 7, 8, 9
TID_FSD, TID_FID, TID_EFE = 256, 257, 266

CHARSET = b"\x00OSTA Compressed Unicode"


# ---------------------------------------------------------------- primitives
def ts(sec=51, minute=46, hour=21, day=4, month=2, year=2026, tz=0x1168):
    return struct.pack("<HHBBBBBBBB", tz, year, month, day, hour, minute, sec, 0, 0, 0)


def ts_now(tz=0x1168):
    import datetime
    dt = datetime.datetime.now()
    return ts(sec=dt.second, minute=dt.minute, hour=dt.hour, day=dt.day,
              month=dt.month, year=dt.year, tz=tz)


def dstring(s, size):
    b = s.encode("utf-8")
    out = bytearray(b"\x08") + b
    assert len(out) < size, f"dstring too long: {s}"
    out = out.ljust(size - 1, b"\x00")
    out.append(1 + len(b))
    return bytes(out)


def charspec():
    return CHARSET.ljust(64, b"\x00")


def regid(ident, suffix=b"\x00" * 8, flags=0):
    b = ident if isinstance(ident, bytes) else ident.encode("utf-8")
    assert len(b) <= 23
    return bytes([flags]) + b.ljust(23, b"\x00") + suffix


def extent_ad(length, loc):
    return struct.pack("<II", length, loc)


def long_ad(length, loc, part=0):
    return struct.pack("<IIH", length, loc, part) + b"\x00" * 6


def tag(tid, body, tagloc, serial=1):
    crclen = len(body)
    crc = binascii.crc_hqx(body, 0)
    raw = bytearray(struct.pack("<HHBBHHHI", tid, 3, 0, 0, serial, crc, crclen, tagloc))
    raw[4] = (sum(raw[:4]) + sum(raw[5:])) & 0xFF
    return bytes(raw) + body


def retag(body, loc):
    return tag(struct.unpack_from("<H", body, 0)[0], body[16:], loc)


# ------------------------------------------------------------ tree model
class Node:
    __slots__ = ("path", "name", "is_dir", "size", "blocks", "chunks",
                 "atime", "mtime", "ctime", "attrtime",
                 "efe_rel", "fid_rel", "link")

    def __init__(self, path, is_dir, size):
        self.path = path
        self.name = path.rstrip("/").split("/")[-1]
        self.is_dir = is_dir
        self.size = size
        self.blocks = 0 if is_dir else (size + SECTOR - 1) // SECTOR
        self.chunks = []
        self.atime = self.mtime = self.ctime = self.attrtime = None
        self.efe_rel = None
        self.fid_rel = None
        self.link = 0


def _parent(path):
    return os.path.dirname(path) or "/"


def build_nodes(src, exclude=()):
    """Walk a source directory, emitting dirs/files in reference order
    (files first, then dirs, byte-order alphabetical). Top-level entries
    whose name is in `exclude` are omitted entirely."""
    nodes = []

    def walk(rel):
        full = os.path.join(src, rel.lstrip("/"))
        p = "/" + rel.rstrip("/") if rel else "/"
        node = Node(p, True, 0)
        nodes.append(node)
        entries = sorted(os.listdir(full))
        for n in entries:
            if n in exclude:
                continue
            if os.path.isdir(os.path.join(full, n)):
                continue
            fp = ("/" + os.path.join(rel, n).rstrip("/")) if rel else "/" + n
            nodes.append(Node(fp, False, os.path.getsize(os.path.join(full, n))))
        for n in entries:
            if n in exclude:
                continue
            if os.path.isfile(os.path.join(full, n)):
                continue
            walk(os.path.join(rel, n) if rel else n)

    walk("")
    return nodes


# ------------------------------------------------------------ descriptors
def build_pvd(label, volset_id, app_id, root_time):
    body = bytearray()
    body += struct.pack("<II", 0, 0)
    body += dstring(label, 32)
    body += struct.pack("<HHHHII", 1, 1, 2, 2, 1, 1)
    body += dstring(volset_id, 128)
    body += charspec()
    body += charspec()
    body += extent_ad(0, 0)
    body += extent_ad(0, 0)
    body += regid(app_id, flags=0)
    body += root_time
    body += regid("*VsoSoftware")
    body += b"\x00" * 64
    body += struct.pack("<IH", 0, 0)
    body += b"\x00" * 22
    assert len(body) == 496
    return tag(TID_PVD, bytes(body), 0)


def build_iuvd(label):
    body = bytearray()
    body += struct.pack("<I", 1)
    body += regid("*UDF LV Info", suffix=struct.pack("<H", 0x0250) + b"\x00" * 6)
    body += charspec()
    body += dstring(label, 128)
    body += b"\x00" * (36 * 3)
    body += regid("*VsoSoftware")
    body += b"\x00" * 128
    assert len(body) == 496
    return tag(TID_IUVD, bytes(body), 0)


def build_pd(part_len):
    body = bytearray()
    body += struct.pack("<IHH", 2, 1, 0)
    body += regid("+NSR03")
    body += b"\x00" * 128
    body += struct.pack("<III", 1, PART_START, part_len)
    body += regid("*VsoSoftware")
    body += b"\x00" * 128
    body += b"\x00" * 156
    assert len(body) == 496
    return tag(TID_PD, bytes(body), 0)


def build_lvd(label, fsd_loc, part0_len, meta_blocks, mirror_efe, alloc=ALLOC_UNIT):
    body = bytearray()
    body += struct.pack("<I", 3)
    body += charspec()
    body += dstring(label, 128)
    body += struct.pack("<I", SECTOR)
    body += regid("*OSTA UDF Compliant", suffix=b"\x50\x02\x03\x00\x00\x00\x00\x00")
    body += long_ad(fsd_loc[0], fsd_loc[1], part=1)
    body += struct.pack("<II", 70, 2)
    body += regid("*VsoSoftware")
    body += b"\x00" * 128
    body += extent_ad(4096, 64)
    body += bytes([1, 6]) + struct.pack("<HH", 1, 0)
    pm2 = bytearray()
    pm2 += bytes([2, 64, 0, 0])
    pm2 += regid("*UDF Metadata Partition", suffix=b"\x50\x02\x00\x00\x00\x00\x00\x00")
    pm2 += struct.pack("<HH", 1, 0)
    pm2 += struct.pack("<I", 0)
    pm2 += struct.pack("<I", mirror_efe)
    pm2 += struct.pack("<I", 0xFFFFFFFF)
    pm2 += struct.pack("<I", alloc)
    pm2 += struct.pack("<I", 0x00010020)
    pm2 += struct.pack("<I", 0)
    assert len(pm2) == 64
    body += pm2
    assert len(body) == 494
    return tag(TID_LVD, bytes(body), 0)


def build_usd():
    return tag(TID_USD, struct.pack("<II", 4, 0), 0)


def build_td():
    return tag(TID_TD, b"\x00" * 496, 0)


def build_lvid(part0_len, meta_blocks, next_uid, num_files, num_dirs, root_time):
    body = bytearray()
    body += root_time
    body += struct.pack("<I", 1)
    body += struct.pack("<II", 0, 0)
    body += struct.pack("<Q", next_uid)
    body += b"\x00" * 24
    body += struct.pack("<II", 2, 46)
    body += struct.pack("<II", 0, 0)
    body += struct.pack("<II", part0_len, meta_blocks)
    body += regid("*VsoSoftware")
    body += struct.pack("<IIHHH", num_files, num_dirs, 0x0250, 0x0250, 0x0250)
    assert len(body) == 126
    return tag(TID_LVID, bytes(body), 0)


def build_avdp_body(reserve_vds):
    return extent_ad(32 * 1024, VDS_MAIN) + extent_ad(32 * 1024, reserve_vds) + b"\x00" * 480


def build_fsd(label, root_efe_loc, root_time):
    body = bytearray()
    body += root_time
    body += struct.pack("<HHII", 3, 3, 1, 1)
    body += struct.pack("<II", 0, 0)
    body += charspec()
    body += dstring(label, 128)
    body += charspec()
    body += dstring(label, 32)
    body += b"\x00" * 64
    body += long_ad(SECTOR, root_efe_loc, part=1)
    body += regid("*OSTA UDF Compliant", suffix=struct.pack("<H", 0x0250) + b"\x00" * 6)
    body += b"\x00" * 16
    body += b"\x00" * 16
    body += b"\x00" * 32
    assert len(body) == 496
    return tag(TID_FSD, bytes(body), 0)


def build_efe(ftype, flags, inf_len, perm, link, unique_id, l_ad, times, logblks, ads=b""):
    icb = bytearray()
    icb += struct.pack("<I", 0)
    icb += struct.pack("<HHH", 4, 0, 1)
    icb += bytes([0, ftype])
    icb += b"\x00" * 6
    icb += struct.pack("<H", flags)
    assert len(icb) == 20
    atime, mtime, ctime, attrtime = times
    body = bytearray()
    body += bytes(icb)
    body += struct.pack("<II", 0xFFFFFFFF, 0xFFFFFFFF)
    body += struct.pack("<I", perm)
    body += struct.pack("<H", link)
    body += struct.pack("<BB", 0, 0)
    body += struct.pack("<I", 0)
    body += struct.pack("<QQQ", inf_len, inf_len, logblks)
    body += atime + mtime + ctime + attrtime
    body += struct.pack("<II", 1, 0)
    body += b"\x00" * 32
    body += regid("*VsoSoftware")
    body += struct.pack("<Q", unique_id)
    body += struct.pack("<I", 0)
    body += struct.pack("<I", l_ad)
    assert len(body) == 200
    body += ads
    return tag(TID_EFE, bytes(body), 0)


def build_fid(name, fchar, icb_loc, tagloc, imp_uid=0):
    if name == "":
        l_fi = 0
        data = b""
    else:
        b = name.encode("utf-8")
        l_fi = 1 + len(b)
        data = b"\x08" + b
    body = bytearray()
    body += struct.pack("<H", 1)
    body += bytes([fchar, l_fi])
    body += struct.pack("<IIH", SECTOR, icb_loc, 1)
    body += b"\x00\x00" + struct.pack("<I", imp_uid)
    body += struct.pack("<H", 0)
    body += data
    n = (len(body) + 3) & ~3
    body += b"\x00" * (n - len(body))
    return tag(TID_FID, bytes(body), tagloc)


# ---------------------------------------------------------------- builder
def build_udf(src, out_path, label, volset_id=None, app_id="ConvertXToHD.exe 3.0.0.",
              root_time=None, times=None, exclude=()):
    if volset_id is None:
        volset_id = label
    if root_time is None:
        root_time = ts()
    nodes = build_nodes(src, exclude=exclude)

    dirs = [n for n in nodes if n.is_dir]
    files = [n for n in nodes if not n.is_dir]
    root = dirs[0]
    assert root.path == "/"

    for n in nodes:
        t = None
        if times:
            t = times.get(n.path)
            if t is None and n.path != "/":
                t = times.get(n.path.lstrip("/"))
        if t:
            n.atime, n.mtime, n.ctime, n.attrtime = t
        else:
            n.atime = n.mtime = n.ctime = n.attrtime = root_time

    children = OrderedDict()
    for n in nodes:
        if n.path != "/":
            children.setdefault(_parent(n.path), []).append(n)

    def sort_children(p):
        lst = sorted(children.get(p, []), key=lambda x: x.name)
        return [x for x in lst if not x.is_dir], [x for x in lst if x.is_dir]

    # ---- file chunks
    for n in files:
        if n.size > CHUNK:
            rem = n.size
            while rem > 0:
                l = min(CHUNK, rem)
                n.chunks.append([l, None])
                rem -= l
        else:
            n.chunks.append([n.size, None])

    # ---- rel sector placement (dir: EFE + FID; file: EFE), DFS order
    rel = 2

    def place(d):
        nonlocal rel
        d.efe_rel = rel
        rel += 1
        d.fid_rel = rel
        rel += 1
        files_c, dirs_c = sort_children(d.path)
        for n in files_c:
            n.efe_rel = rel
            rel += 1
        for n in dirs_c:
            place(n)

    place(root)
    content_sectors = rel
    meta_blocks = ((content_sectors + ALLOC_UNIT - 1) // ALLOC_UNIT) * ALLOC_UNIT

    # ---- file data placement (DFS order), fully contiguous
    cur = META_ANCHOR + meta_blocks
    for n in files:
        for c in n.chunks:
            c[1] = cur
            cur += (c[0] + SECTOR - 1) // SECTOR
    files_end = cur
    mirror_efe = ((files_end + ALLOC_UNIT - 1) // ALLOC_UNIT) * ALLOC_UNIT
    part_len = mirror_efe + 32 + meta_blocks

    # ---- unique ids: precompute in EFE emission order (dir EFE first,
    #      then files, then subdirs): root=0, first non-root gets 16
    uid = {}
    uid_counter = [16]

    def assign(d):
        if d.path == "/":
            uid[d.path] = 0
        else:
            uid[d.path] = uid_counter[0]
            uid_counter[0] += 1
        fc, dc = sort_children(d.path)
        for n in fc:
            uid[n.path] = uid_counter[0]
            uid_counter[0] += 1
        for n in dc:
            assign(n)

    assign(root)
    next_uid = uid_counter[0]
    node_by_path = {n.path: n for n in nodes}

    # ---- build metadata content (each entry tagged with its rel sector)
    meta = [build_fsd(label, root.efe_rel, root_time).ljust(SECTOR, b"\x00")]
    meta.append(retag(build_td(), 1).ljust(SECTOR, b"\x00"))

    def dir_fid(d):
        files_c, dirs_c = sort_children(d.path)
        parent = node_by_path[_parent(d.path)]
        out = [build_fid("", 0x0A, parent.efe_rel, d.fid_rel, imp_uid=uid[parent.path])]
        for n in files_c:
            out.append(build_fid(n.name, 0x00, n.efe_rel, d.fid_rel, imp_uid=uid[n.path]))
        for n in dirs_c:
            out.append(build_fid(n.name, 0x02, n.efe_rel, d.fid_rel, imp_uid=uid[n.path]))
        return b"".join(out)

    def emit(d):
        fid_content = dir_fid(d)
        files_c, dirs_c = sort_children(d.path)
        d.link = 1 + len(dirs_c)
        ads = struct.pack("<II", len(fid_content), d.fid_rel)
        meta.append(retag(build_efe(4, 0x20, len(fid_content), 0x14A5, d.link, uid[d.path], 8,
                                    (d.atime, d.mtime, d.ctime, d.attrtime),
                                    logblks=1, ads=ads), d.efe_rel).ljust(SECTOR, b"\x00"))
        meta.append(fid_content.ljust(SECTOR, b"\x00"))
        for n in files_c:
            ads = b""
            for l, loc in n.chunks:
                ads += long_ad(l, loc, part=0)
            meta.append(retag(build_efe(5, 0x21, n.size, 0x1084, 1, uid[n.path], len(ads),
                                        (n.atime, n.mtime, n.ctime, n.attrtime),
                                        logblks=n.blocks, ads=ads), n.efe_rel).ljust(SECTOR, b"\x00"))
        for n in dirs_c:
            emit(n)

    emit(root)
    assert len(meta) == content_sectors

    meta_len = meta_blocks * SECTOR
    meta_file_efe = build_efe(0xFA, 0x20, meta_len, 0x14A5, 0, 0, 8,
                              (root_time,) * 4, logblks=meta_blocks,
                              ads=struct.pack("<II", meta_len, META_ANCHOR))
    mirror_efe_desc = build_efe(0xFB, 0x20, meta_len, 0x14A5, 0, 0, 8,
                                (root_time,) * 4, logblks=meta_blocks,
                                ads=struct.pack("<II", meta_len, mirror_efe + 32))

    # ---- assemble image
    total = PART_START + part_len + PART_START
    reserve_vds = PART_START + part_len + 32
    reserve_avdp = total - 1

    pvd = build_pvd(label, volset_id, app_id, root_time)
    iuvd = build_iuvd(label)
    pd = build_pd(part_len)
    lvd = build_lvd(label, (4096, 0), part_len, meta_blocks, mirror_efe)
    usd = build_usd()
    td = build_td()
    vds = [pvd, iuvd, pd, lvd, usd, td]
    lvid = build_lvid(part_len, meta_blocks, next_uid, len(files), len(dirs), root_time)
    avdp_main = tag(TID_AVDP, build_avdp_body(reserve_vds), AVDP_MAIN)
    avdp_res = tag(TID_AVDP, build_avdp_body(reserve_vds), reserve_avdp)

    def write_zero(f, n):
        f.write(b"\x00" * (n * SECTOR))

    def write_file(f, n):
        with open(os.path.join(src, n.path.lstrip("/")), "rb") as srcf:
            for l, _ in n.chunks:
                remaining = l
                while remaining > 0:
                    chunk = srcf.read(min(SECTOR, remaining))
                    f.write(chunk)
                    if len(chunk) < SECTOR:
                        f.write(b"\x00" * (SECTOR - len(chunk)))
                    remaining -= SECTOR

    with open(out_path, "wb") as f:
        write_zero(f, 16)
        for sig in (b"BEA01", b"NSR03", b"TEA01"):
            f.write(b"\x00" + sig + b"\x01" + b"\x00" * (SECTOR - 7))
        write_zero(f, 13)
        for i, body in enumerate(vds):
            f.write(retag(body, VDS_MAIN + i).ljust(SECTOR, b"\x00"))
        write_zero(f, LVID_LOC - VDS_MAIN - 6)
        f.write(retag(lvid, LVID_LOC).ljust(SECTOR, b"\x00"))
        f.write(retag(td, LVID_LOC + 1).ljust(SECTOR, b"\x00"))
        write_zero(f, AVDP_MAIN - LVID_LOC - 2)
        f.write(avdp_main.ljust(SECTOR, b"\x00"))
        write_zero(f, PART_START - AVDP_MAIN - 1)
        # partition 0
        f.write(retag(meta_file_efe, 0).ljust(SECTOR, b"\x00"))
        write_zero(f, 31)
        for sec in meta:
            f.write(sec)
        write_zero(f, meta_blocks - content_sectors)
        for n in files:
            write_file(f, n)
        write_zero(f, mirror_efe - files_end)
        f.write(retag(mirror_efe_desc, mirror_efe).ljust(SECTOR, b"\x00"))
        write_zero(f, 31)
        for sec in meta:
            f.write(sec)
        write_zero(f, meta_blocks - content_sectors)
        write_zero(f, reserve_vds - (PART_START + part_len))
        for i, body in enumerate(vds):
            f.write(retag(body, reserve_vds + i).ljust(SECTOR, b"\x00"))
        write_zero(f, 10)
        write_zero(f, reserve_avdp - (reserve_vds + 16))
        f.write(avdp_res.ljust(SECTOR, b"\x00"))
    return total


if __name__ == "__main__":
    import json
    import argparse

    p = argparse.ArgumentParser(
        prog="udf250",
        description="Build a UDF 2.50 filesystem image (AVCHD/Blu-ray) from a "
                    "source directory. The source directory becomes the ISO root.")
    p.add_argument("src", help="source directory to store in the image (ISO root)")
    p.add_argument("out", help="output image file path")
    p.add_argument("label", nargs="?", default="UDF_2_50_DISC",
                   help="volume label (default: UDF_2_50_DISC)")
    p.add_argument("--volset", default=None,
                   help="volume set identifier (default: same as label)")
    p.add_argument("--manifest", default=None, metavar="JSON",
                   help="optional manifest of per-file timestamps (developer use)")
    p.add_argument("--exclude", action="append", default=[], metavar="NAME",
                   help="top-level entry to omit from the image (repeatable)")
    a = p.parse_args()

    times = None
    if a.manifest:
        m = json.load(open(a.manifest))
        times = {}
        for n in m["nodes"]:
            times[n["path"]] = (bytes.fromhex(n["atime"]), bytes.fromhex(n["mtime"]),
                                bytes.fromhex(n["ctime"]), bytes.fromhex(n["attrtime"]))
    n = build_udf(a.src, a.out, a.label, volset_id=a.volset, times=times, exclude=a.exclude)
    print(f"wrote {a.out}: {n} sectors ({n * SECTOR} bytes)")
