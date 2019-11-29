#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

if (CONFIG_HCI_RPMSG)
  message("HERE")

  zephyr_add_executable(hci_rpmsg require_build)
  if (${require_build})
    add_subdirectory(bluetooth/hci_rpmsg ${CMAKE_BINARY_DIR}/hci_rpmsg)
  endif ()

endif()
