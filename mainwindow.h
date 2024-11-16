#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

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

private:
    QLabel *titleLabel;
    QLabel *measurementLabel;
    QLabel *statusLabel;
    QPushButton *startStopButton;
    QPushButton *exitButton;
    QTimer *measurementTimer;
    QTimer *warmupTimer;
    bool isMeasuring;
    int warmupCount;
};

#endif // MAINWINDOW_H
