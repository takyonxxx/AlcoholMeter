#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QDebug>

// Define RASPBERRYPI if compiling for Raspberry Pi
#if defined(__linux__) && defined(__arm__)
#define RASPBERRYPI
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
    void updateMeasurement();
    void updateWarmup();
    void recalibrate();  // New slot for manual recalibration

private:
    // Helper functions for MQ3 calculations
    float calibrateSensor();
    int readADC();  // New function to handle ADC reading

    // UI Elements
    QLabel *titleLabel;
    QLabel *measurementLabel;
    QLabel *statusLabel;
    QLabel *calibrationLabel;  // New label to show R0 value
    QPushButton *startStopButton;
    QPushButton *calibrateButton;  // New button for manual calibration
    QPushButton *exitButton;
    QTimer *measurementTimer;
    QTimer *warmupTimer;

    // State variables
    bool isMeasuring;
    int warmupCount;
    float R0;  // Store calibration value
};

#endif // MAINWINDOW_H
