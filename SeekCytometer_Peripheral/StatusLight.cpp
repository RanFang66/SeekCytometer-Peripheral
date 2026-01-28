#include "StatusLight.h"
#include <QPainter>
#include <QRadialGradient>

StatusLight::StatusLight(QWidget *parent)
    : QWidget(parent), m_state(State::IDLE), m_blinkOn(true), m_enBlink(false)
{
    setMinimumSize(30, 30);

    // 设置闪烁定时器
    m_blinkTimer.setInterval(1000); // 500ms切换一次
    connect(&m_blinkTimer, &QTimer::timeout, this, &StatusLight::onBlinkTimeout);
}

void StatusLight::setState(State state)
{
    if (m_state != state) {
        m_state = state;



        // 控制闪烁
        if (m_enBlink && (m_state == State::RUNNING || m_state == State::FAULT)) {
            m_blinkOn = true;
            m_blinkTimer.start();
        } else {
            m_blinkTimer.stop();
            m_blinkOn = true; // IDLE 常亮
        }


        update();
    }
}

void StatusLight::setBlinkEnable(bool en)
{
    m_enBlink = en;
    if (m_enBlink && (m_state == State::RUNNING || m_state == State::FAULT)) {
        m_blinkOn = true;
        m_blinkTimer.start();
    } else {
        m_blinkTimer.stop();
        m_blinkOn = true; // IDLE 常亮
    }
}

StatusLight::State StatusLight::state() const
{
    return m_state;
}

void StatusLight::onBlinkTimeout()
{
    m_blinkOn = !m_blinkOn;
    update();
}

void StatusLight::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    int side = qMin(width(), height());
    QRect rect((width() - side) / 2, (height() - side) / 2, side, side);

    QColor baseColor;
    switch (m_state) {
    case State::IDLE:
        baseColor = Qt::gray;
        break;
    case State::RUNNING:
        baseColor = m_blinkOn ? Qt::green : Qt::gray; // 闪烁
        break;
    case State::FAULT:
        baseColor = m_blinkOn ? Qt::red : Qt::gray;   // 闪烁
        break;
    }

    // 渐变发光
    QRadialGradient gradient(rect.center(), side / 2);
    gradient.setColorAt(0.0, baseColor.lighter(150));
    gradient.setColorAt(0.6, baseColor);
    gradient.setColorAt(1.0, Qt::gray);

    painter.setBrush(gradient);
    painter.setPen(QPen(Qt::black, 1));
    painter.drawEllipse(rect);

    // 高光效果
    int highlightSize = side / 3;
    QRect highlightRect(rect.topLeft().x() + side/6, rect.topLeft().y() + side/6,
                        highlightSize, highlightSize);
    QRadialGradient highlightGradient(highlightRect.center(), highlightSize/2);
    highlightGradient.setColorAt(0.0, QColor(255,255,255,180));
    highlightGradient.setColorAt(1.0, QColor(255,255,255,0));
    painter.setBrush(highlightGradient);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(highlightRect);
}
