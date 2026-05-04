#include "picshow.h"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QDebug>

Picshow::Picshow(QWidget *parent)
    : QWidget{parent}
    , m_scaleFactor(1.0)
    , m_isDragging(false)
    , m_currentIndex(0)
{
    setWindowTitle("图片预览");
    setMinimumSize(800, 600);
    setStyleSheet("background-color: #1a1a1a;"); // 深色背景

//一个典型的 Qt 焦点问题：PicShow窗口没有抢到键盘焦点，
// 👇 核心修复1：设置窗口标志，让它成为独立的顶级窗口
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
// 强制设置焦点策略，确保能抢到键盘焦点
// 👇 核心修复2：强制设置焦点策略，确保能抢到键盘焦点
    setFocusPolicy(Qt::StrongFocus);
}

// 设置图片列表和当前显示索引
void Picshow::setPhotoList(const QStringList& photoList, int currentIndex){
    m_photoList = photoList;
    m_currentIndex = currentIndex;
    loadCurrentPhoto();
}

//加载当前图片
void Picshow::loadCurrentPhoto(){
    if(m_currentIndex < 0 || m_currentIndex >= m_photoList.size()){
        m_currentPixmap = QPixmap();
        update();
        return;
    }
    QString photoPath = m_photoList.at(m_currentIndex);
    m_currentPixmap.load(photoPath);
    if(m_currentPixmap.isNull()){
        qDebug() << "加载照片失败:" << photoPath;
    }
    // 重置缩放和平移
    m_scaleFactor = 1.0;
    m_offset = QPointF(0, 0);
    update();
}

// 窗口显示时，初始自适应
void Picshow::showEvent(QShowEvent* event)
{
    Q_UNUSED(event); // 1 question
    fitToWindow();
// 👇 核心修复3：显示时强制设置焦点
    this->setFocus(Qt::ActiveWindowFocusReason);
    this->activateWindow();
}

// 自适应窗口大小（保持宽高比，居中显示）
void Picshow::fitToWindow()
{
    if(m_currentPixmap.isNull()) return;

    qreal windowW = width();
    qreal windowH = height();
    qreal pixmapW = m_currentPixmap.width();
    qreal pixmapH = m_currentPixmap.height();

    // 计算缩放比例，留10%边距
    qreal scaleX = windowW / pixmapW;
    qreal scaleY = windowH / pixmapH;
    m_scaleFactor = qMin(scaleX, scaleY) * 0.9;

    // 居中偏移
    m_offset = QPointF(
        (windowW - pixmapW * m_scaleFactor) / 2,
        (windowH - pixmapH * m_scaleFactor) / 2
        );

    update();
}

// 双缓冲绘图（解决闪烁）
//双缓冲的核心思路：不在屏幕上直接作画，而是先在内存里创建一张和控件尺寸一致的「后台画布（缓冲区）」，
//把所有要画的内容全部在内存里画好，画完之后，一次性把这张完整的画布贴到屏幕上。整个过程屏幕只刷新 1 次，完全消除闪烁。
void Picshow::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);

    // 先绘制到缓冲区
    QPixmap buffer(size());
    buffer.fill(Qt::transparent);
    QPainter bufferPainter(&buffer);

    if(!m_currentPixmap.isNull()) {
        bufferPainter.translate(m_offset);
        bufferPainter.scale(m_scaleFactor, m_scaleFactor);
        bufferPainter.drawPixmap(0, 0, m_currentPixmap);
    }

    // 一次性绘制到窗口
    painter.drawPixmap(0, 0, buffer);
}

// 鼠标按下：开始拖拽
void Picshow::mousePressEvent(QMouseEvent* event)
{
    if(event->button() == Qt::LeftButton) {
        m_isDragging = true;
        m_lastMousePos = event->pos();
    }
}

// 鼠标移动：执行平移
void Picshow::mouseMoveEvent(QMouseEvent* event)
{
    if(m_isDragging) {
        QPointF delta = event->pos() - m_lastMousePos;
        m_offset += delta;
        m_lastMousePos = event->pos();
        update();
    }
}

// 鼠标释放：结束拖拽
void Picshow::mouseReleaseEvent(QMouseEvent* event)
{
    if(event->button() == Qt::LeftButton) {
        m_isDragging = false;
    }
}

// 滚轮缩放（以鼠标位置为中心）
void Picshow::wheelEvent(QWheelEvent* event)
{
    if(m_currentPixmap.isNull()) return;

    // 缩放因子：向上滚放大1.1倍，向下滚缩小0.9倍
    qreal scaleFactor = event->angleDelta().y() > 0 ? 1.1 : 0.9;
    qreal newScale = m_scaleFactor * scaleFactor;

    // 限制缩放范围：0.1x ~ 5x
    if(newScale < 0.1 || newScale > 5) return;

    // 以鼠标位置为中心缩放
    QPointF mousePos = event->position();
    QPointF offsetBefore = (mousePos - m_offset) / m_scaleFactor;
    m_scaleFactor = newScale;
    m_offset = mousePos - offsetBefore * m_scaleFactor;

    update();
}

// 键盘事件
void Picshow::keyPressEvent(QKeyEvent* event)
{
    switch(event->key()) {
    case Qt::Key_Left:
        switchPhoto(-1); // 上一张
        break;
    case Qt::Key_Right:
        switchPhoto(1);  // 下一张
        break;
    case Qt::Key_Escape:
        close();          // ESC关闭窗口
        break;
    case Qt::Key_F:
        fitToWindow();    // F键重新自适应
        break;
    default:
        QWidget::keyPressEvent(event);
    }
}

// 切换图片
void Picshow::switchPhoto(int delta)
{
    int newIndex = m_currentIndex + delta;
    if(newIndex < 0 || newIndex >= m_photoList.size()) return;

    m_currentIndex = newIndex;
    loadCurrentPhoto();
    fitToWindow(); // 切换后自动自适应
}








