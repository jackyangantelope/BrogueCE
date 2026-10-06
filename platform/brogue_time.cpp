
#include <ctime>
#include "pico/rand.h"
#include "pico/time.h"

extern "C" time_t time(time_t *result)
{
    static time_t boot_epoch = 0;
    if (boot_epoch == 0)
        boot_epoch = static_cast<time_t>(1352700001u + (get_rand_32() & 0x3fffffffu));
    const time_t now = boot_epoch + static_cast<time_t>(time_us_64() / 1000000u);
    if (result) *result = now;
    return now;
}
