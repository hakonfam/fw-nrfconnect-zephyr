/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define START_ADDRESS 0x0e100000
#define END_ADDRESS 0x0e180000
#define MAGIC_VALUE 0xBA53BA11
#define NUM_4BYTE_WORDS ((END_ADDRESS- START_ADDRESS) / 4)

void read_write_cycle(void)
{
	uint32_t value = 0;
	uintptr_t address = START_ADDRESS;
	uint32_t read_value;

	for (int i = 0; i < NUM_4BYTE_WORDS/4; i++) {
		for (int j = 0; j < 4; j++) {
			if (address >= END_ADDRESS) {
				return;
			}

			read_value = *(uint32_t *)address;
			*(uint32_t *)address = value++;
			address += 4;
		}

		if ( i % 0x1000 == 0) {
			printf("r1: 0x%x ", read_value);
			printf("w: 0x%lx ", address);
			printf("r2: 0x%x\n", *(uint32_t *)address);
		}
	}
}

int main(void)
{
	printf("Hello World! %s\n", CONFIG_BOARD_TARGET);


	for (int z = 0; z < 100; z++) {
		read_write_cycle();
	}

	printf("Done\n");

	return 0;
}
