#ifndef PROSETPAGE_H
#define PROSETPAGE_H

#include <QWizardPage>

namespace Ui {
class proSetPage;
}

class proSetPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit proSetPage(QWidget *parent = nullptr);
    ~proSetPage();
    //重写输入校验
    bool isComplete() const override;

private slots:
    void on_pushButton_browse_clicked();

private:
    Ui::proSetPage *ui;
};

#endif // PROSETPAGE_H
