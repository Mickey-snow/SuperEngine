
# 0) Common to all formats

- **Endianness.** All integer fields in the files are **little-endian**.

- **Output pixel format.** Bytes in memory = **B, G, R, A**. Numerically the 32-bit value for one pixel is `0xAARRGGBB`.

---

# 1) PDT container

## Header (common to PDT10 and PDT11)

Offsets are from the beginning of the file:

| Offset | Size | Meaning                                       |
| -----: | ---: | --------------------------------------------- |
|   0x00 |    5 | ASCII `"PDT10"` or `"PDT11"`                  |
|   0x05 |    3 | Reserved / ignored                            |
|   0x08 |  u32 | **File size** (must equal actual file length) |
|   0x0C |  u32 | **Width** (pixels)                            |
|   0x10 |  u32 | **Height** (pixels)                           |
|   0x14 |  u32 | Mask origin X (unused; files seen are 0)      |
|   0x18 |  u32 | Mask origin Y (unused; files seen are 0)      |
|   0x1C |  u32 | **Mask stream offset** (0 if no mask)         |

The presence of a non-zero `mask_offset` marks the image as “has mask”; if present, an additional **LZSS** compressed stream supplies per-pixel alpha.

## PDT10 Layout (truecolor + optional external mask)

* **Image stream**: from `0x20` up to `mask_offset` (if any) or EOF. Encoded with **PDT10 LZ**.
* **Optional mask stream**: from `mask_offset` to EOF. Encoded with **Mask LZ**.

## PDT11 (paletted + optional external mask)

**Additional header blocks**

* **Palette (color table)** at **0x20**: 256 × u32 colors. Values are used verbatim as `0xAARRGGBB`.
* **Index distance table** at **0x420**: 16 × u32 distances used by the custom LZ for the index stream.
* **Index stream** starts at **0x460** and runs up to `mask_offset` (if present) or EOF.

---

# 2) G00 container

## Common header

| Offset | Size | Meaning              |
| -----: | ---: | -------------------- |
|   0x00 |   u8 | **Type**: 0, 1, 2, 3 |
|   0x01 |  u16 | **Width**            |
|   0x03 |  u16 | **Height**           |

The rest of the header depends on `Type`.

## Type 0 — “Raw truecolor” (compressed 24-bpp)

| Offset | Size | Meaning                       |
| -----: | ---: | ----------------------------- |
|   0x05 |  u32 | **Compressed size**           |
|   0x09 |  u32 | **Uncompressed size** (bytes) |
|   0x0D |      | **Data** (compressed)         |

**Validity:** file length must equal `0x05 + compressed_size`.

**Decoding**

* Decompress with **G00Type0 LZ**. Output is raw 24-bpp B, G, R bytes.
* Convert to 32-bpp ARGB by setting `A=0xFF` for every pixel.

## Type 1 — “Indexed color” (compressed indices + inline palette)

| Offset | Size | Meaning                       |
| -----: | ---: | ----------------------------- |
|   0x05 |  u32 | **Compressed size**           |
|   0x09 |  u32 | **Uncompressed size** (bytes) |
|   0x0D |      | **Data** (compressed)         |

**Decoded payload layout** (at the start of the uncompressed block):

|  Offset | Size | Meaning                                 |
| ------: | ---: | --------------------------------------- |
|    0x00 |  u16 | **Palette length** `N` (0…256; clamped) |
|    0x02 |  N×4 | Palette entries (u32 `0xAARRGGBB`)      |
| 0x02+4N |      | **Pixel indices** (one byte per pixel)  |

**Decoding**

* Decompress with **SCN2k LZ**.
* Read the palette length and entries, then map each index -> 32-bit color. Alpha comes from the palette.

## Type 2 — “Cut data” (tiled/regioned, may include alpha)

**Header & region table**

| Offset | Size | Meaning  |
| -----: | ---: | ---------------- |
|   0x05 |  u16 | **Region count** `M`  |
|   0x09 | M×24 | **Region table**   |

**Region table**: each entry (6×u32) `x1,y1,x2,y2,origin_x,origin_y`, should to canvas.

There’s a special case: if all `M>1` regions are identical, the loader **stacks** them vertically by shifting each region’s `y` by `i*height` and multiplying the overall canvas height by `M`. (This is used by some multi-frame assets.)

**Data block**

Immediately after the region table:

| Offset (from 0x09) | Size | Meaning               |
| -----------------------: | ---: | --------------------- |
|                    +0x00 |  u32 | **Compressed size**   |
|                    +0x04 |  u32 | **Uncompressed size** |
|                    +0x08 |    | **Compressed data**   |

**Uncompressed block layout**:

| Offset | Size | Meaning                                                         |
| -----: | ---: | --------------------------------------------------------------- |
|   0x00 |  u32 | **Region count** (repeat; may cap the earlier `M`)              |
|   0x04 |  M×8 | For each region *i*: `{ offset_i (u32), length_i (u32) }`       |
|        |      | Region payloads at `offset_i` (relative to start of this block) |

Each **region payload** contains a 0x74-byte header, followed by one or more **rect entries**:

* **Rect header**: 0x5C bytes

  * `x` = `s16` at +0x00
  * `y` = `s16` at +0x02
  * (2 bytes unknown at +0x04)
  * `w` = `s16` at +0x06
  * `h` = `s16` at +0x08
  * rest unknown/ignored
* **Rect pixels**: `w * h * 4` bytes of 32-bpp **little-endian** colors.

**Placement**

For each rect, pixels are blitted at `(region.x1 + x, region.y1 + y)` onto the output canvas. Pixel values are copied as 32-bit little-endian integers; alpha is therefore whatever the source block encodes.

**Compression**

The whole uncompressed block is produced by **SCN2k LZ**.
