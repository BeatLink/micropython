MCU_SERIES = m4
MCU_VARIANT = nrf52
MCU_SUB_VARIANT = nrf52832
SOFTDEV_VERSION = 6.1.1
LD_FILES += boards/nrf52832_512k_64k_bldr78.ld

NRF_DEFINES += -DNRF52832_XXAA


MICROPY_VFS_LFS2 = 1

# sd_ble_enable() reports 0x200023a8 for this board's BLE configuration; the rest of the reservation goes to the heap.
LDFLAGS += -Wl,--defsym=_sd_ram_used=0x2400
