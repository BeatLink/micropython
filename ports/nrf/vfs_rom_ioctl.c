/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "py/obj.h"
#include "py/objarray.h"
#include "py/mperrno.h"
#include "extmod/vfs.h"
#include "drivers/flash.h"

#if MICROPY_VFS_ROM_IOCTL

// The partition sits between the application and the filesystem, as memory.ld lays it out.
extern byte _micropy_hw_romfs_part0_start[];
extern byte _micropy_hw_romfs_part0_size[];

#define ROMFS_BASE ((uint32_t)_micropy_hw_romfs_part0_start)
#define ROMFS_BYTES ((uint32_t)_micropy_hw_romfs_part0_size)

// Flash is written a word at a time.
#define ROMFS_MIN_WRITE (4)

// The linker gives the partition's address, which a static initialiser cannot take as an integer.
static MP_DEFINE_MEMORYVIEW_OBJ(romfs_obj, 'B', 0, 0, NULL);

// Tell whether a range lies wholly inside the partition.
static bool romfs_range_ok(mp_int_t offset, mp_int_t len) {
    return offset >= 0 && len >= 0 && (mp_uint_t)offset + (mp_uint_t)len <= ROMFS_BYTES;
}

mp_obj_t mp_vfs_rom_ioctl(size_t n_args, const mp_obj_t *args) {
    if (ROMFS_BYTES == 0) {
        return MP_OBJ_NEW_SMALL_INT(-MP_EINVAL);
    }
    mp_int_t cmd = mp_obj_get_int(args[0]);
    if (cmd == MP_VFS_ROM_IOCTL_GET_NUMBER_OF_SEGMENTS) {
        return MP_OBJ_NEW_SMALL_INT(1);
    }

    // Every other command names a segment, and there is only the one.
    if (n_args < 2 || mp_obj_get_int(args[1]) != 0) {
        return MP_OBJ_NEW_SMALL_INT(-MP_EINVAL);
    }

    switch (cmd) {
        case MP_VFS_ROM_IOCTL_GET_SEGMENT: {
            mp_obj_memoryview_init(&romfs_obj, 'B', 0, ROMFS_BYTES, (void *)ROMFS_BASE);
            return MP_OBJ_FROM_PTR(&romfs_obj);
        }

        case MP_VFS_ROM_IOCTL_GET_MIN_PREPARE:
            return MP_OBJ_NEW_SMALL_INT(FLASH_PAGESIZE);

        case MP_VFS_ROM_IOCTL_WRITE_PREPARE: {
            // Erase every page the range touches; the offset defaults to the start of the partition.
            if (n_args < 3) {
                return MP_OBJ_NEW_SMALL_INT(-MP_EINVAL);
            }
            mp_int_t offset = 0;
            mp_int_t len = mp_obj_get_int(args[n_args - 1]);
            if (n_args == 4) {
                offset = mp_obj_get_int(args[2]);
            }
            if (!romfs_range_ok(offset, len) || !FLASH_IS_PAGE_ALIGNED(offset)) {
                return MP_OBJ_NEW_SMALL_INT(-MP_EINVAL);
            }
            for (mp_int_t at = offset; at < offset + len; at += FLASH_PAGESIZE) {
                flash_page_erase(ROMFS_BASE + at);
            }
            return MP_OBJ_NEW_SMALL_INT(ROMFS_MIN_WRITE);
        }

        case MP_VFS_ROM_IOCTL_WRITE: {
            if (n_args < 4) {
                return MP_OBJ_NEW_SMALL_INT(-MP_EINVAL);
            }
            mp_int_t offset = mp_obj_get_int(args[2]);
            mp_buffer_info_t bufinfo;
            mp_get_buffer_raise(args[3], &bufinfo, MP_BUFFER_READ);
            if (!romfs_range_ok(offset, bufinfo.len)) {
                return MP_OBJ_NEW_SMALL_INT(-MP_EINVAL);
            }
            flash_write_bytes(ROMFS_BASE + offset, bufinfo.buf, bufinfo.len);
            return MP_OBJ_NEW_SMALL_INT(0);
        }

        case MP_VFS_ROM_IOCTL_WRITE_COMPLETE:
            return MP_OBJ_NEW_SMALL_INT(0);

        default:
            return MP_OBJ_NEW_SMALL_INT(-MP_EINVAL);
    }
}

#endif // MICROPY_VFS_ROM_IOCTL
