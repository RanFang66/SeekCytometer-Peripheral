#ifndef ARROWBUTTON_H
#define ARROWBUTTON_H
#include <QAbstractButton>
#include <QColor>

class ArrowButton : public QAbstractButton
{
    Q_OBJECT
public:
    enum class Direction { Up, Down, Left, Right };

    explicit ArrowButton(Direction dir = Direction::Right, QWidget *parent = nullptr);

    QSize sizeHint() const override;

    // 设置/获取方向
    void setDirection(Direction d) { m_direction = d; update(); }
    Direction direction() const { return m_direction; }

    // 配色接口
    void setNormalColor(const QColor &c)  { m_normalColor = c; update(); }
    void setPressedColor(const QColor &c) { m_pressedColor = c; update(); }
    void setOutlineColor(const QColor &c) { m_outlineColor = c; update(); }

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    // 在单位正方形坐标系（中心在 (0,0)，范围约为 [-0.5,0.5]）中构造面向右的箭头多边形
    QPolygonF buildRightFacingArrowNormalized() const;

    Direction m_direction;
    QColor m_normalColor;   // 未按下时箭头填充（白）
    QColor m_pressedColor;  // 按下时箭头填充（绿）
    QColor m_outlineColor;  // 未按下时轮廓色（深色）
    int m_margin;           // 控件内容与边界的内边距（像素）
};
#endif // ARROWBUTTON_H
