#include "albumproject.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QJsonArray>

// 初始化支持的图片格式
QStringList AlbumProject::s_supportedImageFormats = {"jpg", "jpeg", "png", "bmp", "gif", "webp"};

//单例实现
AlbumProject& AlbumProject::getInstance()
{
    static AlbumProject instance;
    return instance;
}
AlbumProject::AlbumProject(QObject *parent) : QObject(parent) {}

//------------------------------项目基础操作-----------------------------------
//创建并初始化一个新相册项目的核心入口
bool AlbumProject::createProject(const QString &projectName, const QString &projectPath)
{
    //拼接项目完整路径
    QString fullPath = QDir(projectPath).filePath(projectName);
    QDir dir;
    //创建项目根目录
    if(!dir.mkpath(fullPath)){
        qDebug() << "创建项目目录失败：" << fullPath;
        return false;
    }
    //初始化项目数据
    m_projectName = projectName;
    m_projectRootPath = fullPath;
    m_projectFilePath = QDir(fullPath).filePath(projectName + ".album"); //保存为.album配置文件

    // 创建项目时默认生成一个"默认相册"
    addAlbum("默认相册");
    return saveProject();
}

//保存项目到JSON文件
bool AlbumProject::saveProject()
{
    QFile file(m_projectFilePath);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text)){
        qDebug() << "打开项目文件失败:" << m_projectFilePath;
        return false;
    }
    //构建JSON对象
    QJsonObject obj;
    obj["projectName"] = m_projectName;
    obj["projectRootPath"] = m_projectRootPath;
    obj["albumList"] = QJsonArray::fromStringList(m_albumList);
    //写入文件
    QJsonDocument doc(obj);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    qDebug() << "项目保存成功" << m_projectFilePath;
    return true;
}

//加载已有项目
bool AlbumProject::loadProject(const QString &projectFilePath)
{
    QFile file(projectFilePath);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)){
        qDebug() << "打开项目文件失败" << projectFilePath;
        return false;
    }
    //解析JSON
    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if(doc.isNull()){
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
    qDebug() << "项目加载成功" << m_projectName;
    return true;
}
//----------------------------获取项目信息-------------------------------
QString AlbumProject::getProjectName() const {return m_projectName;}

QString AlbumProject::getProjectRootPath() const {return m_projectRootPath;}

QString AlbumProject::getProjectFilePath() const {return m_projectFilePath;}

// -------------------------- 相册操作 --------------------------
// 新建相册：同时在内存和磁盘创建
bool AlbumProject::addAlbum(const QString &albumName)
{
    //校验
    if(albumName.trimmed().isEmpty()){
        qDebug() << "相册名不能为空";
        return false;
    }
    if(m_albumList.contains(albumName)){
        qDebug() << "相册已经存在" << albumName;
        return false;
    }
    //在磁盘中创建文件夹
    QString albumPath = QDir(m_projectRootPath).filePath(albumName);
    QDir dir;
    if(!dir.mkpath(albumPath)){
        qDebug() << "创建相册文件夹失败：" << albumPath;
        return false;
    }
    //内存中添加相册
    m_albumList.append(albumName);
    //保存项目配置
    if(!saveProject()) {
        qDebug() << "保存项目失败";
        return false;
    }
    //发送信号，通知UI刷新
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

    // 1. 删除磁盘文件夹
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

// -------------------------- 图片操作 --------------------------
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

bool AlbumProject::importPhotos(const QString& albumName, const QStringList& photoPaths)
{
    // 1. 校验相册是否存在
    if(!m_albumList.contains(albumName)) {
        qDebug() << "相册不存在：" << albumName;
        return false;
    }

    QString albumPath = getAlbumPath(albumName);
    QDir dir(albumPath);
    if(!dir.exists()) {
        qDebug() << "相册路径不存在：" << albumPath;
        return false;
    }

    int successCount = 0;
    for(const QString& photoPath : photoPaths) {
        // 2. 过滤非图片文件
        if(!isImageFile(photoPath)) {
            qDebug() << "跳过非图片文件：" << photoPath;
            continue;
        }

        QFileInfo fileInfo(photoPath);
        QString fileName = fileInfo.fileName();
        QString destPath = dir.filePath(fileName);

        // 3. 处理重名文件：自动重命名（如 image.jpg → image_1.jpg）
        int i = 1;
        while(QFile::exists(destPath)) {
            QString baseName = fileInfo.completeBaseName();
            QString suffix = fileInfo.suffix();
            fileName = QString("%1_%2.%3").arg(baseName, QString::number(i), suffix);
            destPath = dir.filePath(fileName);
            i++;
        }

        // 4. 复制文件到相册目录
        if(QFile::copy(photoPath, destPath)) {
            successCount++;
            qDebug() << "导入成功：" << destPath;
        } else {
            qDebug() << "导入失败：" << photoPath;
        }
    }

    qDebug() << "导入完成，成功" << successCount << "张";
    return successCount > 0;
}











