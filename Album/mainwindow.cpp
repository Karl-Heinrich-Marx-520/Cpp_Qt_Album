#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "albumwizard.h"
#include "albumproject.h"
#include "picshow.h"
#include "SlideShowWidget.h"

#include <QFileDialog>
#include <QTreeWidgetItem>
#include <QInputDialog>
#include <QMessageBox>
#include <QDebug>
#include <QPixmap>
#include <QListWidgetItem>
#include <QScreen>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("电子相册");

    initContextMenu();// 初始化右键菜单
    initPhotoListWidget(); // 初始化右侧缩略图列表
//-----------------------------链接信号---------------------------------
    // 监听数据层的相册变化信号，自动刷新目录树
    connect(&AlbumProject::getInstance(), &AlbumProject::albumListChanged,
            this, &MainWindow::refreshTreeWidget);

    // 连接目录树选中事件
    connect(ui->treeWidgetAlbum, &QTreeWidget::itemSelectionChanged,
            this, [=](){
                QList<QTreeWidgetItem*> selectedItems = ui->treeWidgetAlbum->selectedItems();
                if(!selectedItems.isEmpty()) {
                    onTreeItemSelected(selectedItems.first(), 0);
                }
            });

    // 连接缩略图双击信号
    connect(ui->listWidgetAlbum, &QListWidget::itemDoubleClicked,
            this, &MainWindow::onPhotoItemDoubleClicked);
}

MainWindow::~MainWindow()
{
    delete ui;
}
//-----------------------缩略图-----------------------------------
// 初始化右侧缩略图列表
void MainWindow::initPhotoListWidget()
{
    // 设置为图标模式
    ui->listWidgetAlbum->setViewMode(QListWidget::IconMode);
    // 设置图标大小（150x150像素，可根据需要调整）
    ui->listWidgetAlbum->setIconSize(QSize(150, 150));
    // 设置网格间距
    ui->listWidgetAlbum->setSpacing(10);
    // 设置为可调整大小，自动换行
    ui->listWidgetAlbum->setResizeMode(QListWidget::Adjust);
    // 设置选择模式：单选
    ui->listWidgetAlbum->setSelectionMode(QAbstractItemView::SingleSelection);

    // 设置缩略图列表支持右键菜单
    ui->listWidgetAlbum->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->listWidgetAlbum, &QListWidget::customContextMenuRequested,
            this, [=](const QPoint& pos){
                QMenu menu(this);
                QAction* actSlideShow = menu.addAction("幻灯片播放");
                connect(actSlideShow, &QAction::triggered, this, &MainWindow::on_actionSlideShow_triggered);
                menu.exec(ui->listWidgetAlbum->mapToGlobal(pos));
            });
}

// 刷新右侧缩略图列表
void MainWindow::refreshPhotoList(const QString& albumName)
{
    // 清空之前的缩略图
    ui->listWidgetAlbum->clear();

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
            ui->listWidgetAlbum->iconSize(),
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
        ui->listWidgetAlbum->addItem(item);
    }

    qDebug() << "缩略图刷新完成，共" << photoList.size() << "张";
}

//双击缩略图展示大图
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
    Picshow* picShow = new Picshow(this);
    picShow->setAttribute(Qt::WA_DeleteOnClose); // 关闭时自动释放内存
    picShow->setPhotoList(photoList, currentIndex);

    // 5.设置窗口大小为屏幕的 80%，居中显示
    QScreen* screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->availableGeometry();
    int windowW = screenGeometry.width() * 0.6;
    int windowH = screenGeometry.height() * 0.6;
    picShow->resize(windowW, windowH);

    // 6.居中显示
    int x = (screenGeometry.width() - windowW) / 2;
    int y = (screenGeometry.height() - windowH) / 2;
    picShow->move(x, y);
    picShow->show();
    picShow->raise();
    picShow->activateWindow();
}

// -------------------------- 幻灯片播放槽函数 --------------------------
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
    QListWidgetItem* currentItem = ui->listWidgetAlbum->currentItem();
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

//-------------------------------目录树------------------------
//初始化目录树
void MainWindow::initTreeWidget()
{
    //创建根节点(项目名)
    QTreeWidgetItem* rootItem = new QTreeWidgetItem(ui->treeWidgetAlbum);
    rootItem->setText(0, AlbumProject::getInstance().getProjectName());
    rootItem->setIcon(0, QIcon(":/icon/pic.png"));
    // 遍历所有相册，创建子节点
    refreshTreeWidget();
    //默认展开根节点
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
    QTreeWidgetItem* firstAlbumItem = nullptr; // 记录第一个相册（默认相册）
    for(const QString& albumName : albumList) {
        QTreeWidgetItem* albumItem = new QTreeWidgetItem(rootItem);
        albumItem->setText(0, albumName);
        albumItem->setIcon(0, QIcon(":/icon/bulm.ico"));
        albumItem->setData(0, Qt::UserRole, "album"); // 标记为相册节点
        albumItem->setData(0, Qt::UserRole+1, albumName); // 存储相册名
        // 记录第一个相册节点（就是默认相册）
        if(!firstAlbumItem) {
            firstAlbumItem = albumItem;
        }
    }

    // 核心：自动选中第一个相册（默认相册），触发右侧缩略图刷新
    if(firstAlbumItem) {
        ui->treeWidgetAlbum->setCurrentItem(firstAlbumItem);
        // 手动触发选中事件，直接进入相册
        onTreeItemSelected(firstAlbumItem, 0);
    }

}

// 目录树选中事件处理
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
        ui->listWidgetAlbum->clear();
    }
}
// -------------------------- 初始化右键菜单槽 --------------------------

// 初始化右键菜单
void MainWindow::initContextMenu()
{
    // 创建菜单
    m_treeContextMenu = new QMenu(this);
    QAction* actNewAlbum = m_treeContextMenu->addAction("新建相册");
    QAction* actImportPhotos = m_treeContextMenu->addAction("导入图片");
    QAction* actDeleteAlbum = m_treeContextMenu->addAction("删除相册");

    // 绑定槽函数
    connect(actNewAlbum, &QAction::triggered, this, &MainWindow::on_actionNewAlbum_triggered);
    connect(actImportPhotos, &QAction::triggered, this, &MainWindow::on_actionImportPhotos_triggered);
    connect(actDeleteAlbum, &QAction::triggered, this, &MainWindow::on_actionDeleteAlbum_triggered);

    // 设置目录树支持右键菜单
    ui->treeWidgetAlbum->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->treeWidgetAlbum, &QTreeWidget::customContextMenuRequested,
            this, [=](const QPoint& pos){
                // 在鼠标点击位置弹出菜单
                m_treeContextMenu->exec(ui->treeWidgetAlbum->mapToGlobal(pos));
            });
}

//加载项目并初始化所有UI
void MainWindow::loadProject()
{
    //清空之前的UI
    ui->treeWidgetAlbum->clear();
    ui->listWidgetAlbum->clear();
    //初始化左侧目录树
    initTreeWidget();
    //更新窗口标题
    setWindowTitle(QString("电子相册 - %1").arg(AlbumProject::getInstance().getProjectName()));
    qDebug() << "项目加载完成，UI已初始化";
}

// 新建相册
void MainWindow::on_actionNewAlbum_triggered(){
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

//------------------------------------右键菜单槽函数-------------------------------
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

void MainWindow::on_actionImportPhotos_triggered()
{
    // 1. 校验：必须选中一个相册节点
    QTreeWidgetItem* treeItem = ui->treeWidgetAlbum->currentItem();
    if(!treeItem) {
        QMessageBox::information(this, "提示", "请先选择一个相册");
        return;
    }
    QString nodeType = treeItem->data(0, Qt::UserRole).toString();
    if(nodeType != "album") {
        QMessageBox::information(this, "提示", "请先选择一个相册节点");
        return;
    }
    QString albumName = treeItem->data(0, Qt::UserRole+1).toString();

    // 2. 弹出文件选择对话框，支持多选图片
    QStringList photoPaths = QFileDialog::getOpenFileNames(
        this,
        "选择要导入的图片",
        QDir::homePath(),
        "图片文件 (*.jpg *.jpeg *.png *.bmp *.gif *.webp);;所有文件 (*.*)"
        );

    if(photoPaths.isEmpty()) {
        return; // 用户点击取消，直接返回
    }

    // 3. 调用数据层导入图片
    if(AlbumProject::getInstance().importPhotos(albumName, photoPaths)) {
        // 4. 导入成功：自动刷新右侧缩略图
        refreshPhotoList(albumName);
        QMessageBox::information(this, "成功", "图片导入成功！");
    } else {
        QMessageBox::warning(this, "错误", "图片导入失败！\n请检查文件是否被占用或权限是否足够");
    }
}

// -------------------------- 顶菜单槽函数 --------------------------
//点击"打开项目"
void MainWindow::on_actionopen_triggered()
{
    QString filePath = QFileDialog::getOpenFileName(this, "打开电子相册项目", QDir::currentPath(), "电子相册项目(*.album)");
    if(!filePath.isEmpty()){
        if(AlbumProject::getInstance().loadProject(filePath)){
            loadProject();
        }
    }
}

//点击"创建项目按钮"
void MainWindow::on_actioncreate_triggered()
{
    Albumwizard wizard(this);
    if(wizard.exec() == QDialog::Accepted){
        //从向导获得用户输入
        QString name = wizard.field("projectName").toString();
        QString path = wizard.field("projectpath").toString();
        //调用数据层创建项目
        if(AlbumProject::getInstance().createProject(name, path)){
            loadProject();
        }
    }
}








