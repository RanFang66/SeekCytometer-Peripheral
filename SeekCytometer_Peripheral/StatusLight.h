#ifndef STATUSLIGHT_H
#define STATUSLIGHT_H

#include <QWidget>
#include <QTimer>

class StatusLight : public QWidget
{
    Q_OBJECT
public:
    enum class State { IDLE, RUNNING, FAULT };

    explicit StatusLight(QWidget *parent = nullptr);

    void setState(State state);
    void setBlinkEnable(bool en);
    State state() const;

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onBlinkTimeout();

private:
    State m_state;
    bool m_blinkOn;         // 当前闪烁可见性
    QTimer m_blinkTimer;    // 控制闪烁
    bool m_enBlink;
};

#endif // STATUSLIGHT_H
