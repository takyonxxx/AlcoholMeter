#ifndef ALCOHOLMETER_H
#define ALCOHOLMETER_H

#include <QObject>
#include <QTimer>
#include "gattserver.h"
#include "kalmanfilter.h"
#include "message.h"

class AlcoholMeter : public QObject {
    Q_OBJECT

public:
    // Constants
    static constexpr float VOLT_RESOLUTION = 32767.0f;    // 15-bit resolution for ADS1115
    static constexpr float ADS1115_VOLTAGE_RANGE = 4.096f; // Using ±4.096V range
    static constexpr float SENSOR_VCC = 5.0f;             // MQ3 sensor powered by 5V
    static constexpr int READ_SAMPLES = 100;              // Number of samples for averaging
    static constexpr float CLEAN_AIR_FACTOR = 70.0f;      // RS/R0 ratio in clean air
    static constexpr int MEASUREMENT_INTERVAL = 1000;      // 1 second between measurements
    static constexpr int WARMUP_TIME = 5;                 // 5 second warmup

    static constexpr uint8_t MQ3_POWER_PIN     = 17;  // GPIO17 - Pin 11 - Control sensor power
    static constexpr uint8_t MQ3_STATUS_PIN    = 27;  // GPIO27 - Pin 13 - Get D0, Alcohol status

    explicit AlcoholMeter(QObject *parent = nullptr);
    ~AlcoholMeter();

    // Public interface methods
    void startMeasurement();
    void stopMeasurement();
    float getCurrentR0() const;

    void setPinHigh(uint8_t pin);
    void setPinLow(uint8_t pin);
    bool readPin(uint8_t pin);
    void safePowerUp();
    void safePowerDown();

signals:
    void measurementUpdated(float bac);

private slots:
    void updateWarmup();
    void updateMeasurement();
    void onConnectionStatedChanged(bool state);
    void onDataReceived(QByteArray data);

private:
    int readADC(int addr);
    float calibrateSensor();
    void toggleMeasurement();
    void sendData(uint8_t command, float value);
    void sendString(QString value);

    GattServer *gattServer{nullptr};

    bool isMeasuring;
    int warmupCount;
    bool isConnected = false;
    float R0 = 0.18f;
    float bac = 0.0;
    float adc0 = 0.0;
    float adc1 = 0.0;
    float adc2 = 0.0;
    float adc3 = 0.0;
    QTimer *measurementTimer;
    QTimer *warmupTimer;
    QTimer *adcTimer;          // New timer for ADC readings

    KalmanFilter kalmanBac{0.1};
    double measurementVariance = 0.5;
    double timeDelta = 0.1;
    qreal p_dt{0.0};
    QDateTime p_end;                        // End time for calculations
    QDateTime p_start;
    Message message;
};

#endif // ALCOHOLMETER_H
