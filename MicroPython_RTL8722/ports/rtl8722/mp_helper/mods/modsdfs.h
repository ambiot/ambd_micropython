#include "ff.h"
#include <fatfs_ext/inc/ff_driver.h>
#include <sdcard.h>
#include "flash_api.h"
#include <flash_fatfs.h>
#include "py/mpconfig.h"
#include "py/runtime.h"
#include "py/stream.h"

bool init_sd_fs(void);
