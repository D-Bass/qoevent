#include "courses.h"
#include "uisettings.h"
#include <QDateTime>
#include <QMouseEvent>
#include <QDebug>
#include "displaytime.h"





bool CourseEditor::validate()
{
  QPalette p(pal);
  p.setColor(QPalette::Base,QColor(Qt::red));
  QSqlQuery q;
  vlist=text().split(QRegExp("[),()]{1,2}"),QString::SkipEmptyParts);
  for (QString& s : vlist)
    {
      q.exec(QString("select cp_id from cp where race_id==%1 and map_alias=='%2'")
         .arg(ui_settings.current_race)
         .arg(s));
      if(!q.last())
        {
          ui_settings.statusbar_message_notify(tr("Error: no existant controls"),3000);
          setPalette(p);
          return false;
        }
    }

  q.exec("create temp table if not exists crs (cp_id INTEGER,ord INTEGER,value INTEGER,penalty INTEGER,cutoff INTEGER)");
  q.exec("delete from crs");
  QString str(text());
  QString name;
  QChar c;
  int ord=1,i,str_size=str.size();
  bool inside_group=0;
  bool res;
  QSqlDatabase db=QSqlDatabase::database();
  db.transaction();
  for(i=0;i<=str_size;i++)
    {
      if(i<str_size) c=str.at(i);
      if(c==',' || i==str_size)
        {
          res=q.exec(QString("insert into crs (cp_id,value,penalty,cutoff,ord) select cpid,coalesce(course_data.value,cpval),coalesce(course_data.penalty,cppen),coalesce(course_data.cutoff,0),%1 from (select cp_id as cpid,map_alias as malias,value as cpval,penalty as cppen from cp where race_id==%2) left join course_data on course_data.cp_id==cpid and course_data.course_id==%3 where malias=='%4' limit 1")
                 .arg(ord)
                 .arg(ui_settings.current_race)
                 .arg(course_id)
                 .arg(name));

          name.clear();
          if(!inside_group) ++ord;
          continue;
        }
      if(c=='(') { inside_group=1; continue; }
      if(c==')') { inside_group=0; continue; }
      name.append(c);
    }
  q.exec(QString("delete from course_data where course_id==%1").arg(course_id));
  q.exec(QString("insert into course_data (course_id,cp_id,ord,value,penalty,cutoff) select %1,cp_id,ord,value,penalty,cutoff from crs").arg(course_id));
  q.exec("drop table crs");
  db.commit();
  return true;
}


void CourseEditor::keyPressEvent(QKeyEvent* event)
{
  if(event->key()!=Qt::Key_Return)
    setPalette(pal);
  QLineEdit::keyPressEvent(event);
}


CourseDelegate::CourseDelegate(QObject* parent):
  QStyledItemDelegate(parent)
{}

QWidget* CourseDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{

  qint64 crs_rowid;
  const OSqlTableModel* mdl=static_cast<const OSqlTableModel*>(index.model());
  crs_rowid=mdl->getRowID(index);
  return new CourseEditor(crs_rowid,parent);
}

void CourseDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
  static_cast<QLineEdit*>(editor)->setText(index.model()->data(index,Qt::EditRole).toString());
}


void CourseDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
  editor->setGeometry(option.rect);
}


bool CourseDelegate::eventFilter(QObject *editor, QEvent *event)
{
  if (event->type()==QEvent::KeyPress)
  {
    QKeyEvent* ev=static_cast<QKeyEvent*>(event);
    int key=ev->key();
    if(key==Qt::Key_Down || key==Qt::Key_Up) return true;
    if(key==Qt::Key_Return)
      {
        if(static_cast<CourseEditor*>(editor)->validate())
          {
            commitData(static_cast<QWidget*>(editor));
            closeEditor(static_cast<QWidget*>(editor));
          }
        return true;
      }

  }
  return QStyledItemDelegate::eventFilter(editor,event);
}








//====================== COURSES ===========================//





CoursesModel::CoursesModel(QObject* parent)
  :OSqlTableModel(12,12,"delete from course where course_id==%1",parent)
{
  select();
  seq_txt=new QString;
}

void CoursesModel::selectSoft()
{
  query->exec(QString("select name, 0, 0 , soft_order, overtime, overtime_penalty, min_penalty, max_penalty,min_score,value_is_time,penalty_is_time, max_punch_interval,course_id from course where race_id==%1").arg(ui_settings.current_race));
}


QVariant CoursesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if(orientation==Qt::Horizontal)
  {
    if (role==Qt::DisplayRole)
      return header_name.at(section);

    if (role==Qt::ToolTipRole)
        return header_tootip.at(section);
  }

  return QVariant();
}

QVariant CoursesModel::data(const QModelIndex &index, int role) const
{
  QSqlQuery q;
  int col=index.column();
  if(!index.isValid()) return QVariant();
  query->seek(index.row());
  if(role==Qt::DisplayRole || role==Qt::EditRole)
  {
    qint64 val;
    QDateTime d;
    switch(col)
    {
      case 0: return query->value(0).toString();
      case 1:
        seq_txt->clear();
        q.exec(QString("select group_concat(cp.map_alias) ,count(ord) from course_data join cp using(cp_id) where course_id==%1  group by ord order by course_data.ord")
               .arg(query->value(rowid_column).toLongLong()));
        while(q.next())
        {
          if(q.value(1).toInt()==1)
          {
            seq_txt->append(q.value(0).toString());\
            seq_txt->append(",");
          }
          else
          {
            seq_txt->append("(");
            seq_txt->append(q.value(0).toString());
            seq_txt->append("),");
          }
        }
        if(!seq_txt->isEmpty()) seq_txt->truncate(seq_txt->size()-1);
        return *seq_txt;
      case 2: return distance(query->value(rowid_column).toLongLong());
      case 4:
        val=query->value(4).toLongLong();
        if(!val) return QString();
        if(val<86400*7) return DisplayTime::hour_min_sec(val);
        d=QDateTime::fromTime_t(val);
        return d.toString("dd.MM.yyyy HH:mm");
      case 5:
      case 6:
      case 7:
      case 8:
      case 11:
        return query->value(col).toInt();
    }
  }
  if(role==Qt::CheckStateRole)
    if(col==3 || col==9 || col==10)
      return query->value(col).toInt();
  return QVariant();
}

Qt::ItemFlags CoursesModel::flags(const QModelIndex &index) const
{
  Q_ASSERT(index.isValid());
  int col=index.column();
  if (col==3 || col==9 || col==10)
    return Qt::ItemIsEnabled |Qt::ItemIsEditable | Qt::ItemIsUserCheckable;
  if (index.column()!=2)
    return Qt::ItemIsEnabled |Qt::ItemIsEditable | Qt::ItemIsSelectable;
  else return Qt::ItemIsEnabled;
}


bool CoursesModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
  bool res=true;
  int row=index.row();int column=index.column();
  if(role!=Qt::EditRole && role!=Qt::CheckStateRole) return false;
  query->seek(row);
  last_edited_rowid=query->value(rowid_column).toInt();
  last_edited_column=column;
  QString val=value.toString();
  QString db_cmd=
      QString("update course set %1=").arg(db_field.at(column)) +
      QString("%1") + QString(" where course_id==%1").arg(last_edited_rowid);


  if(column==4 && val!="NULL")
    {
      QRegExp overtime_personal("^[0-4]?[0-9]:[0-5][0-9]$");
      QRegExp overtime_common("^[0-3][0-9]\\.[0-1][0-9]\\.[0-9]{4} [0-2][0-9]:[0-5][0-9]");
      if (overtime_personal.exactMatch(val) )
      {
        int hour=val.left(val.indexOf(':')).toInt();
        int min=val.mid(val.indexOf(':')+1).toInt();
        db_cmd=db_cmd.arg(60*(hour*60+min));
      }
      if(overtime_common.exactMatch(value.toString()))
      {
        db_cmd=db_cmd.arg(
              QDateTime::fromString(val,"dd.MM.yyyy HH:mm")
              .toMSecsSinceEpoch()/1000);
      }
    }
  else
    {
      if(column<2)
        db_cmd=db_cmd.arg("'%1'");
      db_cmd=db_cmd.arg(val);
    }

  res=query->exec(db_cmd);
  selectSoft();
  if(res || column==1) dataChanged(createIndex(row,0,nullptr),createIndex(row,column_count-1,nullptr));
  return res;
}


bool CoursesModel::insertRow(const QString text)
{
  bool res=query->exec(QString("insert into course(name,race_id) values('%1',%2)")
                       .arg(text)
                       .arg(ui_settings.current_race));
  notifyInsertedRow(res);
  return true;
}


QVariant CoursesModel::distance(qint64 rowid) const
{
  QSqlQuery q;
  q.exec(QString("select cp_id from course_data where course_id==%1 group by ord order by ord")
         .arg(rowid));
  q.last();
  if(q.at()<1) return QVariant();
  int distance=0,d;
  qint64 p1,p2;
  q.first();
  p2=q.value(0).toLongLong();
  while(q.next())
  {
    p1=p2;p2=q.value(0).toLongLong();
    d=static_cast<CPTab*>(tabCP)->distance(p1,p2);
    if(d) distance+=d; else return QVariant();
  }
  return QVariant(distance);
}







//=============================== PRICES =====================================//




PricesModel::PricesModel(QObject *parent) : QAbstractTableModel(parent)
{
  query=new QSqlQuery;
}


void PricesModel::selectSoft(const qint64 course)
{
  current_course=course;
  query->exec(QString("select cp.map_alias,course_data.value,course_data.penalty,cutoff,course_data.rowid from course_data join cp using(cp_id) where course_data.course_id==%1 order by course_data.ord")
              .arg(course));
}

QVariant PricesModel::data(const QModelIndex &index, int role) const
{
  int row=index.row(),column=index.column();
  if(!query->seek(row)) return QVariant();

  if(role==Qt::DisplayRole || role==Qt::EditRole)
      if(column<=2)
        return query->value(column).toString();
  if(role==Qt::CheckStateRole && (column>=3))
      return query->value(column).toInt();

  return QVariant();
}


int PricesModel::rowCount(const QModelIndex &parent) const
{
  if(parent.isValid() || !query->last()) return 0;
  return query->at()+1;
}



QVariant PricesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if(orientation!=Qt::Horizontal) return QVariant();
  if(role==Qt::DisplayRole)
    return header_name.at(section);
  if(role==Qt::ToolTipRole)
    return header_tootip.at(section);
  return QVariant();
}


Qt::ItemFlags PricesModel::flags(const QModelIndex &index) const
{
  if(index.column()==0)
    return Qt::ItemIsEnabled;
  if(index.column()<=2)
    return Qt::ItemIsEnabled|Qt::ItemIsEditable;
  else
    return Qt::ItemIsEnabled|Qt::ItemIsEditable|Qt::ItemIsUserCheckable;
}

bool PricesModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
  if(!index.isValid()) return false;
  int row=index.row();int column=index.column();
  bool res;
  if(role==Qt::EditRole || role==Qt::CheckStateRole)
    {
      query->seek(row);
      res=query->exec(QString("update course_data set %1=%2 where rowid==%3")
                        .arg(db_field.at(column))
                        .arg(value.toString())
                        .arg(query->value(4).toLongLong()));
    }
  selectSoft(current_course);
  if(res) dataChanged(createIndex(row,column,nullptr),createIndex(row,column,nullptr));
  return res;
}






