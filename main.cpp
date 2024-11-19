#include <QCoreApplication>
#include "alcoholmeter.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    AlcoholMeter meter;
    return a.exec();
}
