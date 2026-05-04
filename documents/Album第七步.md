# 电子相册 **第七步完整代码（严格延续分层架构）**
## 第七步核心目标
1.  **视图层新增**：自定义`SlideShowWidget`窗口，实现双图片控件叠加、淡入淡出动画、播放控制栏
2.  **控制层联动**：在右侧缩略图列表右键菜单添加「幻灯片播放」选项，传递图片列表
3.  **架构延续**：所有数据查询仍在`AlbumProject`，`SlideShowWidget`纯负责UI和动画，`MainWindow`仅调度

---

## 一、第一步：创建幻灯片播放窗口 `SlideShowWidget`（视图层）
### 1. `SlideShowWidget.h`
```cpp
#ifndef SLIDESHOWWIDGET_H
#define SLIDESHOWWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QTimer>
#include <QPropertyAnimation>
#include <QStringList>
#include <QVBoxLayout>
#include <QHBoxLayout>

class SlideShowWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SlideShowWidget(QWidget *parent = nullptr);
    ~SlideShowWidget() override = default;

    // 设置图片列表和当前索引
    void setPhotoList(const QStringList& photoList, int currentIndex);

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void showEvent(QShowEvent* event) override;

private slots:
    void onPlayPauseClicked();       // 播放/暂停
    void onPrevClicked();             // 上一张
    void onNextClicked();             // 下一张
    void onTimerTimeout();            // 定时器超时（自动播放）
    void onSliderValueChanged(int value); // 进度条拖动
    void onFadeAnimationFinished();   // 淡入淡出动画结束

private:
    void initUI();                    // 初始化UI
    void initAnimations();            // 初始化动画
    void loadCurrentPhoto();          // 加载当前图片
    void startFadeAnimation(const QString& nextPhotoPath); // 开始淡入淡出切换
    void updatePlayPauseButton();     // 更新播放/暂停按钮文字
    void updateSlider();              // 更新进度条

    // UI控件
    QLabel* m_labelCurrent;           // 当前图片（底层）
    QLabel* m_labelNext;              // 下一张图片（上层，用于动画）
    QPushButton* m_btnPlayPause;      // 播放/暂停按钮
    QPushButton* m_btnPrev;           // 上一张按钮
    QPushButton* m_btnNext;           // 下一张按钮
    QPushButton* m_btnClose;          // 关闭按钮
    QSlider* m_sliderProgress;        // 进度条
    QLabel* m_labelProgress;          // 进度文字（如 1/10）

    // 动画和定时器
    QPropertyAnimation* m_animationFadeIn;  // 淡入动画
    QPropertyAnimation* m_animationFadeOut; // 淡出动画
    QTimer* m_timer;                  // 自动播放定时器

    // 数据
    QStringList m_photoList;          // 图片路径列表
    int m_currentIndex;                // 当前图片索引
    bool m_isPlaying;                  // 是否正在播放
    static const int s_interval = 3000; // 自动播放间隔（毫秒）
};

#endif // SLIDESHOWWIDGET_H
```

### 2. `SlideShowWidget.cpp`
```cpp
#include "SlideShowWidget.h"
#include <QKeyEvent>
#include <QScreen>
#include <QGuiApplication>
#include <QDebug>

SlideShowWidget::SlideShowWidget(QWidget *parent)
    : QWidget(parent)
    , m_currentIndex(0)
    , m_isPlaying(true)
{
    setWindowTitle("幻灯片播放");
    setStyleSheet(R"(
        SlideShowWidget { background-color: #1a1a1a; }
        QPushButton {
            background-color: #333; color: white; border: 1px solid #555;
            padding: 8px 16px; border-radius: 4px;
        }
        QPushButton:hover { background-color: #444; }
        QSlider::groove:horizontal {
            height: 6px; background: #333; border-radius: 3px;
        }
        QSlider::handle:horizontal {
            width: 16px; height: 16px; background: #0078d4;
            border-radius: 8px; margin: -5px 0;
        }
        QLabel { color: white; }
    )");

    setWindowFlags(Qt::Window);
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(1024, 768);

    initUI();
    initAnimations();
}

// 初始化UI
void SlideShowWidget::initUI()
{
    // 1. 图片显示区域（两个叠加的Label，用于淡入淡出）
    QWidget* photoWidget = new QWidget(this);
    photoWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QVBoxLayout* photoLayout = new QVBoxLayout(photoWidget);
    photoLayout->setContentsMargins(0, 0, 0, 0);

    m_labelCurrent = new QLabel(photoWidget);
    m_labelCurrent->setAlignment(Qt::AlignCenter);
    m_labelCurrent->setStyleSheet("background-color: transparent;");

    m_labelNext = new QLabel(photoWidget);
    m_labelNext->setAlignment(Qt::AlignCenter);
    m_labelNext->setStyleSheet("background-color: transparent;");
    m_labelNext->raise(); // 放在上层

    photoLayout->addWidget(m_labelCurrent);
    // 手动设置m_labelNext的几何形状，和m_labelCurrent完全重叠
    m_labelNext->setGeometry(m_labelCurrent->geometry());

    // 2. 底部控制栏
    QWidget* controlWidget = new QWidget(this);
    controlWidget->setMaximumHeight(60);
    QHBoxLayout* controlLayout = new QHBoxLayout(controlWidget);
    controlLayout->setContentsMargins(20, 10, 20, 10);

    m_btnPrev = new QPushButton("上一张", controlWidget);
    m_btnPlayPause = new QPushButton("暂停", controlWidget);
    m_btnNext = new QPushButton("下一张", controlWidget);
    m_sliderProgress = new QSlider(Qt::Horizontal, controlWidget);
    m_labelProgress = new QLabel("0/0", controlWidget);
    m_btnClose = new QPushButton("关闭", controlWidget);

    controlLayout->addWidget(m_btnPrev);
    controlLayout->addWidget(m_btnPlayPause);
    controlLayout->addWidget(m_btnNext);
    controlLayout->addSpacing(20);
    controlLayout->addWidget(m_sliderProgress, 1);
    controlLayout->addWidget(m_labelProgress);
    controlLayout->addSpacing(20);
    controlLayout->addWidget(m_btnClose);

    // 3. 主布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(photoWidget, 1);
    mainLayout->addWidget(controlWidget);

    // 4. 连接信号槽
    connect(m_btnPlayPause, &QPushButton::clicked, this, &SlideShowWidget::onPlayPauseClicked);
    connect(m_btnPrev, &QPushButton::clicked, this, &SlideShowWidget::onPrevClicked);
    connect(m_btnNext, &QPushButton::clicked, this, &SlideShowWidget::onNextClicked);
    connect(m_btnClose, &QPushButton::clicked, this, &SlideShowWidget::close);
    connect(m_sliderProgress, &QSlider::valueChanged, this, &SlideShowWidget::onSliderValueChanged);
}

// 初始化动画
void SlideShowWidget::initAnimations()
{
    // 淡入动画：改变m_labelNext的透明度从0到1
    m_animationFadeIn = new QPropertyAnimation(m_labelNext, "windowOpacity", this);
    m_animationFadeIn->setDuration(800); // 动画时长800毫秒
    m_animationFadeIn->setStartValue(0.0);
    m_animationFadeIn->setEndValue(1.0);
    m_animationFadeIn->setEasingCurve(QEasingCurve::InOutQuad);

    // 淡出动画：改变m_labelCurrent的透明度从1到0
    m_animationFadeOut = new QPropertyAnimation(m_labelCurrent, "windowOpacity", this);
    m_animationFadeOut->setDuration(800);
    m_animationFadeOut->setStartValue(1.0);
    m_animationFadeOut->setEndValue(0.0);
    m_animationFadeOut->setEasingCurve(QEasingCurve::InOutQuad);

    // 动画结束后交换角色
    connect(m_animationFadeIn, &QPropertyAnimation::finished, this, &SlideShowWidget::onFadeAnimationFinished);

    // 初始化定时器
    m_timer = new QTimer(this);
    m_timer->setInterval(s_interval);
    connect(m_timer, &QTimer::timeout, this, &SlideShowWidget::onTimerTimeout);
}

// 设置图片列表和当前索引
void SlideShowWidget::setPhotoList(const QStringList& photoList, int currentIndex)
{
    m_photoList = photoList;
    m_currentIndex = currentIndex;
    m_sliderProgress->setRange(0, photoList.size() - 1);
    updateSlider();
}

// 窗口显示时
void SlideShowWidget::showEvent(QShowEvent* event)
{
    Q_UNUSED(event);

    // 设置窗口大小为屏幕的90%，居中
    QScreen* screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->availableGeometry();
    int windowW = screenGeometry.width() * 0.9;
    int windowH = screenGeometry.height() * 0.9;
    resize(windowW, windowH);
    move((screenGeometry.width() - windowW) / 2, (screenGeometry.height() - windowH) / 2);

    // 手动设置m_labelNext的大小，和m_labelCurrent一致
    m_labelNext->setGeometry(m_labelCurrent->geometry());

    // 加载初始图片
    loadCurrentPhoto();
    m_labelNext->setPixmap(m_labelCurrent->pixmap(Qt::ReturnByValue));
    m_labelNext->setWindowOpacity(1.0);
    m_labelCurrent->setWindowOpacity(1.0);

    // 启动自动播放
    if(m_isPlaying) {
        m_timer->start();
    }

    this->setFocus(Qt::ActiveWindowFocusReason);
    this->activateWindow();
}

// 加载当前图片（自适应窗口）
void SlideShowWidget::loadCurrentPhoto()
{
    if(m_currentIndex < 0 || m_currentIndex >= m_photoList.size()) return;

    QString photoPath = m_photoList.at(m_currentIndex);
    QPixmap pixmap(photoPath);
    if(pixmap.isNull()) {
        qDebug() << "加载图片失败：" << photoPath;
        return;
    }

    // 自适应图片显示区域大小，留10%边距
    QSize labelSize = m_labelCurrent->size() * 0.9;
    QPixmap scaledPixmap = pixmap.scaled(
        labelSize,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
    );

    m_labelCurrent->setPixmap(scaledPixmap);
    updateSlider();
}

// 开始淡入淡出切换动画
void SlideShowWidget::startFadeAnimation(const QString& nextPhotoPath)
{
    // 1. 加载下一张图片到m_labelNext
    QPixmap pixmap(nextPhotoPath);
    if(pixmap.isNull()) return;

    QSize labelSize = m_labelNext->size() * 0.9;
    QPixmap scaledPixmap = pixmap.scaled(
        labelSize,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
    );
    m_labelNext->setPixmap(scaledPixmap);
    m_labelNext->setWindowOpacity(0.0);
    m_labelCurrent->setWindowOpacity(1.0);

    // 2. 同时启动淡入和淡出动画
    m_animationFadeIn->start();
    m_animationFadeOut->start();
}

// 淡入淡出动画结束
void SlideShowWidget::onFadeAnimationFinished()
{
    // 交换两个Label的角色：m_labelNext变成新的m_labelCurrent
    QPixmap temp = m_labelNext->pixmap(Qt::ReturnByValue);
    m_labelCurrent->setPixmap(temp);
    m_labelCurrent->setWindowOpacity(1.0);
    m_labelNext->setWindowOpacity(0.0);
}

// 播放/暂停
void SlideShowWidget::onPlayPauseClicked()
{
    m_isPlaying = !m_isPlaying;
    updatePlayPauseButton();
    if(m_isPlaying) {
        m_timer->start();
    } else {
        m_timer->stop();
    }
}

// 上一张
void SlideShowWidget::onPrevClicked()
{
    int newIndex = m_currentIndex - 1;
    if(newIndex < 0) newIndex = m_photoList.size() - 1;

    if(m_animationFadeIn->state() == QAbstractAnimation::Running) return;

    m_currentIndex = newIndex;
    if(m_currentIndex >= 0 && m_currentIndex < m_photoList.size()) {
        startFadeAnimation(m_photoList.at(m_currentIndex));
    }
    updateSlider();
}

// 下一张
void SlideShowWidget::onNextClicked()
{
    int newIndex = m_currentIndex + 1;
    if(newIndex >= m_photoList.size()) newIndex = 0;

    if(m_animationFadeIn->state() == QAbstractAnimation::Running) return;

    m_currentIndex = newIndex;
    if(m_currentIndex >= 0 && m_currentIndex < m_photoList.size()) {
        startFadeAnimation(m_photoList.at(m_currentIndex));
    }
    updateSlider();
}

// 定时器超时（自动播放）
void SlideShowWidget::onTimerTimeout()
{
    onNextClicked();
}

// 进度条拖动
void SlideShowWidget::onSliderValueChanged(int value)
{
    if(value == m_currentIndex) return;
    if(m_animationFadeIn->state() == QAbstractAnimation::Running) return;

    m_currentIndex = value;
    if(m_currentIndex >= 0 && m_currentIndex < m_photoList.size()) {
        startFadeAnimation(m_photoList.at(m_currentIndex));
    }
}

// 更新播放/暂停按钮文字
void SlideShowWidget::updatePlayPauseButton()
{
    m_btnPlayPause->setText(m_isPlaying ? "暂停" : "播放");
}

// 更新进度条
void SlideShowWidget::updateSlider()
{
    m_sliderProgress->blockSignals(true);
    m_sliderProgress->setValue(m_currentIndex);
    m_sliderProgress->blockSignals(false);
    m_labelProgress->setText(QString("%1/%2").arg(m_currentIndex + 1).arg(m_photoList.size()));
}

// 键盘事件
void SlideShowWidget::keyPressEvent(QKeyEvent* event)
{
    switch(event->key()) {
    case Qt::Key_Left:
        onPrevClicked();
        break;
    case Qt::Key_Right:
        onNextClicked();
        break;
    case Qt::Key_Space:
        onPlayPauseClicked();
        break;
    case Qt::Key_Escape:
        close();
        break;
    default:
        QWidget::keyPressEvent(event);
    }
}
```

---

## 二、第二步：修改控制层 `MainWindow`，实现幻灯片播放联动
### 1. 修改 `MainWindow.h`
```cpp
// ... 保留之前的头文件内容 ...
private slots:
    // ... 保留之前的槽函数 ...
    void on_actionSlideShow_triggered(); // 新增：幻灯片播放槽函数
// ... 保留之前的内容 ...
```

### 2. 修改 `MainWindow.cpp`
```cpp
#include "SlideShowWidget.h" // 开头添加：包含SlideShowWidget头文件

// ... 保留之前的代码 ...

// 修改：初始化右侧缩略图列表，添加右键菜单
void MainWindow::initPhotoListWidget()
{
    ui->listWidgetPhoto->setViewMode(QListWidget::IconMode);
    ui->listWidgetPhoto->setIconSize(QSize(150, 150));
    ui->listWidgetPhoto->setSpacing(10);
    ui->listWidgetPhoto->setResizeMode(QListWidget::Adjust);
    ui->listWidgetPhoto->setSelectionMode(QAbstractItemView::SingleSelection);

    // 新增：设置缩略图列表支持右键菜单
    ui->listWidgetPhoto->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->listWidgetPhoto, &QListWidget::customContextMenuRequested,
            this, [=](const QPoint& pos){
        QMenu menu(this);
        QAction* actSlideShow = menu.addAction("幻灯片播放");
        connect(actSlideShow, &QAction::triggered, this, &MainWindow::on_actionSlideShow_triggered);
        menu.exec(ui->listWidgetPhoto->mapToGlobal(pos));
    });
}

// ... 保留之前的代码 ...

// -------------------------- 新增：幻灯片播放槽函数 --------------------------
void MainWindow::on_actionSlideShow_triggered()
{
    // 1. 校验：必须选中一个相册节点
    QTreeWidgetItem* treeItem = ui->treeWidgetAlbum->currentItem();
    if(!treeItem) return;
    QString nodeType = treeItem->data(0, Qt::UserRole).toString();
    if(nodeType != "album") return;
    QString albumName = treeItem->data(0, Qt::UserRole+1).toString();

    // 2. 从数据层获取相册所有图片
    QStringList photoList = AlbumProject::getInstance().getPhotoList(albumName);
    if(photoList.size() < 1) {
        QMessageBox::information(this, "提示", "相册中至少需要1张图片才能播放幻灯片");
        return;
    }

    // 3. 获取当前选中的图片索引（如果没选中，从第0张开始）
    int currentIndex = 0;
    QListWidgetItem* currentItem = ui->listWidgetPhoto->currentItem();
    if(currentItem) {
        QString currentPath = currentItem->data(Qt::UserRole).toString();
        currentIndex = photoList.indexOf(currentPath);
        if(currentIndex == -1) currentIndex = 0;
    }

    // 4. 创建并显示幻灯片窗口
    SlideShowWidget* slideShow = new SlideShowWidget();
    slideShow->setAttribute(Qt::WA_DeleteOnClose);
    slideShow->setPhotoList(photoList, currentIndex);
    slideShow->show();
    slideShow->raise();
    slideShow->activateWindow();
}
```

---

## 三、测试验证（第七步完成标准）
1.  ✅ 右键点击右侧缩略图列表，弹出菜单，显示「幻灯片播放」选项
2.  ✅ 点击「幻灯片播放」，弹出独立的幻灯片窗口，深色背景
3.  ✅ 窗口大小为**屏幕的90%**，居中显示
4.  ✅ 图片初始自适应窗口，底部有播放控制栏
5.  ✅ **淡入淡出切换动画**流畅，时长800毫秒
6.  ✅ 自动播放：默认3秒自动切换下一张
7.  ✅ 播放/暂停按钮：点击暂停，再次点击继续
8.  ✅ 上一张/下一张按钮：手动切换
9.  ✅ 进度条：显示当前进度，拖动可跳转
10. ✅ 键盘快捷键：
    - 空格：播放/暂停
    - 左右方向键：切换
    - ESC：关闭窗口
11. ✅ 窗口关闭时自动释放内存

---

## ✅ 架构延续性说明
这一步完全遵循了之前的分层架构：
- **数据层**：`AlbumProject`仍只负责提供图片路径列表，无任何修改
- **视图层**：`SlideShowWidget`是纯UI控件，只负责绘图、动画和交互，不涉及数据逻辑
- **控制层**：`MainWindow`仅负责监听右键菜单、调用数据层、传递数据、弹出窗口

至此，电子相册项目已经完成了所有核心功能，完整覆盖了**QTreeWidget/QListWidget、双缓冲绘图、信号槽机制、动画系统、绘图/鼠标事件、QSS样式表**等所有Qt核心知识点！
