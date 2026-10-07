# E2 sniffer firmware (ESP32-S3)

Passive 802.11 monitor: per-device bytes, frames, RSSI and direction in 100 ms bins. CSV output and commands use the ESP32-S3's native **USB Serial/JTAG** connection. The protocol and commands are described at the top of `main/sniffer.c`.

Builds successfully with **ESP-IDF 6.1.0** for target `esp32s3`. The generated application is about 0x9DC50 bytes, leaving 38% free in the 1 MB app partition.

## Upload and monitor on macOS

1. Connect the ESP32-S3 using a data-capable USB cable. Close any other serial monitor using the board's port.
2. In Terminal, find the ESP USB Serial/JTAG port:

   ```sh
   ls /dev/cu.*
   ```

   Use the port that appears with the ESP board connected. The native USB port is identified during flashing as `USB-Serial/JTAG`; your latest port is `/dev/cu.usbmodem1101`. Do not use Bluetooth or other device ports.
3. Enter the ESP-IDF environment and go to this firmware folder:

   ```sh
   source /Users/af/.espressif/tools/activate_idf_v6.1.sh
   cd "/Users/af/ITMO/5th_sem/Embeded_System/Project/SpyScan/experiments/E2_wifi_traffic/firmware"
   ```

4. Replace the example port below if yours is different. Set the target only the first time (it cleans and reconfigures the build):

   ```sh
   idf.py set-target esp32s3
   idf.py build
   idf.py -p /dev/cu.usbmodem1101 -b 115200 flash
   idf.py -p /dev/cu.usbmodem1101 -b 115200 monitor
   ```

   Flash uses a conservative baud rate. The native USB monitor does not use the UART baud setting; 115200 is fine. Expect a line containing `boot,spyscan-e2-sniffer` and then status and `B,…` CSV lines. To stop the monitor, press **Ctrl+]**.

If flashing cannot connect, hold **BOOT**, tap **RESET/EN**, release **BOOT**, then run the flash command again. If it connects but transfer fails, close all serial monitors, try another data cable and connect directly to the Mac (without a hub). Disconnect any external wiring from the board while flashing. If the board has a separate native USB connector, try that port for flashing; use the USB-to-UART COM port for this firmware's monitor.

## VS Code

Open this `firmware` folder in VS Code, select target **esp32s3** in the ESP-IDF extension, choose the native USB Serial/JTAG port from `ls /dev/cu.*`, then use **Build**, **Flash**, and **Monitor**. Set both flash and monitor baud to **115200**. PlatformIO is configured with these serial settings too.

## Quick manual test (serial monitor)

Type `ch 6` ⏎ → `I,…,channel,6`. Type `stat` ⏎ → the counters. Type `hop 500` ⏎ → it hops channels 1–13.

## PC side

```
pip install pyserial numpy pandas matplotlib
python tools/e2/e2_capture.py --port /dev/cu.usbserial-XXXX survey --seconds 40
```

Find the port on macOS with `ls /dev/cu.*`; the native USB Serial/JTAG port commonly appears as `cu.usbmodem*`.

**Test without hardware:**
```
python tools/e2/fake_device.py          # prints e.g. /dev/ttys012
python tools/e2/e2_capture.py --port /dev/ttys012 challenge --channel 6 --k 12 --t 2 --baseline 2
```
The fake device produces SYNTHETIC data, for testing the tools only.
