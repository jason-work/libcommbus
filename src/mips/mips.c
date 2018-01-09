#include <stdlib.h>
#include <string.h>

#include "xpt_internal.h"
#include "mips/mediatek.h"

xpt_platform_t
xpt_mips_platform()
{
    xpt_platform_t platform_type = XPT_UNKNOWN_PLATFORM;
    size_t len = 100;
    char* line = malloc(len);
    FILE* fh = fopen("/proc/cpuinfo", "r");
    if (fh != NULL) {
        while (getline(&line, &len, fh) != -1) {
            if (strncmp(line, "machine", 7) == 0) {
                if (strstr(line, "MediaTek LinkIt Smart 7688")) {
                    platform_type = XPT_MTK_LINKIT;
                }
                if (strstr(line, "Onion Omega2")) {
                    platform_type = XPT_MTK_OMEGA2;
                }
            }
        }
        fclose(fh);
    }
    free(line);

    switch (platform_type) {
        case XPT_MTK_LINKIT:
            plat = xpt_mtk_linkit();
            break;
        case XPT_MTK_OMEGA2:
            plat = xpt_mtk_omega2();
            break;
        default:
            plat = NULL;
            syslog(LOG_ERR, "Unknown Platform, currently not supported by XPT");
    }
    return platform_type;
}
