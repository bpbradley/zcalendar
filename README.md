# zcalendar
Calendar API for Zephyr RTOS.

## Features

* Ability to set and receive calendar time through a generic API where the backend is hidden from the user level. If a battery backed RTC is used, the calendar time will be retained through power cycles.

* Supports access from user threads via system calls so it operates the same with or without `CONFIG_USERSPACE=y`

## Supported Backends

Currently, only battery backed STM32 RTC is available as a supported backend. It can be used without battery backup, but the calendar time will be reset whenever power is lost.

## Configuration

zcalendar is fully compatible with the west build system, and is designed to be easily integrated into any Zephyr project through the project's west manifest. It is configured through the project conf file.

### Kconfig

Enable with

```conf
CONFIG_CALENDAR=y
```

Enable the desired backend such as

```conf
CONFIG_STM32_RTC_CALENDAR=y
```

Refer to the Kconfig files to learn about optional configurations which can be used as well.

### Using West

Here is an example west manifest file. Modify according to your project needs

```yaml
manifest:
  remotes:
    - name: zephyrproject-rtos
      url-base: https://github.com/zephyrproject-rtos
    - name: zcal
      url-base: https://github.com/bpbradley
  projects:
    - name: zephyr
      remote: zephyrproject-rtos
      revision: master
      import:
        path-prefix: rtos
    - name: zcalendar
      remote: zcal
      revision: main
      path: modules/zcalendar
  self:
    path: app
```

## Example Usage

```c
#include <zephyr.h>
#include <zcal/calendar.h>

int main(){

    /* Get the device handle */
    const struct device * calendar = \
        device_get_binding(DT_LABEL(DT_INST(0, st_stm32_rtc)));

    if (calendar){
            /* Set the calendar time by epoch */
            struct tm * time;
            const time_t epoch = 946684800;
            time = localtime(&epoch);
            calendar_settime(calendar, time);

            /* Wait */
            k_sleep(K_SECONDS(5));

            /* Check the calendar time */
            int rc = calendar_gettime(calendar, time);

            /* Convert the time to a human readable string */
            if (!rc){
                char timestr[120] = {0};
                strftime(timestr,120,
                        "%A %B %d %Y %H:%M:%S", time);
                
                printk("%s", timestr);
            }
    }
    return 0;
}

```
