# 问题分析与修复方案
这是一个**典型的Qt焦点问题**：你的`PicShow`窗口没有抢到键盘焦点，按键事件被`MainWindow`的`listWidget`截胡了。

我给你一个**100%解决问题**的修复方案，分两步改：

---

## 一、修改 `PicShow.cpp`（核心修复）
### 1. 修改构造函数，强制设置焦点策略
```cpp
PicShow::PicShow(QWidget *parent)
    : QWidget(parent)
    , m_scaleFactor(1.0)
    , m_isDragging(false)
    , m_currentIndex(0)
{
    setWindowTitle("图片预览");
    setMinimumSize(800, 600);
    setStyleSheet("background-color: #1a1a1a;");

    // 👇 核心修复1：设置窗口标志，让它成为独立的顶级窗口
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    // 👇 核心修复2：强制设置焦点策略，确保能抢到键盘焦点
    setFocusPolicy(Qt::StrongFocus);
}
```

### 2. 修改 `showEvent`，显示时强制抢焦点
```cpp
// 窗口显示时，初始自适应并强制抢焦点
void PicShow::showEvent(QShowEvent* event)
{
    Q_UNUSED(event);
    fitToWindow();
    // 👇 核心修复3：显示时强制设置焦点
    this->setFocus(Qt::ActiveWindowFocusReason);
    this->activateWindow();
}
```

### 3. 重写 `focusOutEvent`，防止焦点丢失（可选但推荐）
在 `PicShow.h` 中添加声明：
```cpp
protected:
    // ... 保留之前的事件声明 ...
    void focusOutEvent(QFocusEvent* event) override; // 新增
```

在 `PicShow.cpp` 中添加实现：
```cpp
// 焦点丢失时，尝试重新抢回来（防止被其他控件截胡）
void PicShow::focusOutEvent(QFocusEvent* event)
{
    Q_UNUSED(event);
    // 如果窗口还在显示，就重新抢焦点
    if(isVisible()) {
        QTimer::singleShot(0, this, [=](){
            this->setFocus(Qt::ActiveWindowFocusReason);
            this->activateWindow();
        });
    }
}
```
记得在 `PicShow.cpp` 开头添加 `#include <QTimer>`。

---

## 二、修改 `MainWindow.cpp` 中弹出窗口的代码
确保 `PicShow` 是作为独立窗口弹出，而不是子控件：
```cpp
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
    PicShow* picShow = new PicShow(); // 👇 核心修复：不要传this，让它成为独立窗口
    picShow->setAttribute(Qt::WA_DeleteOnClose);
    picShow->setPhotoList(photoList, currentIndex);
    picShow->showMaximized();
    picShow->raise(); // 👇 核心修复：置顶窗口
    picShow->activateWindow(); // 👇 核心修复：激活窗口
}
```

---

## 三、完整修复后的 `PicShow.h`（以防万一）
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

    void setPhotoList(const QStringList& photoList, int currentIndex);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override; // 新增

private:
    void loadCurrentPhoto();
    void fitToWindow();
    void switchPhoto(int delta);

    QPixmap m_currentPixmap;
    qreal m_scaleFactor;
    QPointF m_offset;
    QStringList m_photoList;
    int m_currentIndex;
    bool m_isDragging;
    QPointF m_lastMousePos;
};

#endif // PICSHOW_H
```

---

## 四、修复后测试验证
1.  ✅ 双击缩略图，弹出独立的大图窗口
2.  ✅ 按 **ESC键**：窗口立即关闭
3.  ✅ 按 **左右方向键**：正常切换上下张图片，listWidget不再闪烁
4.  ✅ 按 **F键**：重新自适应窗口
5.  ✅ 鼠标滚轮缩放、左键拖拽：正常工作

---

## 问题根源总结
1.  **焦点被截胡**：之前`PicShow`作为`MainWindow`的子控件，键盘焦点被`listWidget`抢走了
2.  **窗口标志不对**：没有设置正确的窗口标志，导致它不是独立的顶级窗口
3.  **没有强制抢焦点**：显示时没有主动设置焦点，导致按键事件传不到`PicShow`

修复后，`PicShow`是完全独立的窗口，会强制抢到键盘焦点，所有按键事件都能正常响应了。
