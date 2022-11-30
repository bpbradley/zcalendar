# zcalendar

Calendar API for Zephyr RTOS.

## Features

* Ability to set and receive calendar time through a generic API where the backend is hidden from the user level. If a battery backed RTC is used, the calendar time will be retained through power cycles.

* Supports access from user threads via system calls so it operates the same with or without `CONFIG_USERSPACE=y`

## Supported Backends

* STM32 RTC
* Maxim DS3231
* Micro Crystal RV8263

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

Some backends need specific device tree configurations. This is something I would like to address in the future to make configuration clearer, but for now examples for configuration of each backend are included.

#### Maxim DS3231

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

#### Micro Crystal RV8263

The Micro Crystal RV8263 is independent of all zephyr drivers and APIs, and so includes its own device tree binding.

```dts
&i2c0 {
  calendar_rtc: rv8263@51 {
    compatible = "microcrystal,rv8263-calendar";
    reg = <0x51>;
    label = "RV8263";
  };
};
```

#### STM32

The STM32 implementation is independent of the counter API and does not rely on other devices like i2c, so it does not need any device tree configuration.

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
      revision: main
      import:
        path-prefix: rtos
    - name: zcalendar
      remote: zcal
      revision: zephyr-v2.7.3
      path: modules/zcalendar
  self:
    path: app
```

## Sample Usage

Here is everthing needed for basic usage of the calendar API
with the Micro Crystal RV8263 backend

### Sample Configuration

```conf
CONFIG_CALENDAR=y
CONFIG_RV8263_RTC_CALENDAR=y
```

```dts
&i2c0 {
  calendar_rtc: rv8263@51 {
    compatible = "microcrystal,rv8263-calendar";
    reg = <0x51>;
    label = "RV8263";
  };
};
```

### Sample Code

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
            const time_t epoch = 1669843527;
            time = gmtime(&epoch);
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

### Note about User Mode

If the application is configured in User Mode, where
the api must interact with the kernel only via system calls,
there will be additional configuration required since from from
zephyrs perspective, the application is trying to make system calls.

In this case, the following configuration is needed

```conf
CONFIG_APPLICATION_DEFINED_SYSCALL=y
```

and the `SYSCALL_INCLUDE_DIRS` environment variable needs to include
the path to zcalendar. This can be done in `CMakeLists.txt` like so, *before the boilerplate is loaded*

```txt
list (APPEND SYSCALL_INCLUDE_DIRS 
    path/to/zcalendar
)
```
