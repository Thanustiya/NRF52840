# 📡 Fanstel BLG840X – HTTP Telemetry via nRF9160 (SLM Firmware)

This project sends sensor data from the nRF52840 to the nRF9160, which uploads it to an HTTP server using POST requests. The nRF9160 runs Nordic's Serial LTE Modem (SLM) firmware and uses AT commands via UART.

---

## ✅ What You Need

| 🔌 Device | [Fanstel BLG840X](https://www.fanstel.com/blg840x) (nRF52840 + nRF9160) |
| 💾 Software | [nRF Connect for Desktop](https://www.nordicsemi.com/Products/Development-tools/nrf-connect-for-desktop) |
| 🧠 IDE | [Segger Embedded Studio](https://www.segger.com/products/development-tools/embedded-studio/) |
| 📡 Firmware | Nordic’s [Serial LTE Modem (SLM)](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/applications/serial_lte_modem/README.html) |
| 📲 SIM | Active SIM with mobile data plan |
| 🌐 API | Your HTTP(S) endpoint for data upload |


## 🔧 How to Upload Firmware
Step 1: Flash SLM Firmware to nRF9160

1. Open nRF Connect for Desktop > Launch Programmer.
2. Connect the device using USB and select the nRF9160 target.
3. Load the precompiled SLM firmware `.hex` file.
4. Click Write to flash it.


### Step 2: Upload UART App to nRF52840

1. Open Segger Embedded Studio (SES).
2. Load the `Main.c` file from this repo.
3. Connect the device and choose the `nRF52840` target.
4. Press Build & Run.

📌 This code handles UART communication to send AT commands to the nRF9160.


## 💻 How It Works

- The nRF52840 sends AT commands to nRF9160 via UART.
- The nRF9160 (running SLM) uses LTE to send HTTP POST requests.
- You can modify the code to send real sensor data instead of fixed values.

---

## 🛠️ Sending HTTP Telemetry (AT Commands)

Here's how to manually send data using a UART terminal:

```bash
AT+CEREG?                                          # Check network registration
AT#XHTTP="POST","https://your-api.com/data"       # Set HTTP POST URL
AT#XHTTPHEAD="Content-Type: application/json"     # Set content-type header
AT#XHTTPDATA=37                                    # Declare data length
{"temperature":25.5,"humidity":60}                # Actual data payload
AT#XHTTPEXEC                                       # Execute POST request
