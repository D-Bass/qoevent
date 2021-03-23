#include "comboxeditor.h"
#include "uisettings.h"
#include "osqltable.h"

QWidget* ComboxEditor::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
  QSqlQuery q;
  QString text=query_text;
  q.exec(text.replace(QString("==%1"),QString("==%1").arg(ui_settings.current_race)));
  QComboBox* editor=new QComboBox(parent);
  while(q.next())
    editor->addItem(q.value(0).toString(),q.value(1).toInt());
  editor->setCurrentIndex(editor->findData(static_cast<const OSqlTableModel*>(index.model())->getRowID(index)));
  editor->setGeometry(option.rect);
  editor->setEditable(isEditable);
  editor->showPopup();
  return editor;
}

void ComboxEditor::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
  QSqlQuery q;
  QString text=insert_query_text;
  QComboBox* e=static_cast<QComboBox*>(editor);
  if(isEditable && (e->currentData().toInt()==0))
  {
    q.exec(text.replace(QString("%1"),QString("%1").arg(e->currentText())));
    model->setData(index,q.lastInsertId());
  }
  else
  model->setData(index,e->currentData());
}
