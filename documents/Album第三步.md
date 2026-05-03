# 电子相册 **第三步完整教程（严格延续分层架构）**
## 第三步核心目标
1.  **数据层扩展**：在`AlbumProject`中添加相册数据管理，实现磁盘文件夹与内存数据的同步
2.  **视图层完善**：左侧`QTreeWidget`自动展示所有相册，支持多级目录结构
3.  **交互功能**：右键菜单新建相册，创建时自动在磁盘生成对应文件夹
4.  **架构延续**：所有数据操作只在数据层完成，MainWindow仅负责调度和UI更新

---

## 一、第一步：完善数据层 `AlbumProject`（核心，所有功能的基础）
所有相册的增删改查、磁盘文件操作**必须全部放在这里**，绝对不能写在MainWindow里。

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

    // 相册操作（新增）
    bool addAlbum(const QString& albumName); // 新建相册
    bool deleteAlbum(const QString& albumName); // 删除相册
    QStringList getAlbumList() const; // 获取所有相册名
    QString getAlbumPath(const QString& albumName) const; // 获取相册的磁盘路径

    // 项目信息
    QString getProjectName() const;
    QString getProjectRootPath() const;
    QString getProjectFilePath() const;

signals:
    // 数据变化信号（新增）：通知UI刷新
    void albumListChanged();

private:
    explicit AlbumProject(QObject *parent = nullptr);
    ~AlbumProject() override = default;

    AlbumProject(const AlbumProject&) = delete;
    AlbumProject& operator=(const AlbumProject&) = delete;

    QString m_projectName;
    QString m_projectRootPath;
    QString m_projectFilePath;
    QStringList m_albumList; // 相册名称列表
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

AlbumProject& AlbumProject::getInstance()
{
    static AlbumProject instance;
    return instance;
}

AlbumProject::AlbumProject(QObject *parent)
    : QObject(parent)
{
}

// 创建新项目
bool AlbumProject::createProject(const QString& projectName, const QString& projectPath)
{
    QString fullPath = QDir(projectPath).filePath(projectName);
    QDir dir;

    if(!dir.mkpath(fullPath)) {
        qDebug() << "创建项目目录失败：" << fullPath;
        return false;
    }

    m_projectName = projectName;
    m_projectRootPath = fullPath;
    m_projectFilePath = QDir(fullPath).filePath(projectName + ".album");
    m_albumList.clear();

    // 新增：创建项目时默认生成一个"默认相册"
    addAlbum("默认相册");

    return saveProject();
}

// 保存项目
bool AlbumProject::saveProject()
{
    QFile file(m_projectFilePath);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "打开项目文件失败：" << m_projectFilePath;
        return false;
    }

    QJsonObject obj;
    obj["projectName"] = m_projectName;
    obj["projectRootPath"] = m_projectRootPath;
    obj["albumList"] = QJsonArray::fromStringList(m_albumList); // 保存相册列表

    QJsonDocument doc(obj);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    qDebug() << "项目保存成功：" << m_projectFilePath;
    return true;
}

// 加载项目
bool AlbumProject::loadProject(const QString& projectFilePath)
{
    QFile file(projectFilePath);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "打开项目文件失败：" << projectFilePath;
        return false;
    }

    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if(doc.isNull()) {
        qDebug() << "JSON解析失败";
        file.close();
        return false;
    }

    QJsonObject obj = doc.object();
    m_projectName = obj["projectName"].toString();
    m_projectRootPath = obj["projectRootPath"].toString();
    m_projectFilePath = projectFilePath;
    m_albumList = obj["albumList"].toVariant().toStringList(); // 加载相册列表

    file.close();
    qDebug() << "项目加载成功：" << m_projectName;
    return true;
}

// -------------------------- 新增：相册操作 --------------------------
// 新建相册：同时在内存和磁盘创建
bool AlbumProject::addAlbum(const QString& albumName)
{
    // 1. 校验：相册名不能为空、不能重复
    if(albumName.trimmed().isEmpty()) {
        qDebug() << "相册名不能为空";
        return false;
    }
    if(m_albumList.contains(albumName)) {
        qDebug() << "相册已存在：" << albumName;
        return false;
    }

    // 2. 在磁盘创建相册文件夹
    QString albumPath = QDir(m_projectRootPath).filePath(albumName);
    QDir dir;
    if(!dir.mkpath(albumPath)) {
        qDebug() << "创建相册文件夹失败：" << albumPath;
        return false;
    }

    // 3. 内存中添加相册
    m_albumList.append(albumName);

    // 4. 保存项目配置
    if(!saveProject()) {
        qDebug() << "保存项目失败";
        return false;
    }

    // 5. 发送信号，通知UI刷新
    emit albumListChanged();
    qDebug() << "相册创建成功：" << albumName;
    return true;
}

// 删除相册：同时删除内存数据和磁盘文件夹
bool AlbumProject::deleteAlbum(const QString& albumName)
{
    if(!m_albumList.contains(albumName)) {
        qDebug() << "相册不存在：" << albumName;
        return false;
    }

    // 1. 删除磁盘文件夹（可选，谨慎操作）
    QString albumPath = getAlbumPath(albumName);
    QDir dir(albumPath);
    if(dir.exists()) {
        dir.removeRecursively(); // 递归删除文件夹及所有内容
    }

    // 2. 内存中删除
    m_albumList.removeOne(albumName);

    // 3. 保存配置
    saveProject();

    // 4. 通知UI刷新
    emit albumListChanged();
    qDebug() << "相册删除成功：" << albumName;
    return true;
}

// 获取所有相册名
QStringList AlbumProject::getAlbumList() const
{
    return m_albumList;
}

// 获取相册的完整磁盘路径
QString AlbumProject::getAlbumPath(const QString& albumName) const
{
    return QDir(m_projectRootPath).filePath(albumName);
}

// 基础getter
QString AlbumProject::getProjectName() const { return m_projectName; }
QString AlbumProject::getProjectRootPath() const { return m_projectRootPath; }
QString AlbumProject::getProjectFilePath() const { return m_projectFilePath; }
```

> ✅ 架构亮点：
> 1.  数据与磁盘完全同步：新建/删除相册时，自动操作磁盘文件夹
> 2.  信号通知机制：数据变化时自动发信号，UI只需要监听信号刷新，不用主动轮询
> 3.  单一职责：所有数据逻辑都在这里，UI层完全不用关心磁盘操作

---

## 二、第二步：完善控制层 `MainWindow`，实现目录树自动刷新
### 修改 `MainWindow.h`
```cpp
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMenu> // 新增：右键菜单

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

    // 新增：右键菜单槽函数
    void on_actionNewAlbum_triggered();
    void on_actionDeleteAlbum_triggered();

    // 新增：刷新目录树
    void refreshTreeWidget();

private:
    Ui::MainWindow *ui;
    QMenu* m_treeContextMenu; // 目录树右键菜单

    void initTreeWidget();
    void initContextMenu(); // 新增：初始化右键菜单
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
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("电子相册");

    // 初始化右键菜单
    initContextMenu();

    // 关键：监听数据层的相册变化信号，自动刷新目录树
    connect(&AlbumProject::getInstance(), &AlbumProject::albumListChanged,
            this, &MainWindow::refreshTreeWidget);
}

MainWindow::~MainWindow()
{
    delete ui;
}

// 初始化右键菜单
void MainWindow::initContextMenu()
{
    // 创建菜单
    m_treeContextMenu = new QMenu(this);
    QAction* actNewAlbum = m_treeContextMenu->addAction("新建相册");
    QAction* actDeleteAlbum = m_treeContextMenu->addAction("删除相册");

    // 绑定槽函数
    connect(actNewAlbum, &QAction::triggered, this, &MainWindow::on_actionNewAlbum_triggered);
    connect(actDeleteAlbum, &QAction::triggered, this, &MainWindow::on_actionDeleteAlbum_triggered);

    // 设置目录树支持右键菜单
    ui->treeWidgetAlbum->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->treeWidgetAlbum, &QTreeWidget::customContextMenuRequested,
            this, [=](const QPoint& pos){
        // 在鼠标点击位置弹出菜单
        m_treeContextMenu->exec(ui->treeWidgetAlbum->mapToGlobal(pos));
    });
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
    // 根节点：项目名
    QTreeWidgetItem* rootItem = new QTreeWidgetItem(ui->treeWidgetAlbum);
    rootItem->setText(0, AlbumProject::getInstance().getProjectName());
    rootItem->setIcon(0, QIcon(":/icon/folder.png")); // 替换成你的文件夹图标
    rootItem->setData(0, Qt::UserRole, "root"); // 标记为根节点

    // 遍历所有相册，创建子节点
    refreshTreeWidget();

    // 默认展开根节点
    ui->treeWidgetAlbum->expandItem(rootItem);
    ui->treeWidgetAlbum->setCurrentItem(rootItem);
}

// 刷新目录树：只更新相册子节点，不重建根节点
void MainWindow::refreshTreeWidget()
{
    // 获取根节点
    QTreeWidgetItem* rootItem = ui->treeWidgetAlbum->topLevelItem(0);
    if(!rootItem) return;

    // 清空所有相册子节点
    rootItem->takeChildren();

    // 从数据层获取最新相册列表，重新创建节点
    QStringList albumList = AlbumProject::getInstance().getAlbumList();
    for(const QString& albumName : albumList) {
        QTreeWidgetItem* albumItem = new QTreeWidgetItem(rootItem);
        albumItem->setText(0, albumName);
        albumItem->setIcon(0, QIcon(":/icon/album.png")); // 替换成你的相册图标
        albumItem->setData(0, Qt::UserRole, "album"); // 标记为相册节点
        albumItem->setData(0, Qt::UserRole+1, albumName); // 存储相册名
    }
}

// -------------------------- 右键菜单槽函数 --------------------------
// 新建相册
void MainWindow::on_actionNewAlbum_triggered()
{
    bool ok;
    QString albumName = QInputDialog::getText(
        this,
        "新建相册",
        "请输入相册名称：",
        QLineEdit::Normal,
        "",
        &ok
    );

    if(ok && !albumName.isEmpty()) {
        if(!AlbumProject::getInstance().addAlbum(albumName)) {
            QMessageBox::warning(this, "错误", "创建相册失败！\n可能是名称重复或权限不足");
        }
    }
}

// 删除相册
void MainWindow::on_actionDeleteAlbum_triggered()
{
    // 获取当前选中的节点
    QTreeWidgetItem* currentItem = ui->treeWidgetAlbum->currentItem();
    if(!currentItem) return;

    // 只能删除相册节点，不能删除根节点
    QString nodeType = currentItem->data(0, Qt::UserRole).toString();
    if(nodeType != "album") {
        QMessageBox::information(this, "提示", "只能删除相册节点");
        return;
    }

    QString albumName = currentItem->data(0, Qt::UserRole+1).toString();
    int ret = QMessageBox::question(
        this,
        "确认删除",
        QString("确定要删除相册「%1」吗？\n所有图片将被永久删除！").arg(albumName),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );

    if(ret == QMessageBox::Yes) {
        AlbumProject::getInstance().deleteAlbum(albumName);
    }
}

// -------------------------- 原有菜单槽函数 --------------------------
void MainWindow::on_action创建项目_triggered()
{
    AlbumWizard wizard(this);
    if(wizard.exec() == QDialog::Accepted) {
        QString name = wizard.field("projectName").toString();
        QString path = wizard.field("projectPath").toString();

        if(AlbumProject::getInstance().createProject(name, path)) {
            loadProject();
        }
    }
}

void MainWindow::on_action打开项目_triggered()
{
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "打开电子相册项目",
        QDir::currentPath(),
        "电子相册项目 (*.album)"
    );

    if(!filePath.isEmpty()) {
        if(AlbumProject::getInstance().loadProject(filePath)) {
            loadProject();
        }
    }
}
```

---

## 三、第三步：准备图标资源（可选但推荐）
在你的资源文件`rc.qrc`中添加两个图标：
- `:/icon/folder.png`：项目根节点图标
- `:/icon/album.png`：相册节点图标

如果没有图标，可以先注释掉`setIcon`那两行，不影响功能运行。

---

## 四、测试验证（第三步完成标准）
1.  ✅ 新建项目后，左侧目录树自动显示「项目名 → 默认相册」
2.  ✅ 右键点击目录树空白处或节点，弹出「新建相册/删除相册」菜单
3.  ✅ 点击「新建相册」，输入名称后，目录树自动新增节点，同时磁盘项目目录下生成对应文件夹
4.  ✅ 右键点击相册节点，点击「删除相册」，确认后节点消失，磁盘文件夹被删除
5.  ✅ 关闭程序重新打开项目，所有相册节点正确恢复
6.  ✅ 所有数据自动保存到`.album`配置文件中

---

## ✅ 架构延续性说明
这一步完全遵循了之前的分层架构：
- **数据层**：只负责数据存储和磁盘操作，发信号通知UI
- **控制层**：只负责接收用户操作、调用数据层、更新UI
- **视图层**：纯展示，没有任何业务逻辑

为下一步做好了完美铺垫：
- 第四步做右侧缩略图：只需要监听`treeWidgetAlbum`的`itemClicked`信号，获取选中的相册名，调用`AlbumProject::getAlbumPath()`获取路径，遍历图片生成缩略图即可
- 所有数据都在单例中，任何模块都能随时获取，不用传参
