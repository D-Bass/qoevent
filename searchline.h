#ifndef SEARCHLINE_H
#define SEARCHLINE_H
#include <QLineEdit>
#include <QCompleter>
#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QKeyEvent>
#include <QDebug>

class SearchLine : public QLineEdit
{
  Q_OBJECT
public:
  //query should include 0-rowid and 1-text fields
  SearchLine(QAbstractItemModel *m,QAbstractItemView *v, const int column, QWidget* parent=0);
public slots:
  void popup() {
    clear();
    raise();
    show();
    setFocus();
  }
protected:
  void focusOutEvent(QFocusEvent *) {hide();}
  void keyPressEvent(QKeyEvent *event)
  {
    if (event->key()==Qt::Key_Escape) {hide();event->accept();}
    else QLineEdit::keyPressEvent(event);
  }
private:
  QAbstractItemView* view;
  QAbstractItemModel *model;
  QCompleter* comp;
private slots:
  void activated(const QString& text) {
    view->setCurrentIndex(model->match(comp->completionModel()->index(0,2),Qt::DisplayRole,text).at(0));
    view->setFocus();
    hide();
  }
};

#endif // SEARCHLINE_H
