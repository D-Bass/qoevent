#ifndef COMBOXEDITOR_H
#define COMBOXEDITOR_H

#include <QComboBox>
#include <QStyledItemDelegate>



class ComboxEditor : public QStyledItemDelegate
{
  const QString query_text;
  const QString insert_query_text;
  bool isEditable;
public:
  ComboxEditor(const QString query,bool editable=false,QString insert_query="",QObject *parent=0) :
    QStyledItemDelegate(parent),
    query_text(query),
    insert_query_text(insert_query),
    isEditable(editable)  {}

  QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
  void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
};






#endif // COMBOXEDITOR_H
