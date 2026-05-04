#ifndef ALBUMWIZARD_H
#define ALBUMWIZARD_H
#include <QWizard>

class Albumwizard : public QWizard
{
    Q_OBJECT
public:
    explicit Albumwizard(QWidget* parent = nullptr);
    ~Albumwizard() override = default;
    //枚举页面ID
    enum Pageld{
        Page_Set,
        Page_Confirm
    };
};

#endif // ALBUMWIZARD_H
