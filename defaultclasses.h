#include <QEventLoop>
#include <QListWidget>
#include <QLayout>
#include <QPushButton>
#include <QStringList>
#include <QCloseEvent>
#include <QDialog>

class QListWidgetItemEditable : public QListWidgetItem
{
public:
  QListWidgetItemEditable(const QString& text) :
  QListWidgetItem(text)
  {
    setFlags(flags() | Qt::ItemIsEditable);
  }

};

class DefaultClasses : public QDialog
{
  Q_OBJECT

public:
  DefaultClasses(QWidget* parent=0);
  ~DefaultClasses();
  QListWidget* table;

private slots:
  void submit();
  void editHelper(QListWidgetItem* item);
private:
  QVBoxLayout* layout;
  QPushButton* bOk,*bCancel;
  QStringList* list;
protected:
  virtual void keyPressEvent(QKeyEvent *event);
};


void edit_default_classes();
bool load_default_classes();
