#ifndef TOGGLERUNBUTTON_H
#define TOGGLERUNBUTTON_H

#include <QAbstractButton>
#include <QColor>

class ToggleRunButton : public QAbstractButton
{
    Q_OBJECT
public:
    explicit ToggleRunButton(QWidget *parent = nullptr);

    QSize sizeHint() const override;

    // 可选设置接口
    void setOnColor(const QColor &c)  { m_onColor = c; update(); }
    void setOffColor(const QColor &c) { m_offColor = c; update(); }
    void setSymbolColor(const QColor &c){ m_symbolColor = c; update(); }
    void setCornerRadius(int r) { m_cornerRadius = r; update(); }

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QColor m_onColor   = QColor(0, 180, 0);        // 绿色（运行）
    QColor m_offColor  = QColor(205, 205, 205);    // 淡灰（等待）
    QColor m_symbolColor = QColor(20, 20, 185);    // 符号颜色（条/三角）
    int m_margin = 6;          // 内边距
    int m_cornerRadius = 8;    // 矩形圆角
};

#endif // TOGGLERUNBUTTON_H
