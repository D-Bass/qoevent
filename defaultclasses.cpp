#include "defaultclasses.h"
#include "database.h"
#include <QDebug>
#include <QKeyEvent>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlDatabase>
#include "uisettings.h"

DefaultClasses::DefaultClasses(QWidget *parent)
  :QDialog(parent)

{
  table=new QListWidget;
  layout=new QVBoxLayout(this);
  bOk=new QPushButton(tr("Submit"));
  bCancel=new QPushButton(tr("Cancel"));
  list=new QStringList();
  list->reserve(20);
  layout->addWidget(table);
  layout->addWidget(bOk);
  layout->addWidget(bCancel);
  QSqlQuery q("SELECT list FROM class_default");
  q.first();
  QString qres(q.record().value(0).toString());
  *list=qres.split(',',QString::SkipEmptyParts);
  for(QStringList::iterator l=list->begin();l!=list->end();l++) table->addItem(new QListWidgetItemEditable(*l));
  table->addItem("");
  setLayout(layout);
  setModal(true);
  setWindowTitle(tr("Default classes"));
  setWindowIcon(QIcon(":/micon.gif"));
  resize(400,250);
  connect( bOk,SIGNAL(clicked()),SLOT(submit()) );
  connect( bCancel,SIGNAL(clicked()),SLOT(reject()) );
  connect(table,SIGNAL(itemActivated(QListWidgetItem*)),SLOT(editHelper(QListWidgetItem*)));
  qDebug() << this->children();
  table->setDragDropMode(QAbstractItemView::InternalMove);
}


DefaultClasses::~DefaultClasses()
{
  delete list;
  qDebug() << "decl dtor";
}


void DefaultClasses::keyPressEvent(QKeyEvent *event)
{


  if(event->key()==Qt::Key_Insert)
    {
      QListWidgetItem* item=new QListWidgetItemEditable("");
      event->accept();
      table->insertItem(table->currentRow(),item);
      qDebug() << table->currentRow();
      table->setCurrentRow(table->row(item));
      table->editItem(item);
    }
  if(event->key()==Qt::Key_Delete)
    {
      event->accept();
      qDebug() << "Del";
      if(table->currentItem()->flags() & Qt::ItemIsEditable)
      table->takeItem(table->currentRow());
    }
}

void DefaultClasses::editHelper(QListWidgetItem *item)
{
  QListWidgetItemEditable* new_item;
  if (!(item->flags() & Qt::ItemIsEditable))
    {
      new_item=new QListWidgetItemEditable("");
      table->insertItem(table->currentRow(),new_item);
      item=new_item;
    }
  table->setCurrentRow(table->row(item));
  table->editItem(item);
}

void DefaultClasses::submit()
{
  QStringList list;
  QString t;
  for(int i=0;i<table->count()-1;i++)
  {
    t=table->item(i)->text();
    if(t!="" && !list.contains(t))
      list << t;
  }
  qDebug() << list;
  QSqlQuery(QString("UPDATE class_default SET list=\"%1\"").arg(list.join(','))).exec();
  accept();
}




void edit_default_classes()
{
  qDebug() << "defclass";
  DefaultClasses dc;
  dc.exec();
  qDebug() << "defclass closed";
}

bool load_default_classes()
{
  QSqlQuery q;
  QSqlDatabase db=QSqlDatabase::database();
  QString str;
  QStringList list;
  if(!q.exec("select list from class_default"))return false;
  q.first();
  str=q.value(0).toString();
  list=str.split(',',QString::SkipEmptyParts);
  db.transaction();
  for( auto& s : list)
    q.exec(QString("insert into class(race_id,name) values(%1,'%2')")
        .arg(ui_settings.current_race)
        .arg(s));
  db.commit();
  ui_settings.classes_change_notify();
  return true;
}
