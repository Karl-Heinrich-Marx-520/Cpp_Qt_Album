#include "slideshowwidget.h"
#include <QKeyEvent>
#include <QScreen>
#include <QGuiApplication>
#include <QDebug>


SlideShowWidget::SlideShowWidget(QWidget* parent)
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











