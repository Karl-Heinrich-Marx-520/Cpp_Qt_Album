#include "confirmpage.h"
#include "ui_confirmpage.h"

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

void ConfirmPage::initializePage()
{
    QString name = field("projectName").toString();
    QString path = field("projectPath").toString();
    QString fullPath = QDir(path).filePath(name);
    ui->label_info->setText(QString("项目名称：%1\n保存路径：%2").arg(name, fullPath));
}
