# zcalendar

Calendar API for Zephyr RTOS.

## Features

* Ability to set and receive calendar time through a generic API where the backend is hidden from the user level. If a battery backed RTC is used, the calendar time will be retained through power cycles.

* Supports access from user threads via system calls so it operates the same with or without `CONFIG_USERSPACE=y`

## Supported Backends

* STM32 RTC
* Maxim DS3231

## Configuration

zcalendar is fully compatible with the west build system, and is designed to be easily integrated into any Zephyr project through the project's west manifest. It is configured through the project conf file, and depending on the backend, it may also need to be configured with the device tree.

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

### Device Tree

The Maxim DS3231 backend leverages the existing driver implementation in the counter API. In this case,
you must select the appropriate device in the device tree for zcal to use internally. This can be done
using the included device tree bindings

```dts
&i2c0 {
  calendar_rtc: ds3231@68 {
    compatible = "maxim,ds3231";
    reg = <0x68>;
    label = "DS3231";
    isw-gpios = <&gpio0 0 (GPIO_PULL_UP | GPIO_ACTIVE_LOW)>;
  };
};

/ {
  calendar{
    compatible = "calendar";
    label = "CALENDAR";
    rtc = <&calendar_rtc>;
  };
};
```

Or by using a chosen node

```dts
/ {
  chosen{
    zcal,rtc = &calendar_rtc;
  }
}
```

The STM32 implementation is independent of the counter API and so does not need a handle to that device to work.
As a result, it does not need any device tree configuration.

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
        device_get_binding(DT_LABEL(DT_INST(0, calendar)));

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
