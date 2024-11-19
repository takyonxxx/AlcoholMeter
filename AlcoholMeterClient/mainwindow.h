#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QDebug>

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

private:

#if defined(Q_OS_ANDROID)
    void requestAndroidPermissions();
#endif

    void updateMeasurement(float bac);

    // UI Elements
    QLabel *titleLabel;
    QLabel *measurementLabel;
    QLabel *statusLabel;
    QLabel *calibrationLabel;  // New label to show R0 value
    QPushButton *startStopButton;
    QPushButton *calibrateButton;  // New button for manual calibration
    QPushButton *exitButton;
    bool isMeasuring{false};
};

#endif // MAINWINDOW_H
