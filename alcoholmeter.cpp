// alcoholmeter.cpp
#include "alcoholmeter.h"
#include <QDebug>
#include <QThread>
#include <QRandomGenerator>

#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <ads1115.h>

#define PINBASE 120
#define ADS_ADDR 0x48

AlcoholMeter::AlcoholMeter(QObject *parent)
    : QObject(parent)
    , isMeasuring(false)
    , warmupCount(WARMUP_TIME)
{

    gattServer = GattServer::getInstance();
    if (gattServer)
    {
        qDebug() << "Starting gatt service";
        QObject::connect(gattServer, &GattServer::connectionState, this, &AlcoholMeter::onConnectionStatedChanged);
        QObject::connect(gattServer, &GattServer::dataReceived, this, &AlcoholMeter::onDataReceived);
        gattServer->startBleService();
    }

    // Initialize timers
    measurementTimer = new QTimer(this);
    measurementTimer->setInterval(MEASUREMENT_INTERVAL);

    warmupTimer = new QTimer(this);
    warmupTimer->setInterval(1000);

    // Connect timer signals
    connect(measurementTimer, &QTimer::timeout, this, &AlcoholMeter::updateMeasurement);
    connect(warmupTimer, &QTimer::timeout, this, &AlcoholMeter::updateWarmup);

    if (wiringPiSetupGpio() == -1) {
        qCritical() << "Failed to initialize GPIO! Check permissions and hardware connection.";
        return;
    }

    ads1115Setup(PINBASE, ADS_ADDR);
    pinMode(MQ3_POWER_PIN, OUTPUT);
    digitalWrite(MQ3_POWER_PIN, LOW);

    // Initial calibration
    //R0 = calibrateSensor();
    qDebug() << "Initial R0:" << R0;
}

AlcoholMeter::~AlcoholMeter()
{
    stopMeasurement();
    if (gattServer)
    {
        gattServer->stopBleService();
        delete gattServer;
    }
}

void AlcoholMeter::startMeasurement()
{
    if (!isMeasuring) {
        toggleMeasurement();
    }
}

void AlcoholMeter::stopMeasurement()
{
    if (isMeasuring) {
        toggleMeasurement();
    }
}

void AlcoholMeter::performCalibration()
{
    if (!isMeasuring) {
        R0 = calibrateSensor();
        qDebug() << "Calibration completed. New R0:" << R0;
    } else {
        qDebug() << "Please stop measurements before calibrating.";
    }
}

float AlcoholMeter::getCurrentR0() const
{
    return R0;
}

int AlcoholMeter::readADC(int addr)
{
    int rawValue = analogRead(PINBASE + addr);
    return (rawValue < 0) ? 0 : rawValue;  // Prevent negative readings
}

float AlcoholMeter::calibrateSensor()
{
    qDebug() << "Calibrating sensor...";
    float sensorValue = 0;

    // Get average reading
    for(int x = 0; x < READ_SAMPLES; x++) {
        sensorValue += readADC(0);
        QThread::msleep(10);
    }
    sensorValue = sensorValue / READ_SAMPLES;

    float sensor_volt = (sensorValue / VOLT_RESOLUTION) * ADS1115_VOLTAGE_RANGE;

    float RS_air = 0.0f;
    float R0 = 0.18f;
    if(sensor_volt > 0)
    {
        RS_air = (SENSOR_VCC - sensor_volt) / sensor_volt;
        R0 = RS_air / CLEAN_AIR_FACTOR;
    }

    qDebug() << "Calibration Results:";
    qDebug() << "Average ADC Value:" << sensorValue;
    qDebug() << "Sensor Voltage:" << sensor_volt << "V";
    qDebug() << "RS_air:" << RS_air;
    qDebug() << "R0:" << R0;

    return R0;
}

void AlcoholMeter::safePowerUp() {
    setPinHigh(MQ3_POWER_PIN);
}

void AlcoholMeter::safePowerDown() {
    setPinLow(MQ3_POWER_PIN);
}
void AlcoholMeter::toggleMeasurement()
{
    isMeasuring = !isMeasuring;

    if (isMeasuring) {
        safePowerUp();
        warmupCount = WARMUP_TIME;
        qDebug() << "Starting measurement...";
        qDebug() << "Warming up..." << warmupCount << "s";
        warmupTimer->start();
    } else {
        warmupTimer->stop();
        measurementTimer->stop();
        safePowerDown();
        qDebug() << "Measurement stopped.";
    }
}

void AlcoholMeter::updateWarmup()
{
    warmupCount--;
    if (warmupCount > 0) {
        qDebug() << "Warming up..." << warmupCount << "s";
    } else {
        warmupTimer->stop();
        measurementTimer->start();
        qDebug() << "Warmup complete. Starting measurements...";
    }
}

void AlcoholMeter::updateMeasurement()
{
    float sensorValue = 0;

    p_end = QDateTime::currentDateTime();
    qint64 elapsedTimeMillis = p_start.msecsTo(p_end);

    // Get average reading
    for(int x = 0; x < READ_SAMPLES; x++) {
        sensorValue += readADC(0);
        QThread::msleep(2);
    }
    sensorValue = sensorValue / READ_SAMPLES;

    p_dt = elapsedTimeMillis / 1000.0;

    kalmanBac.Update(sensorValue, measurementVariance, p_dt);
    sensorValue = kalmanBac.GetXAbs();

    // Calculate sensor voltage
    float sensor_volt = (sensorValue / VOLT_RESOLUTION) * ADS1115_VOLTAGE_RANGE;

    // Calculate RS
    float RS = (SENSOR_VCC - sensor_volt) / sensor_volt;

    // Calculate ratio RS/R0
    float rs_ro_ratio = RS / R0;

    // Convert to mg/L based on datasheet curve

    if (rs_ro_ratio > 20.0f) {
        bac = 0.1f * (20.0f / rs_ro_ratio);
    } else if (rs_ro_ratio < 3.0f) {
        bac = 1.0f * (3.0f / rs_ro_ratio);
    } else {
        // Linear interpolation between 0.1 and 1.0 mg/L
        bac = 0.1f + (20.0f - rs_ro_ratio) * (0.9f / 17.0f);
    }

    sendData(mCalcVal0, bac);
    sendData(mAdc0, sensor_volt);

    // Output results
    qDebug() << "\nMeasurement Results:";
    qDebug() << "Raw ADC Value:" << sensorValue;
    qDebug() << "Sensor Voltage:" << sensor_volt << "V";
    qDebug() << "RS:" << RS;
    qDebug() << "RS/R0 ratio:" << rs_ro_ratio;
    qDebug() << "BAC:" << bac << "mg/L";

    // Emit the result for any connected slots
    emit measurementUpdated(bac);
    p_start = p_end;
}

void AlcoholMeter::sendData(uint8_t command, float value)
{
    QByteArray payload = Message::floatToBytes(value);
    // Create message
    QByteArray sendData = message.createMessage(command, mWrite, payload);

    // Verify message
    if (sendData.isEmpty()) {
        qWarning() << "Failed to create message for command:" << command;
        return;
    }
    gattServer->writeValue(sendData);
}

void AlcoholMeter::sendString(uint8_t command, QString value)
{
    QByteArray bytedata;
    bytedata = value.toLocal8Bit();
    QByteArray sendData = message.createMessage(command, mWrite, bytedata);
    gattServer->writeValue(sendData);
}

void AlcoholMeter::onConnectionStatedChanged(bool state)
{
    isConnected = state;
}

void AlcoholMeter::onDataReceived(QByteArray data)
{
    uint8_t parsedCommand;
    uint8_t rw;
    QByteArray parsedValue;
    auto parsed = message.parseMessage(&data, parsedCommand, parsedValue, rw);

    if(!parsed)return;
    float value = message.bytesToFloat(parsedValue);

    if(rw == mRead)
    {
        switch (parsedCommand)
        {
        case mCalcVal0:
        {
            adc0 = readADC(0);
            sendData(mCalcVal0, adc0);
            break;
        }
        case mCalcVal1:
        {
            adc1 = readADC(1);
            sendData(mCalcVal1, adc1);
            break;
        }
        case mCalcVal2:
        {
            adc2 = readADC(2);
            sendData(mCalcVal2, adc2);
            break;
        }
        case mCalcVal3:
        {
            adc3 = readADC(3);
            sendData(mCalcVal3, adc3);
            break;
        }
        case mR0:
        {
            sendData(mR0, R0);
            break;
        }
        default:
            break;
        }
    }
    else if(rw == mWrite)
    {
        switch (parsedCommand)
        {
        case mStart:
        {
            startMeasurement();
            qDebug() << QString("Start Measurement...");
            break;
        }
        case mStop:
        {
            stopMeasurement();
            qDebug() << QString("Stop Measurement...");
            break;
        }
        case mCalibrate:
        {
            calibrateSensor();
            qDebug() << QString("Calibrate Sensor...");
            break;
        }
        default:
            break;
        }
    }
}

void AlcoholMeter::setPinHigh(uint8_t pin) {
    digitalWrite(pin, HIGH);
    qDebug() << "Set GPIO" << pin << "HIGH";
}

void AlcoholMeter::setPinLow(uint8_t pin) {
    digitalWrite(pin, LOW);
    qDebug() << "Set GPIO" << pin << "LOW";
}

bool AlcoholMeter::readPin(uint8_t pin) {
    return digitalRead(pin) == HIGH;
}
