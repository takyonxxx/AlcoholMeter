#include "mainwindow.h"
#include <QRandomGenerator>
#include <QWidget>
#include <QHBoxLayout>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle("Alcohol Meter");
    setStyleSheet("background-color: #f0f0f0;");

#if defined(Q_OS_ANDROID)
    requestAndroidPermissions();
#endif

    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    setCentralWidget(centralWidget);
    mainLayout->setSpacing(20);
    mainLayout->setContentsMargins(40, 40, 40, 40);

    titleLabel = new QLabel("Alcohol Meter", this);
    titleLabel->setStyleSheet("QLabel { font-size: 48px; font-weight: bold; }");
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    calibrationLabel = new QLabel("R0: Not Calibrated", this);
    calibrationLabel->setStyleSheet("QLabel { font-size: 24px; }");
    calibrationLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(calibrationLabel);

    measurementLabel = new QLabel("0.00 mg/L", this);
    measurementLabel->setStyleSheet("QLabel { font-size: 72px; font-weight: bold; color: green; }");
    measurementLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(measurementLabel);

    statusLabel = new QLabel("Status: Ready", this);
    statusLabel->setStyleSheet("QLabel { font-size: 24px; }");
    statusLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(statusLabel);

    startStopButton = new QPushButton("Start", this);
    startStopButton->setStyleSheet(
        "QPushButton { background-color: #4CAF50; color: white; border-radius: 10px; "
        "padding: 20px; min-width: 200px; font-size: 24px; }"
        "QPushButton:hover { background-color: #45a049; }"
        "QPushButton:pressed { background-color: #398439; }");
    mainLayout->addWidget(startStopButton);

    calibrateButton = new QPushButton("Calibrate", this);
    calibrateButton->setStyleSheet(
        "QPushButton { background-color: #2196F3; color: white; border-radius: 10px; "
        "padding: 20px; min-width: 200px; font-size: 24px; }"
        "QPushButton:hover { background-color: #1976D2; }"
        "QPushButton:pressed { background-color: #1565C0; }");
    mainLayout->addWidget(calibrateButton);

    exitButton = new QPushButton("Exit", this);
    exitButton->setStyleSheet(
        "QPushButton { background-color: #f44336; color: white; border-radius: 10px; "
        "padding: 20px; min-width: 200px; font-size: 24px; }"
        "QPushButton:hover { background-color: #da190b; }"
        "QPushButton:pressed { background-color: #d32f2f; }");
    mainLayout->addWidget(exitButton);

    connect(startStopButton, &QPushButton::clicked, this, &MainWindow::toggleMeasurement);
    connect(calibrateButton, &QPushButton::clicked, this, &MainWindow::recalibrate);
    connect(exitButton, &QPushButton::clicked, this, &MainWindow::close);
}

MainWindow::~MainWindow()
{
}

#if defined(Q_OS_ANDROID)
void MainWindow::requestAndroidPermissions()
{
    QJniObject activity = QJniObject::callStaticObjectMethod(
        "org/qtproject/qt/android/QtNative",
        "activity",
        "()Landroid/app/Activity;"
        );

    if (!activity.isValid())
        return;

    jint permission = activity.callMethod<jint>(
        "checkSelfPermission",
        "(Ljava/lang/String;)I",
        QJniObject::fromString("android.permission.BLUETOOTH").object()
        );

    if (permission != 0) { // PERMISSION_GRANTED = 0
        QJniObject::callStaticMethod<void>(
            "org/tbiliyor/alcoholmeter/MainActivity",
            "requestPermission",
            "(Ljava/lang/String;)V",
            QJniObject::fromString("android.permission.BLUETOOTH").object()
            );
    }
}
#endif

void MainWindow::recalibrate()
{
    if (!isMeasuring) {
        QMessageBox::information(this, "Calibration", "Sensor calibration completed.\nNew R0 value: " + QString::number(0, 'f', 3));
    } else {
        QMessageBox::warning(this, "Calibration", "Please stop measurements before calibrating.");
    }
}

void MainWindow::toggleMeasurement()
{
    isMeasuring = !isMeasuring;

    if (isMeasuring) {
        startStopButton->setText("Stop");
        startStopButton->setStyleSheet(
            "QPushButton { background-color: #ff9800; color: white; border-radius: 10px; "
            "            padding: 20px; min-width: 200px; font-size: 24px; }"
            "QPushButton:hover { background-color: #f57c00; }"
            "QPushButton:pressed { background-color: #ef6c00; }"
            );
        statusLabel->setText("Status: Starting up... ");
        startStopButton->setEnabled(false);
        calibrateButton->setEnabled(false);
    } else {
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

void MainWindow::updateMeasurement(float bac)
{
    // Update display
    measurementLabel->setText(QString::number(bac, 'f', 2) + " mg/L");
    // Update color based on BAC levels
    if (bac < 0.3) {
        measurementLabel->setStyleSheet("QLabel { font-size: 72px; font-weight: bold; color: green; }");
    } else if (bac < 0.5) {
        measurementLabel->setStyleSheet("QLabel { font-size: 72px; font-weight: bold; color: orange; }");
    } else {
        measurementLabel->setStyleSheet("QLabel { font-size: 72px; font-weight: bold; color: red; }");
    }
}
