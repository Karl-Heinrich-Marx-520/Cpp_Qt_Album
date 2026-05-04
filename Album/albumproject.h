#ifndef ALBUMPROJECT_H
#define ALBUMPROJECT_H
#include <QObject>
#include <QString>
#include <QStringList>

//数据层
class AlbumProject : public QObject
{
    Q_OBJECT
public:
    //单例模式
    static AlbumProject& getInstance();
    //项目基础操作
    bool createProject(const QString& projectName, const QString& projectPath);
    bool loadProject(const QString& projectFilePath);
    bool saveProject();

    //相册操作
    bool addAlbum(const QString& albumName); // 新建相册
    bool deleteAlbum(const QString& albumName); // 删除相册
    QStringList getAlbumList() const; // 获取所有相册名
    QString getAlbumPath(const QString& albumName) const; // 获取相册的磁盘路径

    // 图片操作
    QStringList getPhotoList(const QString& albumName) const; // 获取相册内所有图片路径
    static bool isImageFile(const QString& fileName); // 判断是否为图片文件
    bool importPhotos(const QString& albumName, const QStringList& photoPaths); // 批量导入图片

    //获取项目信息
    QString getProjectName() const;
    QString getProjectRootPath() const; //项目根目录
    QString getProjectFilePath() const; //.album配置路径

signals:
    // 数据变化信号：通知UI刷新
    void albumListChanged();

private:
    explicit AlbumProject(QObject* parent = nullptr);
    ~AlbumProject() override = default;
    //禁止拷贝
    AlbumProject(const AlbumProject&) = delete;
    AlbumProject& operator=(const AlbumProject&) = delete;

    //项目数据
    QString m_projectName;
    QString m_projectRootPath;
    QString m_projectFilePath;
    QStringList m_albumList; //相册列表
    static QStringList s_supportedImageFormats; // 支持的图片格式列表
};

#endif // ALBUMPROJECT_H
