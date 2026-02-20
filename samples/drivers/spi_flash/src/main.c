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
#include <string.h>

#if defined(CONFIG_BOARD_ADAFRUIT_FEATHER_STM32F405)
#define SPI_FLASH_TEST_REGION_OFFSET 0xf000
#elif defined(CONFIG_BOARD_ARTY_A7_DESIGNSTART_FPGA_CORTEX_M1) ||                                  \
	defined(CONFIG_BOARD_ARTY_A7_DESIGNSTART_FPGA_CORTEX_M3)
/* The FPGA bitstream is stored in the lower 536 sectors of the flash. */
#define SPI_FLASH_TEST_REGION_OFFSET DT_REG_SIZE(DT_NODE_BY_FIXED_PARTITION_LABEL(fpga_bitstream))
#elif defined(CONFIG_BOARD_NPCX9M6F_EVB) || defined(CONFIG_BOARD_NPCX7M6FB_EVB)
#define SPI_FLASH_TEST_REGION_OFFSET 0x7F000
#elif defined(CONFIG_BOARD_EK_RA8M1) || defined(CONFIG_BOARD_EK_RA8D1)
#define SPI_FLASH_TEST_REGION_OFFSET 0x40000
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32_qspi_nor) && DT_PROP(DT_NODELABEL(quadspi), dual_flash)
#define SPI_FLASH_TEST_REGION_OFFSET 0xfe000
#else
#define SPI_FLASH_TEST_REGION_OFFSET 0xff000
#endif
#if defined(CONFIG_BOARD_EK_RA8M1) || defined(CONFIG_BOARD_EK_RA8D1)
#define SPI_FLASH_SECTOR_SIZE 262144
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32_qspi_nor) && DT_PROP(DT_NODELABEL(quadspi), dual_flash)
#define SPI_FLASH_SECTOR_SIZE 8192
#else
#define SPI_FLASH_SECTOR_SIZE 4096
#endif

#if defined(CONFIG_FLASH_STM32_OSPI) || defined(CONFIG_FLASH_STM32_QSPI) ||                        \
	defined(CONFIG_FLASH_STM32_XSPI) || defined(CONFIG_FLASH_RENESAS_RA_OSPI_B) ||             \
	defined(CONFIG_FLASH_RENESAS_RA_QSPI) || defined(CONFIG_FLASH_RENESAS_RZ_QSPI_XSPI) ||     \
	defined(CONFIG_FLASH_RENESAS_RZ_QSPI_SPIBSC)
#define SPI_FLASH_MULTI_SECTOR_TEST
#endif

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

#if defined(CONFIG_FLASH_RENESAS_RA_OSPI_B)
const uint8_t erased[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
#else
const uint8_t erased[] = {0xff, 0xff, 0xff, 0xff};
#endif

void single_sector_test(const struct device *flash_dev)
{
#if defined(CONFIG_FLASH_RENESAS_RA_OSPI_B)
	const uint8_t expected[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
#else
	const uint8_t expected[] = {0x55, 0xaa, 0x66, 0x99};
#endif
	const size_t len = sizeof(expected);
	uint8_t buf[sizeof(expected)];
	int rc;

	printf("\nPerform test on single sector");
	/* Write protection needs to be disabled before each write or
	 * erase, since the flash component turns on write protection
	 * automatically after completion of write and erase
	 * operations.
	 */
	printf("\nTest 1: Flash erase\n");

	/* Full flash erase if SPI_FLASH_TEST_REGION_OFFSET = 0 and
	 * SPI_FLASH_SECTOR_SIZE = flash size
	 */
	rc = flash_erase(flash_dev, SPI_FLASH_TEST_REGION_OFFSET, SPI_FLASH_SECTOR_SIZE);
	if (rc != 0) {
		printf("Flash erase failed! %d\n", rc);
	} else {
		/* Check erased pattern */
		memset(buf, 0, len);
		rc = flash_read(flash_dev, SPI_FLASH_TEST_REGION_OFFSET, buf, len);
		if (rc != 0) {
			printf("Flash read failed! %d\n", rc);
			return;
		}
		if (memcmp(erased, buf, len) != 0) {
			printf("Flash erase failed at offset 0x%x got 0x%x\n",
					SPI_FLASH_TEST_REGION_OFFSET, *(uint32_t *)buf);
			return;
		}
		printf("Flash erase succeeded!\n");
	}
	printf("\nTest 2: Flash write\n");

	printf("Attempting to write %zu bytes\n", len);
	rc = flash_write(flash_dev, SPI_FLASH_TEST_REGION_OFFSET, expected, len);
	if (rc != 0) {
		printf("Flash write failed! %d\n", rc);
		return;
	}

	memset(buf, 0, len);
	rc = flash_read(flash_dev, SPI_FLASH_TEST_REGION_OFFSET, buf, len);
	if (rc != 0) {
		printf("Flash read failed! %d\n", rc);
		return;
	}

	if (memcmp(expected, buf, len) == 0) {
		printf("Data read matches data written. Good!!\n");
	} else {
		const uint8_t *wp = expected;
		const uint8_t *rp = buf;
		const uint8_t *rpe = rp + len;

		printf("Data read does not match data written!!\n");
		while (rp < rpe) {
			printf("%08x wrote %02x read %02x %s\n",
					(uint32_t)(SPI_FLASH_TEST_REGION_OFFSET + (rp - buf)), *wp, *rp,
					(*rp == *wp) ? "match" : "MISMATCH");
			++rp;
			++wp;
		}
	}
}

#if defined SPI_FLASH_MULTI_SECTOR_TEST
void multi_sector_test(const struct device *flash_dev)
{
#if defined(CONFIG_FLASH_RENESAS_RA_OSPI_B)
	const uint8_t expected[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
#else
	const uint8_t expected[] = {0x55, 0xaa, 0x66, 0x99};
#endif
	const size_t len = sizeof(expected);
	uint8_t buf[sizeof(expected)];
	int rc;

	printf("\nPerform test on multiple consecutive sectors");

	/* Write protection needs to be disabled before each write or
	 * erase, since the flash component turns on write protection
	 * automatically after completion of write and erase
	 * operations.
	 */
	printf("\nTest 1: Flash erase\n");

	/* Full flash erase if SPI_FLASH_TEST_REGION_OFFSET = 0 and
	 * SPI_FLASH_SECTOR_SIZE = flash size
	 * Erase 2 sectors for check for erase of consequtive sectors
	 */
	rc = flash_erase(flash_dev, SPI_FLASH_TEST_REGION_OFFSET, SPI_FLASH_SECTOR_SIZE * 2);
	if (rc != 0) {
		printf("Flash erase failed! %d\n", rc);
	} else {
		/* Read the content and check for erased */
		memset(buf, 0, len);
		size_t offs = SPI_FLASH_TEST_REGION_OFFSET;

		while (offs < SPI_FLASH_TEST_REGION_OFFSET + 2 * SPI_FLASH_SECTOR_SIZE) {
			rc = flash_read(flash_dev, offs, buf, len);
			if (rc != 0) {
				printf("Flash read failed! %d\n", rc);
				return;
			}
			if (memcmp(erased, buf, len) != 0) {
				printf("Flash erase failed at offset 0x%x got 0x%x\n", offs,
						*(uint32_t *)buf);
				return;
			}
			offs += SPI_FLASH_SECTOR_SIZE;
		}
		printf("Flash erase succeeded!\n");
	}

	printf("\nTest 2: Flash write\n");

	size_t offs = SPI_FLASH_TEST_REGION_OFFSET;

	while (offs < SPI_FLASH_TEST_REGION_OFFSET + 2 * SPI_FLASH_SECTOR_SIZE) {
		printf("Attempting to write %zu bytes at offset 0x%x\n", len, offs);
		rc = flash_write(flash_dev, offs, expected, len);
		if (rc != 0) {
			printf("Flash write failed! %d\n", rc);
			return;
		}

		memset(buf, 0, len);
		rc = flash_read(flash_dev, offs, buf, len);
		if (rc != 0) {
			printf("Flash read failed! %d\n", rc);
			return;
		}

		if (memcmp(expected, buf, len) == 0) {
			printf("Data read matches data written. Good!!\n");
		} else {
			const uint8_t *wp = expected;
			const uint8_t *rp = buf;
			const uint8_t *rpe = rp + len;

			printf("Data read does not match data written!!\n");
			while (rp < rpe) {
				printf("%08x wrote %02x read %02x %s\n",
						(uint32_t)(offs + (rp - buf)), *wp, *rp,
						(*rp == *wp) ? "match" : "MISMATCH");
				++rp;
				++wp;
			}
		}
		offs += SPI_FLASH_SECTOR_SIZE;
	}
}
#endif

/* Buffer size for full flash operations */
#define FULL_FLASH_BUFFER_SIZE 4096

void full_flash_test(const struct device *flash_dev)
{
	const struct flash_parameters *params = flash_get_parameters(flash_dev);
	uint64_t flash_size_u64;
	size_t flash_size;
	size_t write_block_size = params->write_block_size;
	size_t erase_block_size;
	struct flash_pages_info page_info;
	static uint8_t write_buf[FULL_FLASH_BUFFER_SIZE];
	static uint8_t read_buf[FULL_FLASH_BUFFER_SIZE];
	size_t offset = 0;
	int rc;
	size_t mismatches = 0;
	size_t total_bytes_written = 0;
	size_t total_bytes_read = 0;

	/* Get flash size */
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

	printf("\n========================================\n");
	printf("Full Flash Device Test\n");
	printf("========================================\n");
	printf("Flash size: %zu bytes (0x%zx)\n", flash_size, flash_size);
	printf("Write block size: %zu bytes\n", write_block_size);
	printf("Erase block size: %zu bytes\n", erase_block_size);
	printf("========================================\n");

	/* Step 1: Erase entire flash */
	printf("\nStep 1: Erasing entire flash device...\n");
	printf("This may take a while for large flash devices...\n");

	offset = 0;
	while (offset < flash_size) {
		size_t erase_len = erase_block_size;
		if (offset + erase_len > flash_size) {
			erase_len = flash_size - offset;
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

	/* Step 2: Write pattern to entire flash */
	printf("\nStep 2: Writing pattern to entire flash device...\n");

	/* Generate a pattern that varies with address */
	for (size_t i = 0; i < FULL_FLASH_BUFFER_SIZE; i++) {
		write_buf[i] = (uint8_t)(i & 0xFF);
	}

	offset = 0;
	total_bytes_written = 0;
	while (offset < flash_size) {
		size_t write_len = FULL_FLASH_BUFFER_SIZE;
		if (offset + write_len > flash_size) {
			write_len = flash_size - offset;
		}

		/* Update pattern based on offset */
		for (size_t i = 0; i < write_len; i++) {
			write_buf[i] = (uint8_t)((offset + i) & 0xFF);
		}

		if ((offset % (64 * 1024)) == 0 || offset + write_len >= flash_size) {
			printf("Writing at offset 0x%zx (%.1f%% complete)...\r", offset,
					(100.0 * offset) / flash_size);
		}

		rc = flash_write(flash_dev, offset, write_buf, write_len);
		if (rc != 0) {
			printf("\nFlash write failed at offset 0x%zx! Error: %d\n", offset, rc);
			return;
		}
		total_bytes_written += write_len;
		offset += write_len;
	}
	printf("\nFlash write completed successfully!\n");
	printf("Total bytes written: %zu\n", total_bytes_written);

	/* Step 3: Read back and verify entire flash */
	printf("\nStep 3: Reading back and verifying entire flash device...\n");

	offset = 0;
	total_bytes_read = 0;
	mismatches = 0;

	while (offset < flash_size) {
		size_t read_len = FULL_FLASH_BUFFER_SIZE;
		if (offset + read_len > flash_size) {
			read_len = flash_size - offset;
		}

		if ((offset % (64 * 1024)) == 0 || offset + read_len >= flash_size) {
			printf("Reading at offset 0x%zx (%.1f%% complete)...\r", offset,
					(100.0 * offset) / flash_size);
		}

		rc = flash_read(flash_dev, offset, read_buf, read_len);
		if (rc != 0) {
			printf("\nFlash read failed at offset 0x%zx! Error: %d\n", offset, rc);
			return;
		}

		/* Verify the data */
		for (size_t i = 0; i < read_len; i++) {
			uint8_t expected = (uint8_t)((offset + i) & 0xFF);
			if (read_buf[i] != expected) {
				if (mismatches < 10) {
					printf("\nMismatch at offset 0x%zx: expected 0x%02x, got "
							"0x%02x\n",
							offset + i, expected, read_buf[i]);
				}
				mismatches++;
			}
		}

		total_bytes_read += read_len;
		offset += read_len;
	}

	printf("\nFlash read completed!\n");
	printf("Total bytes read: %zu\n", total_bytes_read);

	if (mismatches == 0) {
		printf("\n========================================\n");
		printf("SUCCESS: All data verified correctly!\n");
		printf("========================================\n");
	} else {
		printf("\n========================================\n");
		printf("ERROR: Found %zu mismatches!\n", mismatches);
		printf("========================================\n");
	}
}

int main(void)
{
	const struct device *flash_dev = DEVICE_DT_GET_ONE(SPI_FLASH_COMPAT);

	if (!device_is_ready(flash_dev)) {
		printk("%s: device not ready.\n", flash_dev->name);
		return 0;
	}

	printf("\n%s SPI flash testing\n", flash_dev->name);
	printf("==========================\n");

	single_sector_test(flash_dev);
#if defined SPI_FLASH_MULTI_SECTOR_TEST
	multi_sector_test(flash_dev);
#endif

	/* Full flash device test */
	full_flash_test(flash_dev);

	return 0;
}
