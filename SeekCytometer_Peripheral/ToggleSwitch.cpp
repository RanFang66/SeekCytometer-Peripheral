#include "ToggleSwitch.h"
#include <QPainter>
#include <QMouseEvent>

ToggleSwitch::ToggleSwitch(QWidget *parent)
    : QAbstractButton(parent)
{
    setCheckable(true);
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::NoFocus); // 不需要键盘控制
    // 默认大小，可按需修改或在外部调用 setFixedSize()/setMinimumSize()
    setFixedSize(50, 30);
}

QSize ToggleSwitch::sizeHint() const
{
    return QSize(50, 30);
}

void ToggleSwitch::paintEvent(QPaintEvent * /*event*/)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const int w = width();
    const int h = height();

    QRectF bgRect(m_margin, m_margin, w - 2*m_margin, h - 2*m_margin);
    qreal radius = bgRect.height() / 2.0;

    // 直接根据 isChecked() 决定背景色与圆点位置（无动画）
    QColor bg = isChecked() ? m_onColor : m_offColor;

    p.setPen(Qt::NoPen);
    p.setBrush(bg);
    p.drawRoundedRect(bgRect, radius, radius);

    // 小圆点
    qreal thumbDiameter = bgRect.height();
    qreal minX = m_margin;
    qreal maxX = w - m_margin - thumbDiameter;
    qreal x = isChecked() ? maxX : minX;
    qreal y = m_margin;

    QRectF thumbRect(x, y, thumbDiameter, thumbDiameter);

    p.setBrush(m_thumbColor);
    p.setPen(QPen(QColor(200,200,200,120)));
    p.drawEllipse(thumbRect);
}

void ToggleSwitch::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // 直接切换 checked 状态并重绘；QAbstractButton 会发出 toggled(bool)
        setChecked(!isChecked());
        update();
    }
    // 不需要调用父类以避免触发额外行为
}

void ToggleSwitch::resizeEvent(QResizeEvent * /*event*/)
{
    // 当控件大小变化时，只需重绘，保持 checked 状态决定位置
    update();
}

