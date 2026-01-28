#ifndef SAMPLEPARAWIDGET_H
#define SAMPLEPARAWIDGET_H

#include <QDockWidget>
#include <QObject>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>

class SampleParaWidget : public QDockWidget
{
    Q_OBJECT
public:
    SampleParaWidget(const QString &tilte, QWidget *parent = nullptr);
    static constexpr int SAMPLE_CH_NUM = 8;

private:
    void initDockWidget();

    void onGainSetChanged(int ch, int val);
    void onRefSetChanged(int ch, int val);

    QSpinBox *spinGains[SAMPLE_CH_NUM];
    QSpinBox *spinRefs[SAMPLE_CH_NUM];

};


#endif // SAMPLEPARAWIDGET_H
