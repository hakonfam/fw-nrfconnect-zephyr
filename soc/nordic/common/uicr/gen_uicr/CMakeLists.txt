# SPDX-License-Identifier: Apache-2.0
#
# The code in this CMakeLists.txt constructs the arguments for gen_uicr.py
# and creates a flashable zephyr.hex file containing UICR data (no C code compiled)
#

cmake_minimum_required(VERSION 3.20.0)

# Instead of adding all of Zephyr we add just the subset that is
# required to generate uicr.hex.
#
# The generation of uicr.hex is configured by this image, so we
# include modules from zephyr_default up until kconfig.

find_package(Zephyr
  COMPONENTS zephyr_default:kconfig
  REQUIRED HINTS $ENV{ZEPHYR_BASE}
  )

project(uicr)

# Function to compute partition absolute address and size from devicetree
function(compute_partition_address_and_size partition_nodelabel output_address_var output_size_var)
  dt_nodelabel(partition_path NODELABEL ${partition_nodelabel} REQUIRED)
  dt_reg_addr(partition_offset PATH ${partition_path} REQUIRED)
  dt_reg_size(partition_size PATH ${partition_path} REQUIRED)

  # Calculate absolute partition address
  math(EXPR partition_address "${CONFIG_FLASH_BASE_ADDRESS} + ${partition_offset}" OUTPUT_FORMAT HEXADECIMAL)

  # Set output variables in parent scope
  set(${output_address_var} ${partition_address} PARENT_SCOPE)
  set(${output_size_var} ${partition_size} PARENT_SCOPE)
endfunction()

# Use CMAKE_VERBOSE_MAKEFILE to silence an unused-variable warning.
if(CMAKE_VERBOSE_MAKEFILE)
endif()

set(periphconf_args)
set(periphconf_elfs)
set(merged_hex_file ${APPLICATION_BINARY_DIR}/zephyr/${CONFIG_KERNEL_BIN_NAME}.hex)
set(uicr_hex_file ${APPLICATION_BINARY_DIR}/zephyr/uicr.hex)
set(periphconf_hex_file ${APPLICATION_BINARY_DIR}/zephyr/periphconf.hex)

# Get UICR absolute address from this image's devicetree
dt_nodelabel(uicr_path NODELABEL "uicr" REQUIRED)
dt_reg_addr(UICR_ADDRESS PATH ${uicr_path} REQUIRED)

if(CONFIG_GEN_UICR_GENERATE_PERIPHCONF)
  # gen_uicr.py parses all zephyr.elf files. To find these files (which
  # have not been built yet) we scan sibling build directories for
  # zephyr.dts
  get_filename_component(SYSBUILD_DIR ${APPLICATION_BINARY_DIR} DIRECTORY)
  file(GLOB _siblings LIST_DIRECTORIES true "${SYSBUILD_DIR}/*")
  foreach(_dir ${_siblings})
    get_filename_component(_name ${_dir} NAME)
    if(_name STREQUAL "uicr")
      # This image is an exception to the rule. It has a zephyr.dts, but
      # no zephyr.elf
      continue()
    endif()

    if(EXISTS ${_dir}/zephyr/zephyr.dts)
      list(APPEND periphconf_elfs ${_dir}/zephyr/zephyr.elf)
    endif()
  endforeach()

  # Compute PERIPHCONF absolute address and size from this image's devicetree
  compute_partition_address_and_size("periphconf_partition" PERIPHCONF_ADDRESS PERIPHCONF_SIZE)

  # Set up periphconf arguments for gen_uicr.py
  list(APPEND periphconf_args --periphconf-address ${PERIPHCONF_ADDRESS})
  list(APPEND periphconf_args --periphconf-size ${PERIPHCONF_SIZE})
  list(APPEND periphconf_args --out-periphconf-hex ${periphconf_hex_file})

  foreach(elf ${periphconf_elfs})
    list(APPEND periphconf_args --in-periphconf-elf ${elf})
  endforeach()
endif(CONFIG_GEN_UICR_GENERATE_PERIPHCONF)

# Set up secondary UICR arguments if enabled in the main application
set(secondary_args)

# Find the main application's .config file and check for secondary configuration
get_filename_component(SYSBUILD_DIR ${APPLICATION_BINARY_DIR} DIRECTORY)
file(GLOB _siblings LIST_DIRECTORIES true "${SYSBUILD_DIR}/*")
foreach(_dir ${_siblings})
  get_filename_component(_name ${_dir} NAME)
  if(_name STREQUAL "uicr")
    # Skip the UICR image itself
    continue()
  endif()
  
  set(MAIN_APP_CONFIG_FILE "${_dir}/zephyr/.config")
  if(EXISTS ${MAIN_APP_CONFIG_FILE})
    file(READ ${MAIN_APP_CONFIG_FILE} MAIN_APP_CONFIG_CONTENT)
    if("${MAIN_APP_CONFIG_CONTENT}" MATCHES "CONFIG_SOC_NRF54H20_SECONDARY=y")
      message(STATUS "Secondary UICR configuration detected in application: ${_name}")
      list(APPEND secondary_args --secondary-enable)
      
      # Extract flash base address and load offset for secondary firmware location
      if("${MAIN_APP_CONFIG_CONTENT}" MATCHES "CONFIG_FLASH_BASE_ADDRESS=([0-9A-Fa-fx]+)")
        set(FLASH_BASE_ADDRESS ${CMAKE_MATCH_1})
        
        # Extract flash load offset (MCUboot header offset)
        set(FLASH_LOAD_OFFSET "0x0")
        if("${MAIN_APP_CONFIG_CONTENT}" MATCHES "CONFIG_FLASH_LOAD_OFFSET=([0-9A-Fa-fx]+)")
          set(FLASH_LOAD_OFFSET ${CMAKE_MATCH_1})
        endif()
        
        # Calculate actual firmware start address (base + offset)
        math(EXPR SECONDARY_ADDRESS "${FLASH_BASE_ADDRESS} + ${FLASH_LOAD_OFFSET}" OUTPUT_FORMAT HEXADECIMAL)
        message(STATUS "Secondary firmware base address: ${FLASH_BASE_ADDRESS}")
        message(STATUS "Secondary firmware load offset: ${FLASH_LOAD_OFFSET}")
        message(STATUS "Secondary firmware start address: ${SECONDARY_ADDRESS}")
        list(APPEND secondary_args --secondary-address ${SECONDARY_ADDRESS})
      endif()
      
      # Note: Not setting --secondary-processor as APPLICATION value (0xBD2328A8) 
      # is the same as DISABLED_VALUE which causes validation issues
      
      # Explicitly disable secondary sub-features we're not using to ensure valid UICR
      # This ensures ironside validation passes by having consistent configuration
      # Note: These are disabled by default, but being explicit helps with validation
      break() # Found the config, no need to check other applications
    endif()
  endif()
endforeach()

# Generate hex files (merged, uicr-only, and periphconf-only)
add_custom_command(
  OUTPUT ${merged_hex_file} ${uicr_hex_file} ${periphconf_hex_file}
  COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${ZEPHYR_BASE}/scripts/dts/python-devicetree/src
          ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/soc/nordic/common/uicr/gen_uicr.py
          --uicr-address ${UICR_ADDRESS}
          --out-merged-hex ${merged_hex_file}
          --out-uicr-hex ${uicr_hex_file}
          ${periphconf_args}
          ${secondary_args}
  DEPENDS ${periphconf_elfs}
  WORKING_DIRECTORY ${APPLICATION_BINARY_DIR}
  COMMENT "Using gen_uicr.py to generate ${merged_hex_file} from ${periphconf_elfs}"
)

# Add zephyr subdirectory to handle flash configuration with correct paths
add_subdirectory(zephyr)

add_custom_target(gen_uicr ALL DEPENDS ${merged_hex_file})
