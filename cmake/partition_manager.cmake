#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

if(FIRST_BOILERPLATE_EXECUTION)
  get_property(
    partition_manager_config_files
    GLOBAL PROPERTY
    PARTITION_MANAGER_CONFIG_FILES
    )
  get_property(
    included_hex_files
    GLOBAL PROPERTY
    INCLUDED_HEX_FILES)
  get_property(
    skipped_hex_file_configs
    GLOBAL PROPERTY
    SKIPPED_HEX_FILES_CONFIG)
  if(partition_manager_config_files)

    set(lol
      COMMAND
      ${PYTHON_EXECUTABLE}
      ${ZEPHYR_BASE}/scripts/partition_manager.py
      --input ${partition_manager_config_files}
      --app-pm-config-dir ${PROJECT_BINARY_DIR}/include/generated
      --included-hex-files ${included_hex_files}
      --skipped-hex-file-configs ${skipped_hex_file_configs}
      )
    print(lol)
    execute_process(
      lol)

    # Make Partition Manager configuration available in CMake
    import_kconfig(PM_ ${PROJECT_BINARY_DIR}/include/generated/pm.config)

    set_property(
      TARGET gen_expr
      PROPERTY MCUBOOT_SLOT_SIZE
      ${PM_MCUBOOT_PARTITIONS_PRIMARY_SIZE})
    set_property(
      TARGET gen_expr
      PROPERTY MCUBOOT_HEADER_SIZE
      ${PM_MCUBOOT_PAD_SIZE})
    set_property(
      TARGET gen_expr
      PROPERTY PARTITION_MANAGER_ENABLED
      1)
    if (PM_SPM_ADDRESS AND PM_MCUBOOT_ADDRESS)
      set(merged_to_sign_hex ${CMAKE_BINARY_DIR}/merged_to_sign.hex)
      add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/merged_to_sign.hex
        COMMAND
        ${PYTHON_EXECUTABLE}
        ${ZEPHYR_BASE}/scripts/mergehex.py
        -o ${merged_to_sign_hex}
        ${PM_SPM_BUILD_DIR}/zephyr.hex
        ${CMAKE_BINARY_DIR}/zephyr/${KERNEL_HEX_NAME}
        DEPENDS
        spm_kernel_elf
        kernel_elf
        )
      add_custom_target(merged_to_sign_target DEPENDS ${merged_to_sign_hex})
      set_property(
        TARGET gen_expr
        PROPERTY MCUBOOT_TO_SIGN
        ${merged_to_sign_hex})
      set_property(
        TARGET gen_expr
        PROPERTY MCUBOOT_TO_SIGN_DEPENDS
        merged_to_sign_target
        )
    endif()
  else()
    set_property(
      TARGET gen_expr
      PROPERTY PARTITION_MANAGER_ENABLED
      0)
  endif()
endif()
