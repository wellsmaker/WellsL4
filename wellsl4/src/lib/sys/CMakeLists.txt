# SPDX-License-Identifier: Apache-2.0

wellsl4_sources_if_kconfig(base64.c)

wellsl4_sources(
        crc.c
        dec.c
        fd.c
        hex.c
        mempool.c
        onoff.c
        printk.c
        rb.c
        string.c
  )

wellsl4_sources_ifdef(CONFIG_JSON_LIBRARY json.c)

wellsl4_sources_if_kconfig(ring_buffer.c)

wellsl4_sources_ifdef(CONFIG_ASSERT assert.c)

include_directories(
        ${WELLSL4_BASE}/inc/sys
)