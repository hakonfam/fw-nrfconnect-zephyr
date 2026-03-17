/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <stdio.h>

#if DT_HAS_COMPAT_STATUS_OKAY(jedec_spi_nor)
#define SPI_FLASH_COMPAT jedec_spi_nor
#elif DT_HAS_COMPAT_STATUS_OKAY(jedec_mspi_nor)
#define SPI_FLASH_COMPAT jedec_mspi_nor
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32_qspi_nor)
#define SPI_FLASH_COMPAT st_stm32_qspi_nor
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32_ospi_nor)
#define SPI_FLASH_COMPAT st_stm32_ospi_nor
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32_xspi_nor)
#define SPI_FLASH_COMPAT st_stm32_xspi_nor
#elif DT_HAS_COMPAT_STATUS_OKAY(nordic_qspi_nor)
#define SPI_FLASH_COMPAT nordic_qspi_nor
#elif DT_HAS_COMPAT_STATUS_OKAY(renesas_ra_ospi_b_nor)
#define SPI_FLASH_COMPAT renesas_ra_ospi_b_nor
#elif DT_HAS_COMPAT_STATUS_OKAY(renesas_ra_qspi_nor)
#define SPI_FLASH_COMPAT renesas_ra_qspi_nor
#elif DT_HAS_COMPAT_STATUS_OKAY(renesas_rz_qspi_xspi)
#define SPI_FLASH_COMPAT renesas_rz_qspi_xspi
#elif DT_HAS_COMPAT_STATUS_OKAY(renesas_rz_qspi_spibsc)
#define SPI_FLASH_COMPAT renesas_rz_qspi_spibsc
#elif DT_HAS_COMPAT_STATUS_OKAY(nxp_xspi_nor)
#define SPI_FLASH_COMPAT nxp_xspi_nor
#elif DT_HAS_COMPAT_STATUS_OKAY(nxp_imx_flexspi_nor)
#define SPI_FLASH_COMPAT nxp_imx_flexspi_nor
#else
#define SPI_FLASH_COMPAT invalid
#endif

#define ERASE_SIZE_TARGET (5 * 1024 * 1024) /* 5MB */

void erase_5mb(const struct device *flash_dev)
{
	uint64_t flash_size_u64;
	size_t flash_size;
	size_t erase_block_size;
	struct flash_pages_info page_info;
	size_t offset = 0;
	size_t erase_size;
	int rc;

	/* Get flash size (may report partition size, not full device size) */
	rc = flash_get_size(flash_dev, &flash_size_u64);
	if (rc != 0) {
		printf("Failed to get flash size! Error: %d\n", rc);
		return;
	}
	flash_size = (size_t)flash_size_u64;

	/* Get erase block size from page layout */
	rc = flash_get_page_info_by_offs(flash_dev, 0, &page_info);
	if (rc != 0) {
		printf("Failed to get page info! Error: %d\n", rc);
		return;
	}
	erase_block_size = page_info.size;

	/* Force erase 5MB regardless of reported size (device is 128Mbit = 16MB) */
	erase_size = ERASE_SIZE_TARGET;

	if (flash_size < ERASE_SIZE_TARGET) {
		printf("Warning: Reported flash size (%zu bytes) is less than 5MB target\n",
		       flash_size);
		printf("Proceeding with 5MB erase anyway (device is 128Mbit = 16MB)\n");
	}

	/* Round up erase_size to nearest erase block boundary to ensure proper alignment */
	size_t erase_blocks_needed = (erase_size + erase_block_size - 1) / erase_block_size;
	size_t erase_size_aligned = erase_blocks_needed * erase_block_size;

	printf("Flash device: %s\n", flash_dev->name);
	printf("Flash size: %zu bytes (0x%zx) - %.2f MB\n", flash_size, flash_size,
	       flash_size / (1024.0 * 1024.0));
	printf("Erase block size: %zu bytes\n", erase_block_size);
	printf("Target erase size: %zu bytes (0x%zx) - %.2f MB\n", erase_size, erase_size,
	       erase_size / (1024.0 * 1024.0));
	printf("Aligned erase size: %zu bytes (0x%zx) - %.2f MB\n", erase_size_aligned,
	       erase_size_aligned, erase_size_aligned / (1024.0 * 1024.0));
	printf("Erasing %zu blocks...\n", erase_blocks_needed);

	offset = 0;
	while (offset < erase_size_aligned) {
		size_t erase_len = erase_block_size;
		if (offset + erase_len > erase_size_aligned) {
			erase_len = erase_size_aligned - offset;
		}

		printf("Erasing block at offset 0x%zx (0x%zx bytes)...\r", offset, erase_len);
		rc = flash_erase(flash_dev, offset, erase_len);
		if (rc != 0) {
			printf("\nFlash erase failed at offset 0x%zx! Error: %d\n", offset, rc);
			return;
		}
		offset += erase_len;
	}
	printf("\nFlash erase completed successfully!\n");
	printf("Erased %zu bytes (0x%zx) - %.2f MB\n", erase_size_aligned, erase_size_aligned,
	       erase_size_aligned / (1024.0 * 1024.0));
}

int main(void)
{
	const struct device *flash_dev = DEVICE_DT_GET_ONE(SPI_FLASH_COMPAT);

	if (!device_is_ready(flash_dev)) {
		printk("%s: device not ready.\n", flash_dev->name);
		return 0;
	}

	printf("\n%s SPI flash erase test\n", flash_dev->name);
	printf("==========================\n");

	erase_5mb(flash_dev);

	printf("\nDONE\n");
	printf("==========================\n");

	return 0;
}
