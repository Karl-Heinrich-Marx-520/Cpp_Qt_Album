#include "albumwizard.h"
#include "prosetpage.h"
#include "confirmpage.h"


Albumwizard::Albumwizard(QWidget* parent) : QWizard(parent) {
    setWindowTitle("创建电子相册项目");
    setWizardStyle(QWizard::ModernStyle);
    //添加页面
    setPage(Page_Set, new proSetPage(this));
    setPage(Page_Confirm, new ConfirmPage(this));
    //设置向导样式(文字隐身)
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
