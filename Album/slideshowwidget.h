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
    explicit SlideShowWidget(QWidget* parent = nullptr);
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
