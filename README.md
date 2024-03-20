<h1 align="center">FLEET Provisioning with CLAIM ON AWS IoT CORE - ESP32 SIDE</h1>
<p></p>

> AWS IoT device provisioning with Fleet provisioning templates, The hardware is ESP32 that built-in a WiFi and Bluetooth module.

## Prerequisites

- [Platform IO CLI](https://docs.platformio.org/en/latest/core/index.html)

## Configuration

```c++
// include/config.h
-------------------
// Edit WiFi SSID and Password.
#define WIFI_SSID "xxxxxxx"
#define WIFI_PASSWORD "xxxxxxxx"
// Change a AWS_IOT_ENDPOINT follow you organize.
#define AWS_IOT_ENDPOINT "xxxxxxx.iot.eu-west-2.amazonaws.com"

// certs directory
------------------
// ca.pem => `RootCA` from AWS.
// cert.pem => `Device certification` from claim certificate.
// private.pem => `Private key` from claim certificate.

```

## Instruction

```sh
make
```

Ensure to edit the `cert` folder to `certs`

You can use PlatformIO GUI in Visual Studio Code also.

First step you should run
`Build Filesystem Image`
then
`Upload Filesystem Image`
and the finally
`Upload and Monitor`
your device.
