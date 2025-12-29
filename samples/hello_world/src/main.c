/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define START_ADDRESS 0x0e1f1000
#define MAGIC_VALUE 0xBA53BA11
#define NUM_4BYTE_WORDS ((0x0e1fd000 - START_ADDRESS) / 4)

int main(void)
{
	printf("Hello World! %s\n", CONFIG_BOARD_TARGET);

	uintptr_t address = START_ADDRESS;
	bool found_start = false;
	uint32_t read_value;

	for (int i = 0; i < NUM_4BYTE_WORDS; i++) {
		read_value = *(uint32_t *)address;
		*(uint32_t *)address = MAGIC_VALUE;

		printf("r: 0x%x\n", read_value);
		printf("w: 0x%lx\n", address);

		address += 4;

	}

	printf("Done\n");

	return 0;
}
