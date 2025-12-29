/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define START_ADDRESS 0x0e100000
#define MAGIC_VALUE 0xBA53BA11
#define NUM_4BYTE_WORDS ((1024*1024) / 4)

int main(void)
{
	printf("Hello World! %s\n", CONFIG_BOARD_TARGET);

	uintptr_t address = START_ADDRESS;
	bool found_start = false;
	uint32_t read_value;

	for (int i = 0; i < NUM_4BYTE_WORDS; i++) {
		read_value = *(uint32_t *)address;

		if (found_start) {
			*(uint32_t *)address = MAGIC_VALUE;
			printf("w: 0x%lx\n", address);
		} else {
			printf("read: 0x%x\n", read_value);
			found_start = (read_value != MAGIC_VALUE);
		}

		address += 4;

	}

	printf("Done\n");

	return 0;
}
