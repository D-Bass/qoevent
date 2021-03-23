#include "classes.h"
#include <QDebug>
#include <typeinfo>
#include <uisettings.h>
#include <QHeaderView>
#include <QMouseEvent>
#include "comboxeditor.h"
#include <QRegExp>
#include <QDateTime>


ClassesModel::ClassesModel(QObject* parent)
  :OSqlTableModel(1,1,"delete from class where class_id==%1",parent)
{
  select();
}

void ClassesModel::selectSoft()
{
  query->exec(QString("select name,class_id from class where race_id==%1 order by name").arg(ui_settings.current_race));
}

QVariant ClassesModel::data(const QModelIndex &index, int role) const
{
  if(index.isValid()) {
    query->seek(index.row());
    if(role==Qt::DisplayRole || role==Qt::EditRole)
      return query->value(0);
  }
  return QVariant();
}

QVariant ClassesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if(orientation==Qt::Horizontal && role==Qt::DisplayRole)
    if(section==0)return QVariant(tr("Name"));
  return QVariant();
}

Qt::ItemFlags ClassesModel::flags(const QModelIndex &index) const
{
  if(index.parent().isValid()) return 0;
  return Qt::ItemIsEnabled |Qt::ItemIsEditable|Qt::ItemIsSelectable;
}

bool ClassesModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
  if(role!=Qt::EditRole) return false;
  query->seek(index.row());
  last_edited_rowid=query->value(rowid_column).toInt();
  bool res=query->exec(QString("update class set name='%1' where class_id==%2")
              .arg(value.toString())
              .arg(query->value(rowid_column).toString()));
  selectSoft();
  if(res)
    {
      dataChanged(index,index);
      ui_settings.classes_change_notify();
    }
  return true;
}



bool ClassesModel::insertRow(const QString text)
{
  bool res=query->exec(QString("insert into class(name,race_id) values('%1',%2)")
                       .arg(text)
                       .arg(ui_settings.current_race));
  notifyInsertedRow(res);
  return true;
}









//========================== CLASSES / COURSES ======================//







CCModel::CCModel(QObject *parent)
  : OSqlTableModel(4,4,"delete from clscrs where clscrs_id==%1",parent)
{
  select();
}

void CCModel::selectSoft()
{
  //with max(lap) grouped by class
  //select classname,discipline.name,clscrs.lap,course.name,clscrs.clscrs_id,classid,discipline.disc_id,laps from (select class_id as classid,name as classname from class where class.race_id==2) left join clscrs on clscrs.class_id==classid left join course on course.course_id==clscrs.course_id join discipline on discipline.disc_id==clscrs.disc_id left join (select clscrs.class_id as clsclassid, max(lap) as laps from clscrs group by clscrs.class) on classid==clsclassid order by classname,clscrs.lap
  query->exec(QString("select classname,discipline.name,clscrs.lap,course.name,clscrs.clscrs_id,classid,discipline.disc_id from (select class_id as classid,name as classname from class where class.race_id==%1) left join clscrs on clscrs.class_id==classid left join course using (course_id) join discipline using (disc_id) order by classname,clscrs.lap")
              .arg(ui_settings.current_race));
}


QVariant CCModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if(role!=Qt::DisplayRole || orientation!=Qt::Horizontal) return QVariant();
  switch(section)
  {
  case 0: return QString(tr("Class"));
  case 1: return QString(tr("Discipline"));
  case 2: return QString(tr("Lap"));
  case 3: return QString(tr("Course"));
  }
  return QVariant();
}

QVariant CCModel::data(const QModelIndex &index, int role) const
{
  if(role==Qt::DisplayRole || role==Qt::EditRole) {
    query->seek(index.row());
      return query->value(index.column()).toString();
  }
  return QVariant();
}

Qt::ItemFlags CCModel::flags(const QModelIndex &index) const
{
  query->seek(index.row());
  int v=query->value(6).toInt();
  int column=index.column();
  if(v!=relay && v!=single && column==3 || v!=relay && column==2)
    return Qt::ItemIsEnabled;
  else
    return Qt::ItemIsEnabled|Qt::ItemIsEditable | Qt::ItemIsSelectable;
}



bool CCModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
  if(!index.isValid()) return false;
  int row=index.row();int column=index.column();
  if(role!=Qt::EditRole) return false;
  bool res=1;
  query->seek(row);
  last_edited_rowid=query->value(rowid_column).toInt();
  last_edited_column=column;
  QString db_cmd=QString("update clscrs set %1=%2 where clscrs_id==%3")
      .arg(db_field.at(column))
      .arg(value.toString())
      .arg(last_edited_rowid);

  if(column==1 && value.toInt()!=single && value.toInt()!=relay)
    res=query->exec(QString("update clscrs set lap=NULL where clscrs_id==%1").arg(last_edited_rowid));
  res&=query->exec(db_cmd);
  selectSoft();
  if(res) dataChanged(createIndex(row,0,nullptr),createIndex(row,3,nullptr));
  return true;
}


bool CCModel::insertRow(qint64 cls, qint64 crs)
{
  if(!cls) return false;
  bool res;
  if(!crs) res=query->exec(
        QString("insert into clscrs(class_id,disc_id,course_id) values(%1,2,NULL)")
        .arg(cls));
  else
    res=query->exec(
           QString("insert into clscrs(class_id,disc_id,course_id) values(%1,1,%2)")
          .arg(cls).arg(crs));
  notifyInsertedRow(res);
  return false;
}






//=============================== TAB =====================================//








ComboxEditor class_selector(QString("select name,class_id from class where class.race_id==%1"));
ComboxEditor discipline_selector(QString("select name,disc_id from discipline"));
ComboxEditor course_selector(QString("select name,course_id from course where course.race_id==%1"));
CourseDelegate course_editor;

ClassesTab::ClassesTab(QWidget *parent) : QWidget(parent)
{
  clsview=new OTableView;
  clsmodel=new ClassesModel(this);
  clsview->setModel(clsmodel);
  crsview=new CoursesView;

  crsmodel=new CoursesModel(this);
  crsview->setModel(crsmodel);
  crsview->setItemDelegateForColumn(1,&course_editor);

  ccview=new OTableView;
  ccmodel=new CCModel(this);
  ccview->setModel(ccmodel);
  ccview->setItemDelegateForColumn(0,&class_selector);
  ccview->setItemDelegateForColumn(1,&discipline_selector);
  ccview->setItemDelegateForColumn(3,&course_selector);

  prview=new PricesView;
  prmodel=new PricesModel(this);
  prview->setModel(prmodel);
  prview->setEditTriggers(QAbstractItemView::AnyKeyPressed);
  prview->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  prview->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  for(QTableView *v : QList<QTableView*>{ccview,crsview,clsview})
  {
    v->setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::AnyKeyPressed);
    v->setSelectionMode(QAbstractItemView::SingleSelection);
    v->horizontalHeader()->setStretchLastSection(true);
    v->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  }
  lClass=new QGridLayout;
  bClassAdd=new QPushButton(QIcon(":/icons/add-team.png"),"");
  bClassRemove=new QPushButton(QIcon(":/icons/remove-team.png"),"");
  bClassAdd->setIconSize((QSize(20,20)));
  bClassRemove->setIconSize((QSize(20,20)));
  bClassAdd->setToolTip(tr("Add class"));
  bClassRemove->setToolTip(tr("Remove class"));
  lClass->addWidget(clsview,0,0,1,3);
  lClass->addWidget(bClassAdd,1,0,1,1,Qt::AlignLeft);
  lClass->addWidget(bClassRemove,1,1,1,1,Qt::AlignLeft);
  lClass->setColumnStretch(1,1);
  lClass->setColumnStretch(2,5);



  lCC=new QGridLayout;
  bCCAdd=new QPushButton(QIcon(":/icons/add-cc"),"");
  bCCRemove =new QPushButton(QIcon(":/icons/remove"),"");
  bCCAdd->setIconSize(QSize(80,20));
  bCCRemove->setIconSize(QSize(32,20));
  bCCAdd->setToolTip(tr("Link class and course"));
  bCCRemove->setToolTip(tr("UnLink class/course"));

  lCC->addWidget(ccview,0,0,1,3);
  lCC->setColumnStretch(2,5);
  lCC->addWidget(bCCAdd,1,0,1,1,Qt::AlignLeft);
  lCC->addWidget(bCCRemove,1,1,1,1,Qt::AlignLeft);


  lGrp=new QHBoxLayout;
  lGrp->addLayout(lClass,1);
  lGrp->addLayout(lCC,5);

  QVector<int>crsW{80,400,50,50,100,65,50,50,50,50,50};
  lCourse=new QGridLayout;
  bCourseAdd=new QPushButton(QIcon(":/icons/add-course.png"),"");
  bCourseRemove=new QPushButton(QIcon(":/icons/remove-course.png"),"");
  bCourseAdd->setIconSize((QSize(24,24)));
  for(int i=0;i<crsW.size();i++) crsview->horizontalHeader()->resizeSection(i,crsW.at(i));
  bCourseRemove->setIconSize((QSize(24,24)));
  bCourseAdd->setToolTip(tr("Add course"));
  bCourseRemove->setToolTip(tr("Remove course"));
  lCourse->addWidget(bCourseAdd,1,0,1,1,Qt::AlignRight);
  lCourse->addWidget(bCourseRemove,2,0,1,1,Qt::AlignRight);
  lCourse->addWidget(crsview,0,1,3,1);
  lCourse->setColumnStretch(0,1);
  lCourse->setColumnStretch(1,50);

  lTab=new QGridLayout;
  lTab->addLayout(lGrp,0,0);
  lTab->addLayout(lCourse,1,0);
  lTab->addWidget(prview,0,1,2,1);
  lTab->setRowStretch(0,4);
  lTab->setRowStretch(1,3);
  lTab->setColumnStretch(0,3);
  lTab->setColumnStretch(1,1);
  setLayout(lTab);
  for(QLayout* x : QList<QLayout*>{lClass,lCourse,lCC,lGrp,lTab})
  {
    x->setSpacing(5);
    x->setContentsMargins(1,1,1,1);
  }
  disable_buttons(true);
  connect(&ui_settings,SIGNAL(race_changed()),SLOT(race_changed()));
  connect(&ui_settings,SIGNAL(classes_changed()),SLOT(classes_update()));
  connect(&ui_settings,SIGNAL(courses_changed()),SLOT(courses_update()));
  connect(&ui_settings,SIGNAL(clscrs_changed()),SLOT(clscrs_update()));
  connect(ccview,SIGNAL(activated(QModelIndex)),ccview,SLOT(edit(QModelIndex)));
  connect(clsview,SIGNAL(activated(QModelIndex)),clsview,SLOT(edit(QModelIndex)));
  connect(crsview,SIGNAL(activated(QModelIndex)),crsview,SLOT(edit(QModelIndex)));
  connect(prview,SIGNAL(activated(QModelIndex)),prview,SLOT(edit(QModelIndex)));
  connect(bClassAdd,SIGNAL(clicked()),SLOT(add_class()));
  connect(bClassRemove,SIGNAL(clicked()),SLOT(remove_class()));
  connect(bCourseAdd,SIGNAL(clicked()),SLOT(add_course()));
  connect(bCourseRemove,SIGNAL(clicked()),SLOT(remove_course()));
  connect(bCCAdd,SIGNAL(clicked()),SLOT(add_cc()));
  connect(bCCRemove,SIGNAL(clicked()),SLOT(remove_cc()));
  connect(clsmodel,SIGNAL(dataChanged(QModelIndex,QModelIndex,QVector<int>)),SLOT(clscrs_update()));
  connect(crsview->selectionModel(),SIGNAL(currentChanged(QModelIndex,QModelIndex)),SLOT(course_selection_changed()));
  connect(crsmodel,SIGNAL(dataChanged(QModelIndex,QModelIndex,QVector<int>)),SLOT(course_data_changed(QModelIndex,QModelIndex,QVector<int>)));
}


void ClassesTab::disable_buttons(bool d)
{
  for(QPushButton* x : QList<QPushButton*>{bClassAdd,bClassRemove,bCourseAdd,
                  bCourseRemove,bCCAdd,bCCRemove})
      x->setDisabled(d);
}

void ClassesTab::add_class()
{
  clsview->enterNewEntry();
  ui_settings.classes_change_notify();
}

void ClassesTab::remove_class()
{
  if(!clsview->currentIndex().isValid()) return;
  clsmodel->removeRow(clsview->currentIndex().row());
  ui_settings.classes_change_notify();
}

void ClassesTab::race_changed()
{
  if(ui_settings.current_race) {
    disable_buttons(false);
    clsmodel->select();
    crsmodel->select();
    ccmodel->select();
    ccview->resizeRowsToContents();
  }
  else
    disable_buttons(true);
}

void ClassesTab::keyPressEvent(QKeyEvent *event)
{

  if(event->key()==Qt::Key_Delete && crsview->hasFocus())
  {
    int col=crsview->currentIndex().column();
    if(col==4) crsmodel->setData(crsview->currentIndex(),"NULL",Qt::EditRole);
    if(col>4)crsmodel->setData(crsview->currentIndex(),"NULL",Qt::EditRole);
  }
  else
    QWidget::keyPressEvent(event);
}




void ClassesTab::add_course()
{
  crsview->enterNewEntry();
}

void ClassesTab::remove_course()
{
  if(!crsview->currentIndex().isValid()) return;
  crsmodel->removeRow(crsview->currentIndex().row());
}


void ClassesTab::add_cc()
{
  ccmodel->insertRow(clsmodel->getRowID(clsview->currentIndex()),
                     crsmodel->getRowID(crsview->currentIndex()));
}

void ClassesTab::remove_cc()
{
  if(!ccview->currentIndex().isValid()) return;
  ccmodel->removeRow(ccview->currentIndex().row());
}


void ClassesTab::course_selection_changed()
{
  prmodel->select(crsmodel->getRowID(crsview->currentIndex()));
}


void ClassesTab::course_data_changed(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
{
  prmodel->select(crsmodel->getRowID(topLeft));
  clscrs_update();
}
