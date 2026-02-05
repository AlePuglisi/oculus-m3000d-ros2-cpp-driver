#ifndef MULTISELECTFORM_H
#define MULTISELECTFORM_H

#include <QDialog>

namespace Ui {
class MultiSelectForm;
}

class MultiSelectForm : public QDialog
{
    Q_OBJECT

public:
    explicit MultiSelectForm(QWidget *parent = 0);
    ~MultiSelectForm();

private:
    Ui::MultiSelectForm *ui;
};

#endif // MULTISELECTFORM_H
