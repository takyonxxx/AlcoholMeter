
#include "mainwindow.h"
#include <QRandomGenerator>
#include <QWidget>
#include <QHBoxLayout>
#include <QMessageBox>

#if defined(Q_OS_IOS)
extern "C" void requestIOSBluetoothPermissions();
#endif

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle("Alcohol Meter");
    setStyleSheet("background-color: #f0f0f0;");

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

    QWidget* measurementWidget = new QWidget(this);
    QVBoxLayout* measurementLayout = new QVBoxLayout(measurementWidget);
    measurementLayout->setSpacing(0);  // Remove spacing between labels
    measurementLayout->setAlignment(Qt::AlignCenter);

    // Value label
    valueLabel = new QLabel(QString::number(bac, 'f', 2), this);
    valueLabel->setStyleSheet("QLabel { font-size: 72px; font-weight: bold; color: green; }");
    valueLabel->setAlignment(Qt::AlignCenter);
    measurementLayout->addWidget(valueLabel);

    // Unit label
    unitLabel = new QLabel("mg/L", this);
    unitLabel->setStyleSheet("QLabel { font-size: 72px; font-weight: bold; color: green; }");
    unitLabel->setAlignment(Qt::AlignCenter);
    measurementLayout->addWidget(unitLabel);

    mainLayout->addWidget(measurementWidget);

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

#if defined(Q_OS_ANDROID)
    requestAndroidPermissions();
#endif

#if defined(Q_OS_IOS)
    requestIOSBluetoothPermissions();
#endif

    m_bleConnection = new BluetoothClient();

    connect(m_bleConnection, &BluetoothClient::statusChanged, this, &MainWindow::statusChanged);
    connect(m_bleConnection, &BluetoothClient::changedState,this, &MainWindow::changedState);

    m_bleConnection->startScan();
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

    jint sdk_version = QJniObject::getStaticField<jint>("android/os/Build$VERSION", "SDK_INT");

    if (sdk_version >= 31) {
        // Android 12+ permissions
        QStringList permissions = {"android.permission.BLUETOOTH_SCAN",
                                   "android.permission.BLUETOOTH_CONNECT"};

        for (const QString& perm : permissions) {
            jint permission = activity.callMethod<jint>(
                "checkSelfPermission",
                "(Ljava/lang/String;)I",
                QJniObject::fromString(perm).object()
                );

            if (permission != 0) {
                QJniObject::callStaticMethod<void>(
                    "org/tbiliyor/alcoholmeter/MainActivity",
                    "requestPermission",
                    "(Ljava/lang/String;)V",
                    QJniObject::fromString(perm).object()
                    );
            }
        }
    } else {
        // Pre-Android 12 permissions
        QStringList permissions = {"android.permission.BLUETOOTH",
                                   "android.permission.BLUETOOTH_ADMIN"};

        for (const QString& perm : permissions) {
            jint permission = activity.callMethod<jint>(
                "checkSelfPermission",
                "(Ljava/lang/String;)I",
                QJniObject::fromString(perm).object()
                );

            if (permission != 0) {
                QJniObject::callStaticMethod<void>(
                    "org/tbiliyor/alcoholmeter/MainActivity",
                    "requestPermission",
                    "(Ljava/lang/String;)V",
                    QJniObject::fromString(perm).object()
                    );
            }
        }
    }
}
#endif

void MainWindow::recalibrate()
{
    if(!m_bleConnection->isConnected())
    {
        QMessageBox::warning(this, "Connection Error", "Device is not connected.\nPlease connect to device first.");
        return;
    }

    if (!isMeasuring) {
        sendData(mCalibrate, 0);
    } else {
        QMessageBox::warning(this, "Calibration", "Please stop measurements before calibrating.");
    }
}

void MainWindow::toggleMeasurement()
{
    if(!m_bleConnection->isConnected())
    {
        QMessageBox::warning(this, "Connection Error", "Device is not connected.\nPlease connect to device first.");
        return;
    }

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
        //startStopButton->setEnabled(false);
        calibrateButton->setEnabled(false);
        sendData(mStart, 0);
        isMeasuring=true;
    } else {
        startStopButton->setText("Start");
        //startStopButton->setEnabled(true);
        calibrateButton->setEnabled(true);
        startStopButton->setStyleSheet(
            "QPushButton { background-color: #4CAF50; color: white; border-radius: 10px; "
            "            padding: 20px; min-width: 200px; font-size: 24px; }"
            "QPushButton:hover { background-color: #45a049; }"
            "QPushButton:pressed { background-color: #398439; }"
            );
        statusLabel->setText("Status: Ready");
        valueLabel->setText("0.00");
        valueLabel->setStyleSheet("QLabel { font-size: 72px; font-weight: bold; color: green; }");
        sendData(mStop, 0);
        isMeasuring=false;
    }
}

void MainWindow::updateMeasurement(float bac, float r0)
{
    valueLabel->setText(QString::number(bac, 'f', 2));
    if (bac < 0.3) {
        valueLabel->setStyleSheet("QLabel { font-size: 72px; font-weight: bold; color: green; }");
    } else if (bac < 0.5) {
        valueLabel->setStyleSheet("QLabel { font-size: 72px; font-weight: bold; color: orange; }");
    } else {
        valueLabel->setStyleSheet("QLabel { font-size: 72px; font-weight: bold; color: red; }");
    }
    calibrationLabel->setText(QString("R0: %1").arg(QString::number(R0, 'f', 2)));
}

void MainWindow::statusChanged(const QString &status)
{
    statusLabel->setText(status);
}

void MainWindow::requestData(uint8_t command)
{
    QByteArray payload;
    QByteArray sendData = message.createMessage(command, mRead, payload);

    // Verify message
    if (sendData.isEmpty()) {
        qWarning() << "Failed to create message for command:" << command;
        return;
    }
    m_bleConnection->writeData(sendData);
}

void MainWindow::sendData(uint8_t command, float value)
{
    QByteArray payload = Message::floatToBytes(value);
    // Create message
    QByteArray sendData = message.createMessage(command, mWrite, payload);

    // Verify message
    if (sendData.isEmpty()) {
        qWarning() << "Failed to create message for command:" << command;
        return;
    }
    m_bleConnection->writeData(sendData);
}

void MainWindow::changedState(BluetoothClient::bluetoothleState state){

    switch(state){

    case BluetoothClient::Scanning:
    {
        statusChanged("Status: Connecting");
        break;
    }
    case BluetoothClient::ScanFinished:
    {
        break;
    }

    case BluetoothClient::Connecting:
    {
        break;
    }
    case BluetoothClient::Connected:
    {
        statusLabel->setText("Status: Ready");
        connect(m_bleConnection, &BluetoothClient::newData, this, &MainWindow::dataHandler);
        break;
    }
    case BluetoothClient::DisConnected:
    {
        statusChanged("Status: Disconnected");
        break;
    }
    case BluetoothClient::ServiceFound:
    {
        break;
    }
    case BluetoothClient::AcquireData:
    {
        requestData(mR0);
        break;
    }
    case BluetoothClient::Error:
    {
        break;
    }
    default:
        //nothing for now
        break;
    }
}

void MainWindow::dataHandler(QByteArray data)
{
    uint8_t parsedCommand;
    uint8_t rw;
    QByteArray parsedValue;
    auto parsed = message.parseMessage(&data, parsedCommand, parsedValue, rw);

    if(!parsed)return;
    float value = message.bytesToFloat(parsedValue);

    if(rw == mWrite)
    {
        switch(parsedCommand)
        {
        case mR0:
        {
            R0 = value;
            break;
        }
        case mCalcVal0:
        {
            bac = value;
            break;
        }
        case mString:
        {
            QString strValue = QString::fromLocal8Bit(parsedValue).simplified();
            statusChanged(strValue);
            break;
        }

        default:
            //nothing for now
            break;
        }
        updateMeasurement(bac, R0);
    }
}
