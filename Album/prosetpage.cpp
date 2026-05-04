#include "prosetpage.h"
#include "ui_prosetpage.h"
#include <QFileDialog>
#include <QDir>

proSetPage::proSetPage(QWidget *parent)
    : QWizardPage(parent)
    , ui(new Ui::proSetPage)
{
    ui->setupUi(this);
    //默认填充当前路径
    ui->lineEdit_path->setText(QDir::currentPath());
    ui->lineEdit_path->setClearButtonEnabled(true);
    ui->lineEdit_name->setClearButtonEnabled(true);
    //输入变化时自动检验
    connect(ui->lineEdit_name, &QLineEdit::textEdited, this, &proSetPage::completeChanged);
    connect(ui->lineEdit_path, &QLineEdit::textEdited, this, &proSetPage::completeChanged);
    //注册向导字段，后面可以直接获取
    registerField("projectName", ui->lineEdit_name);
    registerField("projectPath", ui->lineEdit_path);
}


//浏览按钮点击
void proSetPage::on_pushButton_browse_clicked()
{
    QString path = QFileDialog::getExistingDirectory(this, "选择保存路径", QDir::currentPath());
    if(!path.isEmpty()){
        ui->lineEdit_path->setText(path);
    }
}

//输入检验
bool proSetPage::isComplete() const
{
    QString name = ui->lineEdit_name->text().trimmed();
    QString path = ui->lineEdit_path->text().trimmed();
    //非空检验
    if(name.isEmpty() || path.isEmpty()){
        ui->label_tips->setText("项目名称和路径不能为空");
        return false;
    }
    //路径存在检验
    QDir dir(path);
    if(!dir.exists()){
        ui->label_tips->setText("保存路径不存在");
        return false;
    }
    //项目不重名检验
    QString fullPath = dir.filePath(name);
    if(QDir(fullPath).exists()){
        ui->label_tips->setText("项目已存在，请修改名称或路径");
    }
    //校验通过
    ui->label_tips->clear();
    return true;
}


proSetPage::~proSetPage()
{
    delete ui;
}







