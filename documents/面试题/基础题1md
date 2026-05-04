# Qt电子相册项目 基础面试题（教学版）
共5道题，循序渐进，全部来自电子相册真实代码，只考察Qt核心基础

---

## 第1题 最基础：显示一张本地图片
[基础问题]
你有一个QLabel控件`label_photo`，请写代码将本地路径`"C:/test.jpg"`的图片显示在这个QLabel上，要求图片保持宽高比不变形。

[考察点]
- QLabel显示图片的基本方法
- QPixmap加载本地文件
- 图片缩放与宽高比保持

[学习建议]
- 官方文档：
  - QLabel：https://doc.qt.io/qt-5/qlabel.html#setPixmap
  - QPixmap：https://doc.qt.io/qt-5/qpixmap.html#scaled
- 示例代码：
```cpp
// 1. 加载图片
QPixmap pixmap("C:/test.jpg");
// 2. 缩放图片：保持宽高比，平滑缩放
QPixmap scaledPixmap = pixmap.scaled(
    label_photo->size(),
    Qt::KeepAspectRatio, // 核心：保持宽高比
    Qt::SmoothTransformation // 平滑缩放，更清晰
);
// 3. 显示到QLabel
label_photo->setPixmap(scaledPixmap);
// 4. 让图片在QLabel中居中显示
label_photo->setAlignment(Qt::AlignCenter);
```

[避坑提示]
**初学者最常见错误**：忘记保持宽高比，直接写`label_photo->setPixmap(pixmap)`，导致图片被拉伸变形；或者使用相对路径时，运行时工作目录不是源码目录，导致QPixmap加载失败（显示空白）。

---

## 第2题 基础交互：点击按钮切换下一张图片
[基础问题]
你有一个QPushButton按钮`btn_next`和一个QLabel`label_photo`，还有一个QStringList`photoList`存储了所有图片路径，以及一个int变量`currentIndex`记录当前显示的图片索引。请写代码实现：点击按钮，切换显示下一张图片。

[考察点]
- QPushButton的`clicked()`信号
- Qt5函数指针式信号槽连接写法
- 数组越界判断

[学习建议]
- 官方文档：https://doc.qt.io/qt-5/signalsandslots.html
- 示例代码：
```cpp
// 在MainWindow构造函数中连接信号槽
connect(btn_next, &QPushButton::clicked, this, &MainWindow::onNextClicked);

// 槽函数实现
void MainWindow::onNextClicked()
{
    // 1. 边界判断：最后一张时回到第一张
    currentIndex++;
    if(currentIndex >= photoList.size()) {
        currentIndex = 0;
    }
    // 2. 加载并显示下一张图片
    QPixmap pixmap(photoList[currentIndex]);
    label_photo->setPixmap(pixmap.scaled(label_photo->size(), Qt::KeepAspectRatio));
}
```

[避坑提示]
**初学者最常见错误**：使用Qt4旧的字符串式写法`connect(btn_next, SIGNAL(clicked()), this, SLOT(onNextClicked()))`，这种写法没有编译时检查，拼写错误不会报错，只会在运行时提示"信号槽连接失败"；另外忘记做边界判断，会导致数组越界崩溃。

---

## 第3题 文件操作：遍历文件夹获取所有图片
[基础问题]
请写一个函数，输入一个文件夹路径，返回该文件夹下所有jpg、png格式的图片的完整路径列表。

[考察点]
- QDir遍历文件夹
- QFileInfo判断文件类型
- 文件后缀过滤

[学习建议]
- 官方文档：https://doc.qt.io/qt-5/qdir.html#entryInfoList
- 示例代码：
```cpp
QStringList getImageFiles(const QString& folderPath)
{
    QStringList imageList;
    QDir dir(folderPath);

    // 只获取文件，过滤掉.和..目录
    QFileInfoList fileList = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);

    foreach(const QFileInfo& fileInfo, fileList) {
        // 获取文件后缀，转小写
        QString suffix = fileInfo.suffix().toLower();
        // 过滤图片格式
        if(suffix == "jpg" || suffix == "jpeg" || suffix == "png") {
            // 获取完整路径
            imageList.append(fileInfo.absoluteFilePath());
        }
    }

    return imageList;
}
```

[避坑提示]
**初学者最常见错误**：忘记加`QDir::NoDotAndDotDot`，导致遍历结果包含`.`和`..`两个特殊目录；另外不要用字符串拼接路径（如`folderPath + "/" + fileName`），应该用`dir.filePath(fileName)`，Qt会自动处理不同操作系统的路径分隔符。

---

## 第4题 界面布局：搭建简单的图片查看器
[基础问题]
请用代码实现一个简单的图片查看器布局：窗口上半部分是一个QLabel用于显示图片，下半部分是两个按钮"上一张"和"下一张"，两个按钮水平排列。

[考察点]
- QVBoxLayout垂直布局
- QHBoxLayout水平布局
- 布局嵌套
- 布局与窗口的关联

[学习建议]
- 官方文档：https://doc.qt.io/qt-5/layout.html
- 示例代码：
```cpp
// 在MainWindow构造函数中
// 1. 创建主垂直布局
QVBoxLayout* mainLayout = new QVBoxLayout(this);

// 2. 创建图片显示的QLabel
QLabel* label_photo = new QLabel(this);
label_photo->setAlignment(Qt::AlignCenter);
label_photo->setStyleSheet("background-color: black;");
mainLayout->addWidget(label_photo, 1); // 第二个参数是拉伸因子，1表示占满剩余空间

// 3. 创建按钮水平布局
QHBoxLayout* btnLayout = new QHBoxLayout();
QPushButton* btn_prev = new QPushButton("上一张", this);
QPushButton* btn_next = new QPushButton("下一张", this);
btnLayout->addWidget(btn_prev);
btnLayout->addWidget(btn_next);

// 4. 将按钮布局添加到主布局
mainLayout->addLayout(btnLayout);

// 5. 设置窗口的布局
setLayout(mainLayout);
```

[避坑提示]
**初学者最常见错误**：忘记给布局设置父对象（如`new QVBoxLayout()`而不是`new QVBoxLayout(this)`），导致布局不生效；或者不用布局，直接用`move()`和`resize()`设置控件位置，导致窗口缩放时控件错位。

---

## 第5题 内存管理：QPixmap的正确使用
[基础问题]
下面这段代码是一个新手写的加载缩略图的函数，运行时发现导入100张图片后，程序内存占用飙升。请指出问题并修改。
```cpp
// 错误代码
void MainWindow::loadThumbnails()
{
    for(int i=0; i<photoList.size(); i++) {
        // 加载原始大图
        QPixmap* original = new QPixmap(photoList[i]);
        // 生成缩略图
        QPixmap thumbnail = original->scaled(150, 150, Qt::KeepAspectRatio);
        // 添加到列表
        QListWidgetItem* item = new QListWidgetItem();
        item->setIcon(QIcon(thumbnail));
        listWidget->addItem(item);
    }
}
```

[考察点]
- QPixmap的内存管理
- 堆对象的释放
- Qt父子对象机制

[学习建议]
- 官方文档：https://doc.qt.io/qt-5/objecttrees.html
- 修正后的代码：
```cpp
// 正确代码
void MainWindow::loadThumbnails()
{
    for(int i=0; i<photoList.size(); i++) {
        // 用栈对象代替堆对象，函数结束自动释放
        QPixmap original(photoList[i]);
        QPixmap thumbnail = original.scaled(150, 150, Qt::KeepAspectRatio);
        
        QListWidgetItem* item = new QListWidgetItem(listWidget); // 给item设置父对象
        item->setIcon(QIcon(thumbnail));
    }
}
```

[避坑提示]
**初学者最常见错误**：没有必要地用`new`创建QPixmap堆对象，又忘记`delete`，导致内存泄漏；另外QListWidgetItem必须设置父对象为listWidget，否则listWidget销毁时不会自动释放item，也会导致内存泄漏。
