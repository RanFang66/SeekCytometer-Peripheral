#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include "SampleChipWidget.h"
#include "SerialPortConfigWidget.h"
#include "MicroFluidicWidget.h"
#include "OpticsControlWidget.h"
#include "SampleParaWidget.h"
#include "PZTCtrlWidget.h"
#include "MFCParasSetWidget.h"
#include "TempControlWidget.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void handleResponse(uint8_t slave, uint8_t func, const QVector<uint16_t> &regs);
    void handleWriteAck(uint8_t slave, uint8_t func, uint16_t addr, uint16_t qtyOrVal);
    void handleError(const QString &err);
    void onNeedUpdateStatus();

private:
    void initDockWidgets();
    QTimer *statusUpdateTimer;
    int m_count;
    SampleChipWidget *sampleChipWidget;
    MicroFluidicWidget *microFluidicWidget;
    SerialPortConfigWidget *serialConfigWidget;
    OpticsControlWidget *opticsControlWidget;
    SampleParaWidget *sampleParaWidget;
    PZTCtrlWidget   *pztCtrlWidget;
    MFCParasSetWidget *mfcParasWidget;
    TempControlWidget *tempCtrlWidget;
};
#endif // MAINWINDOW_H
