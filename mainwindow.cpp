#include "mainwindow.h"
#include <QRandomGenerator>
#include <QWidget>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QThread>

// Include Raspberry Pi specific headers only if on Raspberry Pi
#ifdef RASPBERRYPI
    #include <wiringPi.h>
    #include <wiringPiI2C.h>
    #include <ads1115.h>
    #define PINBASE 120
    #define ADS_ADDR 0x48
#endif

// ADS1115 Constants
#define VOLT_RESOLUTION 32767.0f    // 15-bit resolution for ADS1115 (16-bit signed)
#define ADS1115_VOLTAGE_RANGE 4.096f // Using ±4.096V range (gain of 1)
#define SENSOR_VCC 5.0f            // MQ3 sensor powered by 5V

// MQ3 Sensor Constants
#define READ_SAMPLES 100           // Number of samples for averaging
#define CLEAN_AIR_FACTOR 70.0f     // The ratio of RS/R0 is 70 in clean air
#define MEASUREMENT_INTERVAL 1000   // 1 second between measurements
#define WARMUP_TIME 3              // 3 second warmup

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , isMeasuring(false)
    , warmupCount(WARMUP_TIME)
    , R0(0.0f)
{
    // Set window properties
    setWindowTitle("Alcohol Meter");
    setMinimumSize(800, 480);
    setStyleSheet("background-color: #f0f0f0;");

    // Create central widget and main layout
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    setCentralWidget(centralWidget);

    // Create and setup title label
    titleLabel = new QLabel("Alcohol Meter", this);
    titleLabel->setStyleSheet("QLabel { font-size: 48px; font-weight: bold; }");
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);
    mainLayout->addSpacing(20);

    // Create and setup calibration label
    calibrationLabel = new QLabel("R0: Not Calibrated", this);
    calibrationLabel->setStyleSheet("QLabel { font-size: 24px; }");
    calibrationLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(calibrationLabel);
    mainLayout->addSpacing(20);

    // Create and setup measurement label
    measurementLabel = new QLabel("0.000 mg/L", this);
    measurementLabel->setStyleSheet("QLabel { font-size: 72px; font-weight: bold; color: green; }");
    measurementLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(measurementLabel);
    mainLayout->addSpacing(20);

    // Create and setup status label
    statusLabel = new QLabel("Status: Ready", this);
    statusLabel->setStyleSheet("QLabel { font-size: 24px; }");
    statusLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(statusLabel);
    mainLayout->addSpacing(40);

    // Create horizontal layout for buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    mainLayout->addLayout(buttonLayout);

    // Create and setup start/stop button
    startStopButton = new QPushButton("Start", this);
    startStopButton->setStyleSheet(
        "QPushButton { background-color: #4CAF50; color: white; border-radius: 10px; "
        "            padding: 20px; min-width: 200px; font-size: 24px; }"
        "QPushButton:hover { background-color: #45a049; }"
        "QPushButton:pressed { background-color: #398439; }"
    );
    buttonLayout->addWidget(startStopButton);

    // Create and setup calibrate button
    calibrateButton = new QPushButton("Calibrate", this);
    calibrateButton->setStyleSheet(
        "QPushButton { background-color: #2196F3; color: white; border-radius: 10px; "
        "            padding: 20px; min-width: 200px; font-size: 24px; }"
        "QPushButton:hover { background-color: #1976D2; }"
        "QPushButton:pressed { background-color: #1565C0; }"
    );
    buttonLayout->addWidget(calibrateButton);

    // Create and setup exit button
    exitButton = new QPushButton("Exit", this);
    exitButton->setStyleSheet(
        "QPushButton { background-color: #f44336; color: white; border-radius: 10px; "
        "            padding: 20px; min-width: 200px; font-size: 24px; }"
        "QPushButton:hover { background-color: #da190b; }"
        "QPushButton:pressed { background-color: #d32f2f; }"
    );
    buttonLayout->addWidget(exitButton);

    // Set up margins for the main layout
    mainLayout->setContentsMargins(40, 40, 40, 40);

    // Initialize measurement timer
    measurementTimer = new QTimer(this);
    measurementTimer->setInterval(MEASUREMENT_INTERVAL);

    // Initialize warmup timer
    warmupTimer = new QTimer(this);
    warmupTimer->setInterval(1000);

    // Connect signals and slots
    connect(startStopButton, &QPushButton::clicked, this, &MainWindow::toggleMeasurement);
    connect(calibrateButton, &QPushButton::clicked, this, &MainWindow::recalibrate);
    connect(exitButton, &QPushButton::clicked, this, &MainWindow::close);
    connect(measurementTimer, &QTimer::timeout, this, &MainWindow::updateMeasurement);
    connect(warmupTimer, &QTimer::timeout, this, &MainWindow::updateWarmup);

    #ifdef RASPBERRYPI
        // Initialize GPIO and ADC
        int ioerr = wiringPiSetupGpio();
        if (ioerr == -1) {
            QMessageBox::critical(this, "Error", "Failed to initialize GPIO!\nPlease check your permissions and hardware connection.");
            QTimer::singleShot(0, this, &QMainWindow::close);
            return;
        }
        ads1115Setup(PINBASE, ADS_ADDR);

        // Initial calibration
        R0 = calibrateSensor();
        calibrationLabel->setText(QString("R0: %1").arg(R0, 0, 'f', 3));
    #else
        R0 = 0.18f;  // Default value for testing
        calibrationLabel->setText(QString("R0: %1 (Test Mode)").arg(R0, 0, 'f', 3));
    #endif
}

MainWindow::~MainWindow()
{
}

int MainWindow::readADC()
{
    #ifdef RASPBERRYPI
        int rawValue = analogRead(PINBASE);
        return (rawValue < 0) ? 0 : rawValue;  // Prevent negative readings
    #else
        return QRandomGenerator::global()->bounded(100, 32767);
    #endif
}

float MainWindow::calibrateSensor()
{
    float sensorValue = 0;
    statusLabel->setText("Status: Calibrating...");

    // Get average reading
    for(int x = 0; x < READ_SAMPLES; x++) {
        sensorValue += readADC();
        QThread::msleep(10);  // Short delay between readings
    }
    sensorValue = sensorValue / READ_SAMPLES;

    // Calculate sensor voltage using ADS1115 voltage range
    float sensor_volt = (sensorValue / VOLT_RESOLUTION) * ADS1115_VOLTAGE_RANGE;

    // Calculate RS_air using sensor VCC
    float RS_air = (SENSOR_VCC - sensor_volt) / sensor_volt;

    // Calculate R0
    float R0 = RS_air / CLEAN_AIR_FACTOR;

    qDebug() << "Calibration Results:";
    qDebug() << "Average ADC Value:" << sensorValue;
    qDebug() << "Sensor Voltage:" << sensor_volt << "V";
    qDebug() << "RS_air:" << RS_air;
    qDebug() << "R0:" << R0;

    statusLabel->setText("Status: Ready");
    return R0;
}

void MainWindow::recalibrate()
{
    if (!isMeasuring) {
        R0 = calibrateSensor();
        calibrationLabel->setText(QString("R0: %1").arg(R0, 0, 'f', 3));
        QMessageBox::information(this, "Calibration", "Sensor calibration completed.\nNew R0 value: " + QString::number(R0, 'f', 3));
    } else {
        QMessageBox::warning(this, "Calibration", "Please stop measurements before calibrating.");
    }
}

void MainWindow::toggleMeasurement()
{
    isMeasuring = !isMeasuring;

    if (isMeasuring) {
        warmupCount = WARMUP_TIME;
        startStopButton->setText("Stop");
        startStopButton->setStyleSheet(
            "QPushButton { background-color: #ff9800; color: white; border-radius: 10px; "
            "            padding: 20px; min-width: 200px; font-size: 24px; }"
            "QPushButton:hover { background-color: #f57c00; }"
            "QPushButton:pressed { background-color: #ef6c00; }"
        );
        statusLabel->setText("Status: Warming up... " + QString::number(warmupCount) + "s");
        warmupTimer->start();
        startStopButton->setEnabled(false);
        calibrateButton->setEnabled(false);
    } else {
        warmupTimer->stop();
        measurementTimer->stop();
        startStopButton->setText("Start");
        startStopButton->setEnabled(true);
        calibrateButton->setEnabled(true);
        startStopButton->setStyleSheet(
            "QPushButton { background-color: #4CAF50; color: white; border-radius: 10px; "
            "            padding: 20px; min-width: 200px; font-size: 24px; }"
            "QPushButton:hover { background-color: #45a049; }"
            "QPushButton:pressed { background-color: #398439; }"
        );
        statusLabel->setText("Status: Ready");
        measurementLabel->setText("0.000 mg/L");
        measurementLabel->setStyleSheet("QLabel { font-size: 72px; font-weight: bold; color: green; }");
    }
}

void MainWindow::updateWarmup()
{
    warmupCount--;
    if (warmupCount > 0) {
        statusLabel->setText("Status: Warming up... " + QString::number(warmupCount) + "s");
    } else {
        warmupTimer->stop();
        measurementTimer->start();
        statusLabel->setText("Status: Measuring...");
        startStopButton->setEnabled(true);
    }
}

void MainWindow::updateMeasurement()
{
    float sensorValue = 0;

    // Get average reading
    for(int x = 0; x < READ_SAMPLES; x++) {
        sensorValue += readADC();
        QThread::msleep(2);  // Short delay between readings
    }
    sensorValue = sensorValue / READ_SAMPLES;

    // Calculate sensor voltage using ADS1115 voltage range
    float sensor_volt = (sensorValue / VOLT_RESOLUTION) * ADS1115_VOLTAGE_RANGE;

    // Calculate RS using sensor VCC
    float RS = (SENSOR_VCC - sensor_volt) / sensor_volt;

    // Calculate ratio RS/R0
    float rs_ro_ratio = RS / R0;

    // Convert to mg/L based on datasheet curve
    float bac;
    if (rs_ro_ratio > 20.0f) {
        bac = 0.1f * (20.0f / rs_ro_ratio);
    } else if (rs_ro_ratio < 3.0f) {
        bac = 1.0f * (3.0f / rs_ro_ratio);
    } else {
        // Linear interpolation between 0.1 and 1.0 mg/L
        bac = 0.1f + (20.0f - rs_ro_ratio) * (0.9f / 17.0f);
    }

    // Debug output
    qDebug() << "\nMeasurement Results:";
    qDebug() << "Raw ADC Value:" << sensorValue;
    qDebug() << "Sensor Voltage:" << sensor_volt << "V";
    qDebug() << "RS:" << RS;
    qDebug() << "RS/R0 ratio:" << rs_ro_ratio;
    qDebug() << "BAC:" << bac << "mg/L";

    // Update display
    measurementLabel->setText(QString::number(bac, 'f', 3) + " mg/L");

    // Update color based on BAC levels
    if (bac < 0.3) {
        measurementLabel->setStyleSheet("QLabel { font-size: 72px; font-weight: bold; color: green; }");
    } else if (bac < 0.5) {
        measurementLabel->setStyleSheet("QLabel { font-size: 72px; font-weight: bold; color: orange; }");
    } else {
        measurementLabel->setStyleSheet("QLabel { font-size: 72px; font-weight: bold; color: red; }");
    }
}
