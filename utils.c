#include "utils.h"

#include <errno.h>  // errno
#include <string.h> // strerror
#include <unistd.h> // sysconf

int
get_open_max() {
    errno        = 0;
    int open_max = sysconf(_SC_OPEN_MAX);
    if (open_max == -1) {
        if (errno == 0) {
            err_log("OPEN_MAX is indeterminate");
        } else {
            err_log("can't get OPEN_MAX, sysconf failed: %s", strerror(errno));
        }
        return -1;
    }
    return open_max;
}
