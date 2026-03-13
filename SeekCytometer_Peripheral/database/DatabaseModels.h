#ifndef DATABASEMODELS_H
#define DATABASEMODELS_H

#include <QString>

struct ChipPosition {
    int id = -1;
    QString name;
    int xPosition = 0;
    int yPosition = 0;
};

struct LensPosition {
    int id = -1;
    QString name;
    int zPosition = 0;
};

struct LaserConfig {
    int id = -1;
    QString name;
    bool laser638nmEnable = false;
    bool laser448nmEnable = false;
    bool whiteLedEnable = false;
    int laser638nmPower = 0;
    int laser448nmPower = 0;
    int whiteLedPower = 0;
};

struct HydraulicControlParams {
    int id = -1;
    QString name;
    float ch1Kp = 2.0f;
    float ch1Ki = 1.0f;
    int ch1Feedforward = 13000;
    float ch2Kp = 2.0f;
    float ch2Ki = 1.0f;
    int ch2Feedforward = 13000;
    float ch3Kp = 2.0f;
    float ch3Ki = 1.0f;
    int ch3Feedforward = 13000;
    float ch4Kp = 2.0f;
    float ch4Ki = 1.0f;
    int ch4Feedforward = 13000;
    float ch5Kp = 2.0f;
    float ch5Ki = 1.0f;
    int ch5Feedforward = 13000;
};

#endif // DATABASEMODELS_H
