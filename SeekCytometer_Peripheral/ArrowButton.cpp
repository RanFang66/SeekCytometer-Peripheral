#include "ArrowButton.h"
#include <QPainter>
#include <QPaintEvent>
#include <QtMath>

ArrowButton::ArrowButton(Direction dir, QWidget *parent)
    : QAbstractButton(parent),
    m_direction(dir),
    m_normalColor(Qt::white),
    m_pressedColor(QColor(0, 180, 0)),
    m_outlineColor(QColor(80, 80, 80)),
    m_margin(4)
{
    setCheckable(false);
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::NoFocus);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    setMinimumSize(48, 48);
}

QSize ArrowButton::sizeHint() const
{
    return QSize(64, 64);
}

QPolygonF ArrowButton::buildRightFacingArrowNormalized() const
{
    const qreal fullW = 1.0;
    const qreal fullH = 1.0;

    const qreal shaftLen = 0.52;
    const qreal headLen  = fullW - shaftLen;

    const qreal shaftH = 0.36 * fullH;
    const qreal headBaseH = 0.68 * fullH;

    qreal left  = -fullW/2.0;
    qreal right = fullW/2.0;
    qreal top   = -fullH/2.0;
    qreal bottom= fullH/2.0;

    qreal shaftLeft  = left;
    qreal shaftRight = shaftLeft + shaftLen;

    qreal shaftTop = -shaftH / 2.0;
    qreal shaftBottom = shaftTop + shaftH;

    qreal headHalf = headBaseH / 2.0;
    qreal headBaseTop = -headHalf;
    qreal headBaseBottom = headHalf;
    qreal headTipX = right;

    QPolygonF poly;
    poly << QPointF(shaftLeft, shaftTop);
    poly << QPointF(shaftRight, shaftTop);
    poly << QPointF(shaftRight, headBaseTop);
    poly << QPointF(headTipX, 0.0);
    poly << QPointF(shaftRight, headBaseBottom);
    poly << QPointF(shaftRight, shaftBottom);
    poly << QPointF(shaftLeft, shaftBottom);

    return poly;
}

void ArrowButton::paintEvent(QPaintEvent * /*event*/)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    QRectF area = rect().adjusted(m_margin, m_margin, -m_margin, -m_margin);
    if (area.width() <= 0 || area.height() <= 0) return;

    // 构造单位箭头（面向右），之后对每个点做手算变换：先 scale，再 rotate，再 translate 到 area.center()
    QPolygonF arrow = buildRightFacingArrowNormalized();

    // 以 area 的短边为基准进行等比缩放，保证所有方向都完整显示
    qreal shortSide = qMin(area.width(), area.height());
    const qreal paddingFactor = 0.86; // 缩小比例，避免紧贴边缘
    qreal scale = shortSide * paddingFactor; // 单位坐标(1.0) -> scale 像素

    // 旋转角度（弧度）
    qreal angleDeg = 0;
    switch (m_direction) {
    case Direction::Right: angleDeg = 0; break;
    case Direction::Left:  angleDeg = 180; break;
    case Direction::Up:    angleDeg = -90; break;
    case Direction::Down:  angleDeg = 90; break;
    }
    qreal angle = qDegreesToRadians(angleDeg);
    qreal ca = qCos(angle);
    qreal sa = qSin(angle);

    // 将归一化的点映射到像素坐标（中心对齐）
    QPointF center = area.center();
    QPolygonF arrowPx;
    arrowPx.reserve(arrow.size());
    for (const QPointF &pt : arrow) {
        // 先按 scale 缩放
        qreal sx = pt.x() * scale;
        qreal sy = pt.y() * scale;
        // 再绕原点旋转
        qreal rx = sx * ca - sy * sa;
        qreal ry = sx * sa + sy * ca;
        // 最后平移到中心像素坐标
        arrowPx << QPointF(center.x() + rx, center.y() + ry);
    }

    QColor fill = isDown() ? m_pressedColor : m_normalColor;

    if (isDown()) {
        p.setPen(Qt::NoPen);
        p.setBrush(fill);
        p.drawPolygon(arrowPx);
    } else {
        p.setPen(Qt::NoPen);
        p.setBrush(fill);
        p.drawPolygon(arrowPx);
        QPen pen(m_outlineColor);
        pen.setWidthF(qMax(1.0, scale * 0.02));
        pen.setJoinStyle(Qt::RoundJoin);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        p.drawPolygon(arrowPx);
    }
}

void ArrowButton::resizeEvent(QResizeEvent * /*event*/)
{
    update();
}
