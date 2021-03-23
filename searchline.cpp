#include "searchline.h"

SearchLine::SearchLine(QAbstractItemModel *m,QAbstractItemView *v, const int column, QWidget* parent)
  :QLineEdit(parent),model(m),view(v)
{
  comp=new QCompleter(m,this);
  comp->setCaseSensitivity(Qt::CaseInsensitive);
  comp->setCompletionColumn(column);
  comp->setCompletionRole(Qt::DisplayRole);
  setCompleter(comp);
  resize(250,30);
  connect(comp,SIGNAL(activated(const QString&)),SLOT(activated(const QString&)));
  hide();
}
