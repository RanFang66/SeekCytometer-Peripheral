#ifndef SERIALPORTCONFIGWIDGET_H
#define SERIALPORTCONFIGWIDGET_H

#include <QDockWidget>
#include <QObject>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
class SerialPortConfigWidget : public QDockWidget
{
    Q_OBJECT

public:
    SerialPortConfigWidget(const QString &tilte, QWidget *parent = nullptr);


signals:
    void commPortConnectedOk();
    void commPortDisconnected();

private slots:
    void onRefreshPortsClicked();
    void onConnectClicked();


private:
    void initDockWidget();
    void populatePortList();
    void populateBaudrateList();
    void setUiEnabledWhenConnected(bool connected);


    QComboBox *comboPort;
    QComboBox *comboBaudrate;
    QPushButton *btnRefreshPorts;
    QPushButton *btnConnect; // Connect / Disconnect
    QLabel *statusLabel;
};

#endif // SERIALPORTCONFIGWIDGET_H
