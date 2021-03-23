#include "osqltable.h"
#include <QKeyEvent>


OSqlTableModel::OSqlTableModel(int column_cnt,qint64 rowid_col, QString del_query, QObject* parent)
  :QAbstractTableModel(parent),
    column_count(column_cnt),
    rowid_column(rowid_col),
    delete_query(del_query)
{
  query=new QSqlQuery;
}

void OSqlTableModel::select()
{
  beginResetModel();
  selectSoft();
  query_line.clear();
  int row=0;
  query->seek(-1);
  while (query->next()) query_line.insert(query->value(rowid_column).toLongLong(),row++);
  endResetModel();
  emit setCurrent(lastSelected());
}

int OSqlTableModel::columnCount(const QModelIndex &parent) const
{
  if(parent.isValid()) return 0;
  return column_count;
}

QModelIndex OSqlTableModel::index(const qint64 qline,const int column) const
{
  if(!query_line.contains(qline)) return QModelIndex();
  return QAbstractTableModel::index(query_line.value(qline),column);
}


qint64 OSqlTableModel::getRowID(const QModelIndex& index) const
{
  if(query->seek(index.row())) return query->value(rowid_column).toLongLong();
  else return 0;
}

int OSqlTableModel::rowCount(const QModelIndex &parent) const
{
  if(parent.isValid() || !query->last()) return 0;
  return query->at()+1;
}


QModelIndex OSqlTableModel::lastInserted() const
{
  QVariant id=query->lastInsertId();
  query->seek(-1);
  while(query->next())
    if(id==query->value(rowid_column)) return createIndex(query->at(),0,nullptr);
  return QModelIndex();
}


void OSqlTableModel::notifyInsertedRow(bool success)
{
  selectSoft();
  if(success)
  {
    qint64 last_inserted=query->lastInsertId().toLongLong();
    beginInsertRows(QModelIndex(),last_inserted,last_inserted);
    endInsertRows();
  }
}

QModelIndex OSqlTableModel::lastEdited() const
{

//  query->seek(-1);
//  while(query->next())
//    if(query->value(rowid_column).toLongLong()==last_edited_rowid)
//      return QAbstractTableModel::index(query->at(),last_edited_column,QModelIndex());
      //return createIndex(query->at(),last_edited_column,nullptr);

  return index(last_edited_rowid,last_edited_column);
  return QModelIndex();
}

QModelIndex OSqlTableModel::lastSelected() const
{
  if(last_selected_column==-1 || last_selected_rowid==-1) return QModelIndex();
  return index(last_selected_rowid,last_selected_column);
}

void OSqlTableModel::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
  if(!query->seek(current.row())) return;
  last_selected_rowid=query->value(rowid_column).toLongLong();
  last_selected_column=current.column();
}



bool OSqlTableModel::removeRow(int row)
{
  if(!query->seek(row)) return false;
  bool res=query->exec(delete_query
              .arg(query->value(rowid_column).toString()));

  selectSoft();
  if(res) {
    beginRemoveRows(QModelIndex(),row,row);
    endRemoveRows();
  }
  return true;
}










OTableView::OTableView(QWidget* parent)
  : QTableView(parent),validator(nullptr)
{
  setTabKeyNavigation(false);
  setTabOrder(this,this);
}

OTableView::~OTableView()
{
  if(nedit!=nullptr)
  delete nedit;
  if(validator!=nullptr)
    delete validator;
}

void OTableView::setModel(QAbstractItemModel *model)
{
  QTableView::setModel(model);
  nedit=new NewEntryEdit(static_cast<OSqlTableModel*>(model),this,this);
  nedit->setGeometry(0,20,300,30);
  nedit->setFont(QFont("tahoma",10,QFont::Bold));
  nedit->hide();
  connect(model,SIGNAL(setCurrent(QModelIndex)),SLOT(setCurrentIndex(QModelIndex)));
}


void OTableView::setValidator(const QString regex)
{
  validator=new QRegExpValidator(QRegExp(regex),this);
  nedit->setValidator(validator);
}


void OTableView::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
{
  QTableView::dataChanged(topLeft,bottomRight,roles);
  setCurrentIndex(static_cast<OSqlTableModel*>(model())->lastEdited());
}


void NewEntryEdit::enterNew()
{
  clear();
  setFocus();
  show();
  raise();
}

void NewEntryEdit::keyPressEvent(QKeyEvent *ev)
{
  if(ev->key()==Qt::Key_Return)  {
    if(text().isEmpty()) return;
    static_cast<OSqlTableModel*>(model)->insertRow(text());
    hide();
    static_cast<QWidget*>(this->parent())->setFocus();
    view->scrollTo(model->lastInserted());
    view->setCurrentIndex(model->lastInserted());
    return;
  }

  if(ev->key()==Qt::Key_Escape) {
    hide();static_cast<QWidget*>(this->parent())->setFocus();return;
  }

  QLineEdit::keyPressEvent(ev);
}

