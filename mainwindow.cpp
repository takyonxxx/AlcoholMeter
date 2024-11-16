#include "mainwindow.h"
#include <QRandomGenerator>
#include <QWidget>
#include <QHBoxLayout>
#include <QMessageBox>

#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <ads1115.h>

#define  PINBASE  120    // choose  PINBASE  value arbitrarily
#define  ADS_ADDR 0x48

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , isMeasuring(false)
    , warmupCount(3)
{
    // Set window properties
    setWindowTitle("Alcohol Meter");
    setMinimumSize(800, 480); // Increased window size for bigger content
    setStyleSheet("background-color: #f0f0f0;");

    // Create central widget and main layout
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    setCentralWidget(centralWidget);

    // Create and setup title label
    titleLabel = new QLabel("Alcohol Meter", this);
    titleLabel->setStyleSheet("QLabel { font-size: 48px; font-weight: bold; }"); // Increased from 24px
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);
    mainLayout->addSpacing(40); // Increased spacing

    // Create and setup measurement label
    measurementLabel = new QLabel("0.00 mg/L", this);
    measurementLabel->setStyleSheet("QLabel { font-size: 72px; font-weight: bold; color: green; }"); // Increased from 36px
    measurementLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(measurementLabel);
    mainLayout->addSpacing(20); // Increased spacing

    // Create and setup status label
    statusLabel = new QLabel("Status: Ready", this);
    statusLabel->setStyleSheet("QLabel { font-size: 24px; }"); // Increased from 12px
    statusLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(statusLabel);
    mainLayout->addSpacing(40); // Increased spacing

    // Create horizontal layout for buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    mainLayout->addLayout(buttonLayout);

    // Create and setup start/stop button
    startStopButton = new QPushButton("Start", this);
    startStopButton->setStyleSheet(
        "QPushButton { background-color: #4CAF50; color: white; border-radius: 10px; " // Increased border-radius
        "            padding: 20px; min-width: 240px; font-size: 24px; }" // Increased padding, width, and font size
        "QPushButton:hover { background-color: #45a049; }"
        "QPushButton:pressed { background-color: #398439; }"
        );
    buttonLayout->addWidget(startStopButton);

    // Create and setup exit button
    exitButton = new QPushButton("Exit", this);
    exitButton->setStyleSheet(
        "QPushButton { background-color: #f44336; color: white; border-radius: 10px; " // Increased border-radius
        "            padding: 20px; min-width: 240px; font-size: 24px; }" // Increased padding, width, and font size
        "QPushButton:hover { background-color: #da190b; }"
        "QPushButton:pressed { background-color: #d32f2f; }"
        );
    buttonLayout->addWidget(exitButton);

    // Set up margins for the main layout
    mainLayout->setContentsMargins(40, 40, 40, 40); // Increased margins

    // Initialize measurement timer
    measurementTimer = new QTimer(this);
    measurementTimer->setInterval(1000);

    // Initialize warmup timer
    warmupTimer = new QTimer(this);
    warmupTimer->setInterval(1000);

    // Connect signals and slots
    connect(startStopButton, &QPushButton::clicked, this, &MainWindow::toggleMeasurement);
    connect(exitButton, &QPushButton::clicked, this, &MainWindow::close);
    connect(measurementTimer, &QTimer::timeout, this, &MainWindow::updateMeasurement);
    connect(warmupTimer, &QTimer::timeout, this, &MainWindow::updateWarmup);

    int ioerr = wiringPiSetupGpio();
    if (ioerr == -1) {
        QMessageBox::critical(this, "Error", "Failed to initialize GPIO!\nPlease check your permissions and hardware connection.");
        QTimer::singleShot(0, this, &QMainWindow::close); // Close app after showing error
        return;
    }

    ads1115Setup (PINBASE, ADS_ADDR);

}

MainWindow::~MainWindow()
{

}

void MainWindow::toggleMeasurement()
{
    isMeasuring = !isMeasuring;

    if (isMeasuring) {
        warmupCount = 3;
        startStopButton->setText("Stop");
        startStopButton->setStyleSheet(
            "QPushButton { background-color: #ff9800; color: white; border-radius: 10px; "
            "            padding: 20px; min-width: 240px; font-size: 24px; }"
            "QPushButton:hover { background-color: #f57c00; }"
            "QPushButton:pressed { background-color: #ef6c00; }"
            );
        statusLabel->setText("Status: Warming up... " + QString::number(warmupCount) + "s");
        warmupTimer->start();
        startStopButton->setEnabled(false);
    } else {
        warmupTimer->stop();
        measurementTimer->stop();
        startStopButton->setText("Start");
        startStopButton->setEnabled(true);
        startStopButton->setStyleSheet(
            "QPushButton { background-color: #4CAF50; color: white; border-radius: 10px; "
            "            padding: 20px; min-width: 240px; font-size: 24px; }"
            "QPushButton:hover { background-color: #45a049; }"
            "QPushButton:pressed { background-color: #398439; }"
            );
        statusLabel->setText("Status: Ready");
        measurementLabel->setText("0.00 mg/L");
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
    int randomValue = QRandomGenerator::global()->bounded(0, 801);
    // double value = randomValue / 1000.0;
    double value = analogRead(PINBASE+0);

    measurementLabel->setText(QString::number(value, 'f', 2) + " mg/L");

    if (value < 0.3) {
        measurementLabel->setStyleSheet("QLabel { font-size: 72px; font-weight: bold; color: green; }");
    } else if (value < 0.5) {
        measurementLabel->setStyleSheet("QLabel { font-size: 72px; font-weight: bold; color: orange; }");
    } else {
        measurementLabel->setStyleSheet("QLabel { font-size: 72px; font-weight: bold; color: red; }");
    }
}
