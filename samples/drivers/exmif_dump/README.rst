.. zephyr:code-sample:: exmif-dump
   :name: EXMIF Dump to GRAM

   Test external SPI flash via EXMIF controller (W25Q256FW).

Overview
********

Read a configurable amount of data (no larger than available GRAM) and dump it to GRAM.
Pass CONFIG_EXMIF_DUMP_SIZE to change the amount of read data.
Use the following command to get the data in intelhex.

.. code-block:: console

   nrfutil device x-read --direct --address 0x2f000000 --bytes 0x90000 --traits jlink --core Application | \
   tail -n +2 | sed 's/|.*|$//' | \
     awk '{addr=$1; gsub(/:/, "", addr); for(i=2; i<=NF; i++) {
       w=$i;
       # Reverse byte order (little-endian) and print each byte
       print substr(w,7,2); print substr(w,5,2); print substr(w,3,2); print substr(w,1,2)
     }}' | \
     xxd -r -p > output.bin && \
   objcopy -I binary -O ihex output.bin output.hex
