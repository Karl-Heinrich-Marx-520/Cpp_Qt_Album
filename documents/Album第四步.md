# 电子相册 **第四步完整教程（严格延续分层架构）**
## 第四步核心目标
1.  **数据层扩展**：在`AlbumProject`中添加「获取相册图片列表」功能，自动过滤常见图片格式
2.  **控制层联动**：监听左侧目录树选中事件，选中相册时自动刷新右侧缩略图
3.  **视图层优化**：右侧`QListWidget`设置为图标模式，生成缩略图展示，避免大图片卡顿
4.  **架构延续**：所有图片路径查询只在数据层完成，MainWindow仅负责调度和UI渲染

---

## 一、第一步：完善数据层 `AlbumProject`（添加图片查询功能）
### 修改 `AlbumProject.h`
```cpp
#ifndef ALBUMPROJECT_H
#define ALBUMPROJECT_H

#include <QObject>
#include <QString>
#include <QStringList>

class AlbumProject : public QObject
{
    Q_OBJECT
public:
    static AlbumProject& getInstance();

    // 项目基础操作
    bool createProject(const QString& projectName, const QString& projectPath);
    bool loadProject(const QString& projectFilePath);
    bool saveProject();

    // 相册操作
    bool addAlbum(const QString& albumName);
    bool deleteAlbum(const QString& albumName);
    QStringList getAlbumList() const;
    QString getAlbumPath(const QString& albumName) const;

    // 图片操作（新增）
    QStringList getPhotoList(const QString& albumName) const; // 获取相册内所有图片路径
    static bool isImageFile(const QString& fileName); // 判断是否为图片文件

    // 项目信息
    QString getProjectName() const;
    QString getProjectRootPath() const;
    QString getProjectFilePath() const;

signals:
    void albumListChanged();

private:
    explicit AlbumProject(QObject *parent = nullptr);
    ~AlbumProject() override = default;

    AlbumProject(const AlbumProject&) = delete;
    AlbumProject& operator=(const AlbumProject&) = delete;

    QString m_projectName;
    QString m_projectRootPath;
    QString m_projectFilePath;
    QStringList m_albumList;
    static QStringList s_supportedImageFormats; // 支持的图片格式列表
};

#endif // ALBUMPROJECT_H
```

### 修改 `AlbumProject.cpp`
```cpp
#include "AlbumProject.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

// 初始化支持的图片格式
QStringList AlbumProject::s_supportedImageFormats = {"jpg", "jpeg", "png", "bmp", "gif", "webp"};

AlbumProject& AlbumProject::getInstance()
{
    static AlbumProject instance;
    return instance;
}

AlbumProject::AlbumProject(QObject *parent)
    : QObject(parent)
{
}

// ... 保留之前的项目/相册操作代码不变 ...

// -------------------------- 新增：图片操作 --------------------------
// 判断是否为支持的图片文件
bool AlbumProject::isImageFile(const QString& fileName)
{
    QString suffix = QFileInfo(fileName).suffix().toLower();
    return s_supportedImageFormats.contains(suffix);
}

// 获取指定相册内的所有图片完整路径
QStringList AlbumProject::getPhotoList(const QString& albumName) const
{
    QStringList photoList;
    QString albumPath = getAlbumPath(albumName);
    QDir dir(albumPath);

    if(!dir.exists()) {
        qDebug() << "相册路径不存在：" << albumPath;
        return photoList;
    }

    // 遍历文件夹，过滤出图片文件
    QFileInfoList fileInfoList = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
    for(const QFileInfo& fileInfo : fileInfoList) {
        if(isImageFile(fileInfo.fileName())) {
            photoList.append(fileInfo.absoluteFilePath());
        }
    }

    qDebug() << "相册" << albumName << "找到" << photoList.size() << "张图片";
    return photoList;
}

// ... 保留之前的getter方法不变 ...
```

> ✅ 架构亮点：
> 1.  图片格式统一管理：后续要添加新格式（如HEIC），只需要修改`s_supportedImageFormats`
> 2.  纯数据查询：不涉及UI，只返回图片路径列表，UI层负责渲染
> 3.  静态工具方法：`isImageFile`可以在任何地方调用，判断文件是否为图片

---

## 二、第二步：完善控制层 `MainWindow`，实现目录树与缩略图联动
### 修改 `MainWindow.h`
```cpp
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMenu>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void loadProject();

private slots:
    void on_action创建项目_triggered();
    void on_action打开项目_triggered();
    void on_actionNewAlbum_triggered();
    void on_actionDeleteAlbum_triggered();
    void refreshTreeWidget();

    // 新增：目录树选中事件槽函数
    void onTreeItemSelected(QTreeWidgetItem* item, int column);
    // 新增：刷新右侧缩略图列表
    void refreshPhotoList(const QString& albumName);

private:
    Ui::MainWindow *ui;
    QMenu* m_treeContextMenu;

    void initTreeWidget();
    void initContextMenu();
    void initPhotoListWidget(); // 新增：初始化右侧缩略图列表
};

#endif // MAINWINDOW_H
```

### 修改 `MainWindow.cpp`
```cpp
#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "AlbumWizard.h"
#include "AlbumProject.h"
#include <QFileDialog>
#include <QTreeWidgetItem>
#include <QInputDialog>
#include <QMessageBox>
#include <QPixmap>
#include <QListWidgetItem>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("电子相册");

    initContextMenu();
    initPhotoListWidget(); // 新增：初始化右侧缩略图列表

    connect(&AlbumProject::getInstance(), &AlbumProject::albumListChanged,
            this, &MainWindow::refreshTreeWidget);

    // 新增：连接目录树选中事件
    connect(ui->treeWidgetAlbum, &QTreeWidget::itemSelectionChanged,
            this, [=](){
        QList<QTreeWidgetItem*> selectedItems = ui->treeWidgetAlbum->selectedItems();
        if(!selectedItems.isEmpty()) {
            onTreeItemSelected(selectedItems.first(), 0);
        }
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}

// 新增：初始化右侧缩略图列表
void MainWindow::initPhotoListWidget()
{
    // 设置为图标模式
    ui->listWidgetPhoto->setViewMode(QListWidget::IconMode);
    // 设置图标大小（150x150像素，可根据需要调整）
    ui->listWidgetPhoto->setIconSize(QSize(150, 150));
    // 设置网格间距
    ui->listWidgetPhoto->setSpacing(10);
    // 设置为可调整大小，自动换行
    ui->listWidgetPhoto->setResizeMode(QListWidget::Adjust);
    // 设置选择模式：单选
    ui->listWidgetPhoto->setSelectionMode(QAbstractItemView::SingleSelection);
}

// 加载项目
void MainWindow::loadProject()
{
    ui->treeWidgetAlbum->clear();
    ui->listWidgetPhoto->clear();

    initTreeWidget();
    setWindowTitle(QString("电子相册 - %1").arg(AlbumProject::getInstance().getProjectName()));
    qDebug() << "项目加载完成";
}

// 初始化目录树
void MainWindow::initTreeWidget()
{
    QTreeWidgetItem* rootItem = new QTreeWidgetItem(ui->treeWidgetAlbum);
    rootItem->setText(0, AlbumProject::getInstance().getProjectName());
    rootItem->setData(0, Qt::UserRole, "root");

    refreshTreeWidget();

    ui->treeWidgetAlbum->expandItem(rootItem);
    ui->treeWidgetAlbum->setCurrentItem(rootItem);
}

// 刷新目录树
void MainWindow::refreshTreeWidget()
{
    QTreeWidgetItem* rootItem = ui->treeWidgetAlbum->topLevelItem(0);
    if(!rootItem) return;

    rootItem->takeChildren();

    QStringList albumList = AlbumProject::getInstance().getAlbumList();
    for(const QString& albumName : albumList) {
        QTreeWidgetItem* albumItem = new QTreeWidgetItem(rootItem);
        albumItem->setText(0, albumName);
        albumItem->setData(0, Qt::UserRole, "album");
        albumItem->setData(0, Qt::UserRole+1, albumName);
    }
}

// 新增：目录树选中事件处理
void MainWindow::onTreeItemSelected(QTreeWidgetItem* item, int column)
{
    Q_UNUSED(column);
    if(!item) return;

    QString nodeType = item->data(0, Qt::UserRole).toString();
    if(nodeType == "album") {
        // 选中相册节点：刷新右侧缩略图
        QString albumName = item->data(0, Qt::UserRole+1).toString();
        refreshPhotoList(albumName);
    } else {
        // 选中根节点：清空右侧缩略图
        ui->listWidgetPhoto->clear();
    }
}

// 新增：刷新右侧缩略图列表
void MainWindow::refreshPhotoList(const QString& albumName)
{
    // 清空之前的缩略图
    ui->listWidgetPhoto->clear();

    // 从数据层获取图片列表
    QStringList photoList = AlbumProject::getInstance().getPhotoList(albumName);
    if(photoList.isEmpty()) {
        qDebug() << "相册" << albumName << "没有图片";
        return;
    }

    // 遍历图片列表，生成缩略图并添加到列表
    for(const QString& photoPath : photoList) {
        // 1. 加载图片并生成缩略图（保持宽高比，平滑缩放）
        QPixmap originalPixmap(photoPath);
        if(originalPixmap.isNull()) {
            qDebug() << "加载图片失败：" << photoPath;
            continue;
        }
        QPixmap thumbnail = originalPixmap.scaled(
            ui->listWidgetPhoto->iconSize(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        );

        // 2. 创建列表项
        QListWidgetItem* item = new QListWidgetItem();
        // 设置缩略图图标
        item->setIcon(QIcon(thumbnail));
        // 设置文件名（去掉路径，只显示文件名）
        item->setText(QFileInfo(photoPath).fileName());
        // 存储完整图片路径（后续双击看大图时用）
        item->setData(Qt::UserRole, photoPath);
        // 设置文字在图标下方
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignBottom);

        // 3. 添加到列表
        ui->listWidgetPhoto->addItem(item);
    }

    qDebug() << "缩略图刷新完成，共" << photoList.size() << "张";
}

// ... 保留之前的右键菜单和原有菜单槽函数不变 ...
```

---

## 三、测试验证（第四步完成标准）
1.  ✅ 选中左侧目录树的「默认相册」节点，右侧`QListWidget`自动切换为图标模式
2.  ✅ 手动往磁盘的「默认相册」文件夹里放几张图片，重新选中相册节点，右侧自动显示所有图片的缩略图
3.  ✅ 缩略图保持宽高比，不会变形，大小统一为150x150像素
4.  ✅ 文件名显示在缩略图下方，文字居中
5.  ✅ 选中根节点时，右侧缩略图自动清空
6.  ✅ 新建相册后，往里面放图片，选中后能正常显示缩略图

---

## ✅ 架构延续性说明
这一步完全遵循了之前的分层架构：
- **数据层**：只负责查询图片路径，不涉及UI渲染
- **控制层**：只负责监听选中事件、调用数据层、生成缩略图、更新UI
- **视图层**：纯展示，没有任何业务逻辑

为下一步做好了完美铺垫：
- 第五步做「双击看大图」：只需要监听`listWidgetPhoto`的`itemDoubleClicked`信号，获取存储的图片路径，弹出大图展示窗口即可
- 所有图片路径都存储在Item的`UserRole`中，随时可以获取，不用重新查询
