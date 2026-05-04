# 电子相册 **第五步完整代码（严格延续分层架构）**
## 第五步核心目标
1.  **视图层新增**：自定义`PicShow`控件，实现双缓冲绘图、鼠标拖拽平移、滚轮缩放、键盘切换
2.  **控制层联动**：监听右侧缩略图双击事件，弹出大图窗口，传递当前相册所有图片
3.  **架构延续**：所有数据查询仍在`AlbumProject`，`PicShow`纯负责UI交互，`MainWindow`仅调度

---

## 一、第一步：创建自定义大图展示控件 `PicShow`（视图层）
### 1. `PicShow.h`
```cpp
#ifndef PICSHOW_H
#define PICSHOW_H

#include <QWidget>
#include <QPixmap>
#include <QStringList>
#include <QPointF>

class PicShow : public QWidget
{
    Q_OBJECT
public:
    explicit PicShow(QWidget *parent = nullptr);
    ~PicShow() override = default;

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
```

### 2. `PicShow.cpp`
```cpp
#include "PicShow.h"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QDebug>

PicShow::PicShow(QWidget *parent)
    : QWidget(parent)
    , m_scaleFactor(1.0)
    , m_isDragging(false)
    , m_currentIndex(0)
{
    setWindowTitle("图片预览");
    setMinimumSize(800, 600);
    setStyleSheet("background-color: #1a1a1a;"); // 深色背景，和整体主题统一
}

// 设置图片列表和当前索引
void PicShow::setPhotoList(const QStringList& photoList, int currentIndex)
{
    m_photoList = photoList;
    m_currentIndex = currentIndex;
    loadCurrentPhoto();
}

// 加载当前图片
void PicShow::loadCurrentPhoto()
{
    if(m_currentIndex < 0 || m_currentIndex >= m_photoList.size()) {
        m_currentPixmap = QPixmap();
        update();
        return;
    }

    QString photoPath = m_photoList.at(m_currentIndex);
    m_currentPixmap.load(photoPath);
    if(m_currentPixmap.isNull()) {
        qDebug() << "加载图片失败：" << photoPath;
    }

    // 重置缩放和平移
    m_scaleFactor = 1.0;
    m_offset = QPointF(0, 0);
    update();
}

// 窗口显示时，初始自适应
void PicShow::showEvent(QShowEvent* event)
{
    Q_UNUSED(event);
    fitToWindow();
}

// 自适应窗口大小（保持宽高比，居中显示）
void PicShow::fitToWindow()
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
void PicShow::paintEvent(QPaintEvent* event)
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
void PicShow::mousePressEvent(QMouseEvent* event)
{
    if(event->button() == Qt::LeftButton) {
        m_isDragging = true;
        m_lastMousePos = event->pos();
    }
}

// 鼠标移动：执行平移
void PicShow::mouseMoveEvent(QMouseEvent* event)
{
    if(m_isDragging) {
        QPointF delta = event->pos() - m_lastMousePos;
        m_offset += delta;
        m_lastMousePos = event->pos();
        update();
    }
}

// 鼠标释放：结束拖拽
void PicShow::mouseReleaseEvent(QMouseEvent* event)
{
    if(event->button() == Qt::LeftButton) {
        m_isDragging = false;
    }
}

// 滚轮缩放（以鼠标位置为中心）
void PicShow::wheelEvent(QWheelEvent* event)
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
void PicShow::keyPressEvent(QKeyEvent* event)
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
void PicShow::switchPhoto(int delta)
{
    int newIndex = m_currentIndex + delta;
    if(newIndex < 0 || newIndex >= m_photoList.size()) return;

    m_currentIndex = newIndex;
    loadCurrentPhoto();
    fitToWindow(); // 切换后自动自适应
}
```

---

## 二、第二步：修改控制层 `MainWindow`，实现双击联动
### 1. 修改 `MainWindow.h`
```cpp
// ... 保留之前的头文件内容 ...
private slots:
    // ... 保留之前的槽函数 ...
    void onPhotoItemDoubleClicked(QListWidgetItem* item); // 新增：双击缩略图槽函数
// ... 保留之前的内容 ...
```

### 2. 修改 `MainWindow.cpp`
```cpp
#include "PicShow.h" // 开头添加：包含PicShow头文件

// ... 保留之前的代码 ...

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("电子相册");

    initContextMenu();
    initPhotoListWidget();

    connect(&AlbumProject::getInstance(), &AlbumProject::albumListChanged,
            this, &MainWindow::refreshTreeWidget);

    connect(ui->treeWidgetAlbum, &QTreeWidget::itemSelectionChanged,
            this, [=](){
        QList<QTreeWidgetItem*> selectedItems = ui->treeWidgetAlbum->selectedItems();
        if(!selectedItems.isEmpty()) {
            onTreeItemSelected(selectedItems.first(), 0);
        }
    });

    // 新增：连接缩略图双击信号
    connect(ui->listWidgetPhoto, &QListWidget::itemDoubleClicked,
            this, &MainWindow::onPhotoItemDoubleClicked);
}

// ... 保留之前的代码 ...

// -------------------------- 新增：双击缩略图展示大图 --------------------------
void MainWindow::onPhotoItemDoubleClicked(QListWidgetItem* item)
{
    if(!item) return;

    // 1. 获取当前选中的相册
    QTreeWidgetItem* treeItem = ui->treeWidgetAlbum->currentItem();
    if(!treeItem) return;
    QString nodeType = treeItem->data(0, Qt::UserRole).toString();
    if(nodeType != "album") return;
    QString albumName = treeItem->data(0, Qt::UserRole+1).toString();

    // 2. 从数据层获取相册所有图片
    QStringList photoList = AlbumProject::getInstance().getPhotoList(albumName);
    if(photoList.isEmpty()) return;

    // 3. 找到当前双击图片的索引
    QString currentPath = item->data(Qt::UserRole).toString();
    int currentIndex = photoList.indexOf(currentPath);
    if(currentIndex == -1) currentIndex = 0;

    // 4. 创建并显示大图窗口
    PicShow* picShow = new PicShow(this);
    picShow->setAttribute(Qt::WA_DeleteOnClose); // 关闭时自动释放内存
    picShow->setPhotoList(photoList, currentIndex);
    picShow->showMaximized(); // 最大化显示
}
```

---

## 三、测试验证（第五步完成标准）
1.  ✅ 双击右侧缩略图，弹出**最大化的大图窗口**，深色背景
2.  ✅ 图片初始**自适应窗口大小**，居中显示，不变形
3.  ✅ 鼠标**滚轮缩放**：以鼠标位置为中心，限制0.1x~5x
4.  ✅ 鼠标**左键拖拽**：平移图片，松手停止
5.  ✅ 键盘**左右方向键**：切换上下张图片，切换后自动自适应
6.  ✅ 键盘**ESC键**：关闭窗口，**F键**：重新自适应窗口
7.  ✅ 窗口关闭时自动释放内存，无内存泄漏

---

## ✅ 架构延续性说明
这一步完全遵循了之前的分层架构：
- **数据层**：`AlbumProject`仍只负责提供图片路径列表，无任何修改
- **视图层**：`PicShow`是纯UI控件，只负责绘图和交互，不涉及数据逻辑
- **控制层**：`MainWindow`仅负责监听双击事件、调用数据层、传递数据、弹出窗口

为下一步做好了完美铺垫：
- 第六步做「导入图片」：只需要在菜单栏加导入动作，调用`QFileDialog`选择图片，复制到相册目录，刷新缩略图即可
- 所有数据都在单例中，任何模块都能随时获取
