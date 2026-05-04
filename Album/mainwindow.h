#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMenu>
#include <QTreeWidgetItem>
#include <QListWidgetItem>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;
    //加载项目并初始化UI
    void loadProject();

private slots:
    void on_actioncreate_triggered();

    void on_actionopen_triggered();

    void on_actionNewAlbum_triggered();

    void on_actionDeleteAlbum_triggered();

    void refreshTreeWidget();

    void onTreeItemSelected(QTreeWidgetItem* item, int column);// 目录树选中事件槽函数

    void refreshPhotoList(const QString& albumName);// 刷新右侧缩略图列表

    void onPhotoItemDoubleClicked(QListWidgetItem* item); // 双击缩略图槽函数

    void on_actionImportPhotos_triggered(); // 导入图片槽函数

    void on_actionSlideShow_triggered(); // 幻灯片播放槽函数

private:
    Ui::MainWindow *ui;
    QMenu* m_treeContextMenu; // 目录树右键菜单
    //初始化左侧目录树
    void initTreeWidget();
    //初始化右键菜单
    void initContextMenu();
    //初始化右侧缩略图列表
    void initPhotoListWidget();
};
#endif // MAINWINDOW_H
