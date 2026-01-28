#ifndef TOGGLESWITCH_H
#define TOGGLESWITCH_H

#include <QAbstractButton>
#include <QColor>

class ToggleSwitch : public QAbstractButton
{
    Q_OBJECT

public:
    explicit ToggleSwitch(QWidget *parent = nullptr);

    QSize sizeHint() const override;

    // 可选接口：设置颜色
    void setOnColor(const QColor &c)  { m_onColor = c; update(); }
    void setOffColor(const QColor &c) { m_offColor = c; update(); }
    void setThumbColor(const QColor &c){ m_thumbColor = c; update(); }

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    // 外观参数
    QColor m_onColor  = QColor(0, 180, 0);
    QColor m_offColor = QColor(190, 190, 190);
    QColor m_thumbColor= QColor(255, 255, 255);
    int m_margin = 3;
};

#endif // TOGGLESWITCH_H
