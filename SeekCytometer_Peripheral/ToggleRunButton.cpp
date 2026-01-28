#include "ToggleRunButton.h"
#include <QPainter>
#include <QMouseEvent>
#include <cmath>

ToggleRunButton::ToggleRunButton(QWidget *parent)
    : QAbstractButton(parent)
{
    setCheckable(true);
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::NoFocus); // 不需要键盘控制
    setFixedSize(120, 60);       // 默认尺寸，可在外部调整
}

QSize ToggleRunButton::sizeHint() const
{
    return QSize(120, 60);
}

void ToggleRunButton::paintEvent(QPaintEvent * /*event*/)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRectF fullRect = rect();
    // 背景矩形区域（考虑边距）
    QRectF bgRect = fullRect.adjusted(m_margin, m_margin, -m_margin, -m_margin);

    // 背景颜色根据状态
    QColor bg = isChecked() ? m_onColor : m_offColor;

    // 画背景（圆角矩形）
    p.setPen(Qt::NoPen);
    p.setBrush(bg);
    p.drawRoundedRect(bgRect, m_cornerRadius, m_cornerRadius);

    // 中心坐标
    qreal cx = bgRect.center().x();
    qreal cy = bgRect.center().y();

    if (isChecked()) {
        // 运行状态：画两条粗竖线（类似 pause），占区域高度的一部分
        qreal barHeight = bgRect.height() * 0.6;     // 两条竖线高度
        qreal barWidth  = bgRect.width() * 0.08;     // 每条竖线宽度
        qreal gap       = bgRect.width() * 0.08;     // 两条之间的间隙

        // 左条和右条的 x
        qreal totalWidth = barWidth * 2 + gap;
        qreal leftX = cx - totalWidth/2.0;
        qreal rightX = leftX + barWidth + gap;

        QRectF leftBar(leftX, cy - barHeight/2.0, barWidth, barHeight);
        QRectF rightBar(rightX, cy - barHeight/2.0, barWidth, barHeight);

        p.setBrush(m_symbolColor);
        p.drawRoundedRect(leftBar, barWidth/2.0, barWidth/2.0);
        p.drawRoundedRect(rightBar, barWidth/2.0, barWidth/2.0);
    } else {
        // 等待状态：画一个等边三角形，顶点朝右（等边三角形按边长 s）
        // 为让三角形适配矩形：计算可用的最大边长 s，
        // 三角形外接矩形宽 = h = sqrt(3)/2 * s， 高 = s
        qreal availW = bgRect.width() * 0.72; // 留些空白
        qreal availH = bgRect.height() * 0.72;
        const qreal sqrt3 = std::sqrt(3.0);
        // s must satisfy: s <= availH and (sqrt3/2 * s) <= availW
        qreal s1 = availH;
        qreal s2 = availW * 2.0 / sqrt3;
        qreal s = std::min(s1, s2);

        // 顶点按照以质心为原点的坐标（推导见笔记），使三角形中心位于 (cx,cy)
        // 顶点相对中心的坐标（等边，顶点指向右）:
        // v1 = ( s/√3, 0 )
        // v2 = ( -s/(2√3),  s/2 )
        // v3 = ( -s/(2√3), -s/2 )
        qreal vx1 =  s / sqrt3;
        qreal vy1 =  0;
        qreal vx2 = -s / (2.0 * sqrt3);
        qreal vy2 =  s * 0.5;
        qreal vx3 = vx2;
        qreal vy3 = -s * 0.5;

        QPointF p1(cx + vx1, cy + vy1);
        QPointF p2(cx + vx2, cy + vy2);
        QPointF p3(cx + vx3, cy + vy3);

        QPolygonF tri;
        tri << p1 << p2 << p3;

        p.setBrush(m_symbolColor);
        p.drawPolygon(tri);
    }
}

void ToggleRunButton::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // 切换 checked 状态；QAbstractButton 会发出 toggled(bool)
        setChecked(!isChecked());
        update();
    }
    // 不调用父类以确保行为简单明了
}

void ToggleRunButton::resizeEvent(QResizeEvent * /*event*/)
{
    // 仅需重绘以适应新大小
    update();
}

