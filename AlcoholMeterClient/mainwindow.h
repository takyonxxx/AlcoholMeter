#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QDebug>
#include "bluetoothclient.h"
#include "message.h"

#if defined(Q_OS_ANDROID)
#include <QJniObject>
#endif

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void toggleMeasurement();
    void recalibrate();
    void statusChanged(const QString &status);
    void changedState(BluetoothClient::bluetoothleState state);
    void dataHandler(QByteArray data);

private:

#if defined(Q_OS_ANDROID)
    void requestAndroidPermissions();
#endif

    void updateMeasurement(float bac, float r0);
    void requestData(uint8_t command);
    void sendData(uint8_t command, float value);

#if defined(Q_OS_IOS)
    void requestIOSPermissions();
#endif

    BluetoothClient *m_bleConnection{nullptr};
    Message message;

    float R0 = 0.18f;
    float bac = 0.0;

    // UI Elements
    QLabel *titleLabel;
    QLabel* valueLabel;
    QLabel* unitLabel;
    QLabel *statusLabel;
    QLabel *calibrationLabel;  // New label to show R0 value
    QPushButton *startStopButton;
    QPushButton *calibrateButton;  // New button for manual calibration
    QPushButton *exitButton;
    bool isMeasuring{false};
};

#endif // MAINWINDOW_H
