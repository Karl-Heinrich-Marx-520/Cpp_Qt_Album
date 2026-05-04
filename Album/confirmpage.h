#ifndef CONFIRMPAGE_H
#define CONFIRMPAGE_H

#include <QWizardPage>
#include <QDir>

namespace Ui {
class ConfirmPage;
}

class ConfirmPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit ConfirmPage(QWidget *parent = nullptr);
    ~ConfirmPage();
    //页面显示时更新信息
    void initializePage() override;

private:
    Ui::ConfirmPage *ui;
};

#endif // CONFIRMPAGE_H
