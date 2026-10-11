MCU_SERIES = m4
MCU_VARIANT = nrf52
MCU_SUB_VARIANT = nrf52832
SOFTDEV_VERSION = 6.1.1
LD_FILES += boards/nrf52832_512k_64k_bldr78.ld

NRF_DEFINES += -DNRF52832_XXAA


MICROPY_VFS_LFS2 = 1

# sd_ble_enable() reports 0x200023a8 (s132) or 0x20001ca0 (s112) for this board's BLE configuration; the rest of the reservation goes to the heap.
ifeq ($(SD),s112)
LDFLAGS += -Wl,--defsym=_sd_ram_used=0x1d00
else
LDFLAGS += -Wl,--defsym=_sd_ram_used=0x2400
endif

# Internal flash below the bootloader holds a ROMFS image that runs in place and is rewritten without a firmware update.
ROMFS_SIZE ?= $(if $(filter s112,$(SD_LOWER)),0x10000,0x4000)
LDFLAGS += -Wl,--defsym=_micropy_hw_romfs_part0_size=$(ROMFS_SIZE)
