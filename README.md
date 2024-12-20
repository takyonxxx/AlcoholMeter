# AlcoholMeter Project

A Raspberry Pi-based alcohol detection system utilizing Bluetooth Low Energy (BLE) for remote monitoring.

## Hardware Components
- Raspberry Pi
- ADS1115 ADC
- MQ-3 Alcohol Sensor
- Power Control Circuit

![Raspberry Pi ADS1115 Connection](raspberry_adc.jpg)
*Raspberry Pi and ADS1115 ADC connection diagram*

![MQ-3 ADC Connection](mq-3_adc.jpg)
*MQ-3 Alcohol sensor and ADC connection diagram*

![AlcoholMeter Pinout](pinout.png)
*AlcoholMeter circuit pinout diagram*

## Key Features
- Real-time alcohol concentration measurements (mg/L)
- BLE communication for remote monitoring
- Kalman filtering for stable readings
- Automatic sensor calibration
- Power management for sensor longevity

## Technical Details
- Uses WiringPi for GPIO control
- ADS1115 16-bit ADC for precise measurements
- Implements warm-up cycle for sensor stability
- Data smoothing via Kalman filter
- BLE GATT service for data transmission
- Supports remote start/stop/calibration commands

## Operation Flow
1. **Initialization**: Sets up GPIO, ADC, and BLE service
2. **Calibration**: Determines R0 reference value in clean air
3. **Measurement Cycle**:
   - Warm-up period (configurable duration)
   - Continuous ADC sampling
   - Voltage conversion and BAC calculation
   - Real-time data transmission via BLE

## Safety Features
- Controlled power cycling of sensor
- Error checking on ADC readings
- Automatic power-down on disconnection
- Status monitoring and reporting

## Service Installation
1. Create service file:
```bash
sudo nano /etc/systemd/system/alcoholmeter.service
```

2. Add configuration:
```ini
[Unit]
Description=AlcoholMeter Service
After=bluetooth.target network.target
Requires=bluetooth.target

[Service]
Type=simple
ExecStart=/home/pi/AlcoholMeter/AlcoholMeter
Restart=always
User=pi
Group=bluetooth
WorkingDirectory=/home/pi/AlcoholMeter
Environment=DISPLAY=:0
# Add necessary capabilities for Bluetooth
AmbientCapabilities=CAP_NET_ADMIN CAP_NET_RAW
SupplementaryGroups=bluetooth

[Install]
WantedBy=multi-user.target
```

3. Enable and start service:
```bash
sudo systemctl daemon-reload
sudo systemctl enable alcoholmeter.service
sudo systemctl start alcoholmeter.service
```

4. Check service status:
```bash
sudo systemctl status alcoholmeter.service
```

5. View service logs:
```bash
sudo journalctl -fu alcoholmeter.service
```

This project provides a professional-grade alcohol detection solution with remote monitoring capabilities, suitable for various applications requiring precise alcohol concentration measurements.
