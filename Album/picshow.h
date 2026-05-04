#ifndef PICSHOW_H
#define PICSHOW_H

#include <QWidget>
#include <QPixmap>
#include <QStringList>
#include <QPointF>

class Picshow : public QWidget
{
    Q_OBJECT
public:
    explicit Picshow(QWidget *parent = nullptr);
    ~Picshow() override = default;
    // 设置图片列表和当前显示索引
    void setPhotoList(const QStringList& photoList, int currentIndex);
protected:
    // 绘图事件：双缓冲绘图
    void paintEvent(QPaintEvent* event) override;
    // 鼠标事件：拖拽平移
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    // 滚轮事件：缩放
    void wheelEvent(QWheelEvent* event) override;
    // 键盘事件：切换上下张、ESC关闭、F键自适应
    void keyPressEvent(QKeyEvent* event) override;
    // 窗口显示事件：初始自适应
    void showEvent(QShowEvent* event) override;

private:
    // 加载当前图片
    void loadCurrentPhoto();
    // 自适应窗口大小
    void fitToWindow();
    // 切换图片
    void switchPhoto(int delta);

    // 成员变量
    QPixmap m_currentPixmap;   // 当前图片
    qreal m_scaleFactor;        // 缩放比例
    QPointF m_offset;           // 平移偏移量
    QStringList m_photoList;    // 图片路径列表
    int m_currentIndex;         // 当前图片索引
    bool m_isDragging;          // 是否正在拖拽
    QPointF m_lastMousePos;     // 上次鼠标位置
};

#endif // PICSHOW_H
