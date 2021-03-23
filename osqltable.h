#ifndef OSQLTABLE_H
#define OSQLTABLE_H

#include <QAbstractTableModel>
#include <QTableView>
#include <QSqlQuery>
#include <QValidator>
#include <QCompleter>
#include <QLineEdit>


class OSqlTableModel : public QAbstractTableModel
{
  Q_OBJECT
public:
  OSqlTableModel(int column_cnt,qint64 rowid_col, QString del_query, QObject* parent=0);
  ~OSqlTableModel() {delete query;}
  virtual bool removeRow(int row);
  virtual bool insertRow(const QString text)=0;
  QModelIndex index(const qint64 qline, const int column) const;
  int rowCount(const QModelIndex &parent) const;
  int columnCount(const QModelIndex &parent) const;
  QModelIndex lastEdited() const;
  QModelIndex lastInserted() const;
  QModelIndex lastSelected() const;
  virtual qint64 getRowID(const QModelIndex &index) const;

public slots:
  void select();
  virtual void setSortColumn(int col) {sort_column=col;select();}
protected:
  void notifyInsertedRow(bool success);
  virtual void selectSoft()=0;
  qint64 last_edited_rowid;
  int last_edited_column;
  qint64 last_selected_rowid=-1;
  int last_selected_column=-1;
  int column_count;
  int rowid_column;
  int sort_column=-1;
  int sort_clause_num=0;
  QSqlQuery* query;
  QHash<qint64,int> query_line;
protected slots:
  void currentChanged(const QModelIndex &current, const QModelIndex &previous);
private:
  QString delete_query;
signals:
  void setCurrent(QModelIndex idx);
};




class NewEntryEdit : public QLineEdit
{
public:
  NewEntryEdit(OSqlTableModel *m,QTableView* v, QWidget* parent=0)
    : QLineEdit(parent),model(m),view(v) {}
  void enterNew();
protected:
  void focusOutEvent(QFocusEvent *) {hide();}
  void keyPressEvent(QKeyEvent *ev);
private:
  OSqlTableModel* model;
  QTableView* view;
};




class OTableView: public QTableView
{
  Q_OBJECT
public:
  OTableView(QWidget* parent=0);
  ~OTableView();
  void enterNewEntry() {nedit->enterNew();}
  void setValidator(const QString regex);
  void setTextHint(const QString text) {if(nedit) nedit->setPlaceholderText(text);}
  void setCompleter(QCompleter* comp) {if(nedit) nedit->setCompleter(comp);}
  void setModel(QAbstractItemModel *model);
protected slots:
  void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles);
protected:
  NewEntryEdit* nedit=nullptr;
  QValidator* validator;

};





#endif // OSQLTABLE_H
