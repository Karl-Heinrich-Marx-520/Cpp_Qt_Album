# 电子相册 **第二步完整教程（严格遵循分层架构）**
## 第二步核心目标（架构先行，绝不乱堆代码）
1.  **严格遵循「数据层-视图层-控制层」分层**，所有代码按职责拆分到对应文件
2.  实现**创建项目向导**（2步：项目配置 → 确认信息），带完整输入校验
3.  实现**项目数据持久化**（生成`.album`格式的JSON配置文件）
4.  实现**打开已有项目**功能
5.  项目加载后，自动初始化第一步搭好的「左侧目录树」骨架

---

## 一、先建文件结构（架构的基础，一步到位）
在Qt Creator左侧项目树，右键项目名 → 添加新文件，按下面顺序创建：
| 文件类型 | 文件名 | 所属层级 | 职责 |
|----------|--------|----------|------|
| C++类 | `AlbumProject` | **数据层** | 专门管理项目数据，负责读写JSON配置文件 |
| Qt设计师界面类 | `ProSetPage` | 视图层 | 向导第一页：输入项目名和路径 |
| Qt设计师界面类 | `ConfirmPage` | 视图层 | 向导第二页：确认项目信息 |
| C++类 | `AlbumWizard` | 视图层 | 向导主类，整合两个页面 |

> ✅ 关键原则：**绝对不要把数据读写、业务逻辑写在MainWindow里**
> MainWindow只做「控制层」：接收用户操作 → 调用数据层 → 更新视图层

---

## 二、第一步：实现数据层 `AlbumProject`（最核心，所有模块的基础）
这个类是整个项目的数据中心，后面所有模块（目录树、缩略图、幻灯片）都要依赖它，必须先写。

### 1. `AlbumProject.h`
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
    // 单例模式（全局唯一的项目实例，所有地方都能访问）
    static AlbumProject& getInstance();

    // 创建新项目
    bool createProject(const QString& projectName, const QString& projectPath);
    // 加载已有项目
    bool loadProject(const QString& projectFilePath);
    // 保存项目
    bool saveProject();

    // 获取项目信息
    QString getProjectName() const;
    QString getProjectRootPath() const; // 项目根目录
    QString getProjectFilePath() const; // .album配置文件路径

private:
    explicit AlbumProject(QObject *parent = nullptr);
    ~AlbumProject() override = default;

    // 禁止拷贝
    AlbumProject(const AlbumProject&) = delete;
    AlbumProject& operator=(const AlbumProject&) = delete;

    // 项目数据
    QString m_projectName;
    QString m_projectRootPath;
    QString m_projectFilePath;
    QStringList m_albumList; // 相册列表（后续扩展）
};

#endif // ALBUMPROJECT_H
```

### 2. `AlbumProject.cpp`
```cpp
#include "AlbumProject.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

// 单例实现
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
    // 1. 拼接项目完整路径
    QString fullPath = QDir(projectPath).filePath(projectName);
    QDir dir;

    // 2. 创建项目根目录
    if(!dir.mkpath(fullPath)) {
        qDebug() << "创建项目目录失败：" << fullPath;
        return false;
    }

    // 3. 初始化项目数据
    m_projectName = projectName;
    m_projectRootPath = fullPath;
    m_projectFilePath = QDir(fullPath).filePath(projectName + ".album");

    // 4. 保存为.album配置文件
    return saveProject();
}

// 保存项目到JSON文件
bool AlbumProject::saveProject()
{
    QFile file(m_projectFilePath);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "打开项目文件失败：" << m_projectFilePath;
        return false;
    }

    // 构建JSON对象
    QJsonObject obj;
    obj["projectName"] = m_projectName;
    obj["projectRootPath"] = m_projectRootPath;
    obj["albumList"] = QJsonArray::fromStringList(m_albumList);

    // 写入文件
    QJsonDocument doc(obj);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    qDebug() << "项目保存成功：" << m_projectFilePath;
    return true;
}

// 加载已有项目
bool AlbumProject::loadProject(const QString& projectFilePath)
{
    QFile file(projectFilePath);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "打开项目文件失败：" << projectFilePath;
        return false;
    }

    // 解析JSON
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
    m_albumList = obj["albumList"].toVariant().toStringList();

    file.close();
    qDebug() << "项目加载成功：" << m_projectName;
    return true;
}

// 简单的getter方法
QString AlbumProject::getProjectName() const { return m_projectName; }
QString AlbumProject::getProjectRootPath() const { return m_projectRootPath; }
QString AlbumProject::getProjectFilePath() const { return m_projectFilePath; }
```

> ✅ 架构优势：
> 所有项目数据都存在这里，后面任何地方需要项目信息，只需要调用 `AlbumProject::getInstance().getXXX()` 即可，不用到处传参。

---

## 三、第二步：实现视图层「创建项目向导」
### 1. 向导第一页：`ProSetPage`（项目配置页）
和你之前写的逻辑完全一致，只是现在和数据层解耦了。

#### 1.1 UI设计（`prosetpage.ui`）
拖入控件，按下面命名：
- 标签：`label_name`（文字：项目名称：）
- 输入框：`lineEdit_name`
- 标签：`label_path`（文字：保存路径：）
- 输入框：`lineEdit_path`
- 按钮：`pushButton_browse`（文字：浏览）
- 标签：`label_tips`（文字：Tips:，用于显示错误提示）

#### 1.2 `ProSetPage.h`
```cpp
#ifndef PROSETPAGE_H
#define PROSETPAGE_H

#include <QWizardPage>

namespace Ui {
class ProSetPage;
}

class ProSetPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit ProSetPage(QWidget *parent = nullptr);
    ~ProSetPage() override;

    // 重写输入校验
    bool isComplete() const override;

private slots:
    void on_pushButton_browse_clicked();

private:
    Ui::ProSetPage *ui;
};

#endif // PROSETPAGE_H
```

#### 1.3 `ProSetPage.cpp`
```cpp
#include "ProSetPage.h"
#include "ui_ProSetPage.h"
#include <QFileDialog>
#include <QDir>

ProSetPage::ProSetPage(QWidget *parent)
    : QWizardPage(parent)
    , ui(new Ui::ProSetPage)
{
    ui->setupUi(this);

    // 默认填充当前路径
    ui->lineEdit_path->setText(QDir::currentPath());
    ui->lineEdit_path->setClearButtonEnabled(true);
    ui->lineEdit_name->setClearButtonEnabled(true);

    // 输入变化时自动校验
    connect(ui->lineEdit_name, &QLineEdit::textEdited, this, &ProSetPage::completeChanged);
    connect(ui->lineEdit_path, &QLineEdit::textEdited, this, &ProSetPage::completeChanged);

    // 注册向导字段，后面可以直接获取
    registerField("projectName", ui->lineEdit_name);
    registerField("projectPath", ui->lineEdit_path);
}

ProSetPage::~ProSetPage()
{
    delete ui;
}

// 浏览按钮点击
void ProSetPage::on_pushButton_browse_clicked()
{
    QString path = QFileDialog::getExistingDirectory(this, "选择保存路径", QDir::currentPath());
    if(!path.isEmpty()) {
        ui->lineEdit_path->setText(path);
    }
}

// 输入校验
bool ProSetPage::isComplete() const
{
    QString name = ui->lineEdit_name->text().trimmed();
    QString path = ui->lineEdit_path->text().trimmed();

    // 1. 非空校验
    if(name.isEmpty() || path.isEmpty()) {
        ui->label_tips->setText("项目名称和路径不能为空");
        return false;
    }

    // 2. 路径存在校验
    QDir dir(path);
    if(!dir.exists()) {
        ui->label_tips->setText("保存路径不存在");
        return false;
    }

    // 3. 项目不重名校验
    QString fullPath = dir.filePath(name);
    if(QDir(fullPath).exists()) {
        ui->label_tips->setText("项目已存在，请修改名称或路径");
        return false;
    }

    // 校验通过
    ui->label_tips->clear();
    return true;
}
```

### 2. 向导第二页：`ConfirmPage`（确认信息页）
#### 2.1 UI设计（`confirmpage.ui`）
拖入两个标签：
- `label_title`（文字：请确认以下信息：）
- `label_info`（用于显示项目名和路径）

#### 2.2 `ConfirmPage.h`
```cpp
#ifndef CONFIRMPAGE_H
#define CONFIRMPAGE_H

#include <QWizardPage>

namespace Ui {
class ConfirmPage;
}

class ConfirmPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit ConfirmPage(QWidget *parent = nullptr);
    ~ConfirmPage() override;

    // 页面显示时更新信息
    void initializePage() override;

private:
    Ui::ConfirmPage *ui;
};

#endif // CONFIRMPAGE_H
```

#### 2.3 `ConfirmPage.cpp`
```cpp
#include "ConfirmPage.h"
#include "ui_ConfirmPage.h"

ConfirmPage::ConfirmPage(QWidget *parent)
    : QWizardPage(parent)
    , ui(new Ui::ConfirmPage)
{
    ui->setupUi(this);
}

ConfirmPage::~ConfirmPage()
{
    delete ui;
}

// 页面显示时，从向导获取上一页的字段值
void ConfirmPage::initializePage()
{
    QString name = field("projectName").toString();
    QString path = field("projectPath").toString();
    QString fullPath = QDir(path).filePath(name);

    ui->label_info->setText(QString("项目名称：%1\n保存路径：%2").arg(name, fullPath));
}
```

### 3. 向导主类：`AlbumWizard`
```cpp
#ifndef ALBUMWIZARD_H
#define ALBUMWIZARD_H

#include <QWizard>

class AlbumWizard : public QWizard
{
    Q_OBJECT

public:
    explicit AlbumWizard(QWidget *parent = nullptr);
    ~AlbumWizard() override = default;

    // 枚举页面ID
    enum PageId {
        Page_Set,
        Page_Confirm
    };
};

#endif // ALBUMWIZARD_H
```

```cpp
#include "AlbumWizard.h"
#include "ProSetPage.h"
#include "ConfirmPage.h"

AlbumWizard::AlbumWizard(QWidget *parent)
    : QWizard(parent)
{
    setWindowTitle("创建电子相册项目");
    setWizardStyle(QWizard::ModernStyle);

    // 添加页面
    setPage(Page_Set, new ProSetPage(this));
    setPage(Page_Confirm, new ConfirmPage(this));

    // 设置向导样式（解决之前的文字隐身问题，不影响主窗口）
    setStyleSheet(R"(
        QWizard {
            background-color: #2b2b2b;
            color: white;
        }
        QWizardPage {
            background-color: #2b2b2b;
            color: white;
        }
        QLabel {
            color: white;
        }
        QLineEdit {
            background-color: #3c3c3c;
            color: white;
            border: 1px solid #555;
        }
        QPushButton {
            background-color: #444;
            color: white;
            border: 1px solid #555;
            padding: 5px 15px;
            border-radius: 3px;
        }
        QPushButton:disabled {
            background-color: #333;
            color: #888;
        }
    )");
}
```

---

## 四、第三步：实现控制层 `MainWindow`（连接所有模块）
现在把数据层和视图层连接起来，实现点击菜单弹出向导，创建/打开项目后初始化UI。

### 1. `MainWindow.h`
```cpp
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // 加载项目并初始化UI
    void loadProject();

private slots:
    void on_action创建项目_triggered();
    void on_action打开项目_triggered();

private:
    Ui::MainWindow *ui;

    // 初始化左侧目录树
    void initTreeWidget();
};

#endif // MAINWINDOW_H
```

### 2. `MainWindow.cpp`
```cpp
#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "AlbumWizard.h"
#include "AlbumProject.h"
#include <QFileDialog>
#include <QTreeWidgetItem>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("电子相册");
}

MainWindow::~MainWindow()
{
    delete ui;
}

// 点击"创建项目"菜单
void MainWindow::on_action创建项目_triggered()
{
    AlbumWizard wizard(this);
    if(wizard.exec() == QDialog::Accepted) {
        // 1. 从向导获取用户输入
        QString name = wizard.field("projectName").toString();
        QString path = wizard.field("projectPath").toString();

        // 2. 调用数据层创建项目
        if(AlbumProject::getInstance().createProject(name, path)) {
            // 3. 加载项目并初始化UI
            loadProject();
        }
    }
}

// 点击"打开项目"菜单
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

// 加载项目并初始化所有UI
void MainWindow::loadProject()
{
    // 清空之前的UI
    ui->treeWidgetAlbum->clear();
    ui->listWidgetPhoto->clear();

    // 初始化左侧目录树
    initTreeWidget();

    // 更新窗口标题
    setWindowTitle(QString("电子相册 - %1").arg(AlbumProject::getInstance().getProjectName()));

    qDebug() << "项目加载完成，UI已初始化";
}

// 初始化左侧目录树
void MainWindow::initTreeWidget()
{
    // 创建根节点（项目名）
    QTreeWidgetItem* rootItem = new QTreeWidgetItem(ui->treeWidgetAlbum);
    rootItem->setText(0, AlbumProject::getInstance().getProjectName());
    rootItem->setIcon(0, QIcon(":/icon/folder.png")); // 替换成你的图标

    // 默认展开根节点
    ui->treeWidgetAlbum->expandItem(rootItem);
    ui->treeWidgetAlbum->setCurrentItem(rootItem);
}
```

---

## 五、测试验证（第二步完成标准）
1.  运行程序，点击「文件 → 创建项目」
2.  输入项目名和路径，点击下一步，确认信息后点击完成
3.  ✅ 项目目录自动创建，生成`.album`配置文件
4.  ✅ 主窗口标题更新为项目名
5.  ✅ 左侧目录树显示项目根节点
6.  关闭程序，重新运行，点击「文件 → 打开项目」，选择刚才的`.album`文件
7.  ✅ 项目成功加载，目录树和窗口标题正确显示

---

## ✅ 架构总结（为什么这么写？）
| 层级 | 职责 | 优点 |
|------|------|------|
| **数据层 AlbumProject** | 只做数据读写和存储 | 数据统一管理，修改项目格式只改这一个类 |
| **视图层 AlbumWizard/ProSetPage** | 只做UI展示和输入 | UI和逻辑分离，修改样式不影响业务 |
| **控制层 MainWindow** | 只做连接和调度 | 代码简洁，不会变成"上帝类" |

这样的架构为后面所有模块打好了基础：
- 第三步做相册增删改查：只需要在`AlbumProject`里加相册数据，在`initTreeWidget`里加子节点
- 第四步做缩略图：只需要监听目录树选中事件，加载对应相册的图片
- 所有地方需要项目信息，都直接调用单例，不用传参
