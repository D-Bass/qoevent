#include "finish.h"
#include "protocol.h"
#include "uisettings.h"
#include <QHeaderView>
#include "split_editor.h"
#include <QFileDialog>
#include <QFile>
#include <QDateTime>
#include <QSqlError>
#include <QMouseEvent>
#include <QDebug>
#include <QContextMenuEvent>
#include <QMessageBox>
#include <QInputDialog>
#include "displaytime.h"
#include "controlpoints.h"
#include <QProcess>
extern "C" {
#include "crc.h"
}
#include "disciplines.h"
#include "comm.h"
#include "spool.h"


FinishModel::FinishModel(QObject* parent)
  : OSqlTableModel(12,12,"delete from split where prot_id==%1",parent)
{
  sort_clause_num=1;
  select();
}

QVector<qint64>place_order;

void FinishModel::selectSoft()
{
  if (!ui_settings.current_race) return;
  QString sort_clause;
  query->exec(QString("select sort_order from race where race_id==%1").arg(ui_settings.current_race));
  query->next();
  if(query->value(0).toInt()==0)
    sort_clause=ui_settings.account_teams ? "score desc,team_time,team_id" : "score desc,total_time";
  else
    sort_clause="total_time,score desc";
  query->exec(QString("with t as (select class_id,count(class_id) as cnt_class from class join team using(class_id) join team_member using(team_id) join protocol using(tm_id) where start_time is not null group by class_id), x as (select team_id,sum(total_time) as team_time from protocol join team_member using (tm_id) group by team_id) select class.class_id,score,total_time,prot_id,t.cnt_class,x.team_time,team_id from protocol join team_member using(tm_id) join team using (team_id) join class using(class_id) join t using(class_id) join x using(team_id) where race_id==%1 and start_time not null group by prot_id order by class.class_id,score is NULL or score<0,"+sort_clause)
              .arg(ui_settings.current_race));
  group_place.clear();
  place_order.clear();
  int group=-1,place;
  qint64 team=-1;
  while(query->next())
    {
      if(query->value(0).toInt()!=group)
        {
          group=query->value(0).toInt();
          place=0;
        }
      if (query->value(1).toInt()<0)
        {
          place_order.push_back(query->value(3).toLongLong());
          group_place.insert(query->value(3).toLongLong(),QString("DQ(%1)")
                             .arg(query->value(4).toString()));
        }
      else
        {
          if(ui_settings.account_teams)
            {
              if (query->value(6).toLongLong()!=team)
                {
                  team=query->value(6).toLongLong();
                  ++place;
                }
            }
          else
            ++place;
          place_order.push_back(query->value(3).toLongLong());
          group_place.insert(query->value(3).toLongLong(),QString("%1(%2)")
                             .arg(place)
                             .arg(query->value(4).toString()));
        }
    }
  if(sort_column>=0 && sort_column<=4 ||
     sort_column>=8 && sort_column<=10) sort_clause_num=sort_column;
  switch(sort_clause_num)
    {
    case 0: sort_clause=" order by 1";break;
    case 1: sort_clause=" order by split.rowid";break;
    case 2: sort_clause=" order by 3";break;
    case 3: sort_clause=" order by 4,3";break;
    case 4: sort_clause=" order by 5,3";break;
    case 8: sort_clause=ui_settings.account_teams ? " order by 9 desc,4,10" : " order by 9 desc,10";
      break;
    case 9: sort_clause=" order by 10";break;
    case 10: sort_clause=" order by 11";break;
    }

  query->exec(QString("with t as (select min(total_time) as min_tot_time,class_id from protocol join team_member using(tm_id) join competitor using(comp_id) join team using(team_id) where score>=0 group by class_id),x as (select team_id,sum(total_time) as team_time from protocol join team_member using(tm_id) group by team_id) select  start_time,max(time), competitor.name||' '||birthdate, team.name,class.name,lap,max(time),0,score,total_time,total_time-t.min_tot_time,count(split.time)-1,prot_id,class_id,chip,start_time_forced,start_time_selector,competitor.name,birthdate,finish_time,team_id,team_time from protocol left join split using(prot_id) join team_member using(tm_id) join competitor using(comp_id) join team using(team_id) join class using(class_id) left join t using(class_id) join x using(team_id) where class.race_id==%1" + filter_clause + " group by prot_id having count(time)!=0" + sort_clause)
              .arg(ui_settings.current_race));
}



QVariant FinishModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if(role!=Qt::DisplayRole || orientation!=Qt::Horizontal) return QVariant();
  switch(section)
    {
    case 0: return QString(tr("Start"));
    case 1: return QString(tr("Finish"));
    case 2: return QString(tr("Name"));
    case 3: return QString(tr("Team"));
    case 4: return QString(tr("Class"));
    case 5: return QString(tr("Lap"));
    case 6: return QString(tr("Race\ntime"));
    case 7: return QString(tr("Place"));
    case 8: return QString(tr("Total\nscore"));
    case 9: return QString(tr("Total\ntime"));
    case 10: return QString(tr("Lag from\nleader"));
    case 11: return QString(tr("Control taken"));
    }
  return QVariant();
}

QVariant FinishModel::data(const QModelIndex &index, int role) const
{
  int row=index.row();int column=index.column();
  query->seek(row);
  if(role==Qt::DisplayRole || role==Qt::EditRole)
    {
      qint64 start_time=
          query->value( query->value(16).toInt()==1 && query->value(15).toInt()>0 ? 15 : 0  )
          .toLongLong();
      qint64 finish_time=(query->value(19).toInt() ? query->value(19).toInt() : query->value(0).toLongLong()+query->value(1).toLongLong());
      int32_t val;
      switch (column)
        {
        case 0:
          return QDateTime::fromMSecsSinceEpoch(start_time*1000).toString("hh:mm:ss");
        case 1:
          if(role==Qt::DisplayRole)
            return QDateTime::fromMSecsSinceEpoch(finish_time*1000).toString("hh:mm:ss");
          else
            return QDateTime::fromMSecsSinceEpoch(finish_time*1000).toString("hh:mm:ss dd.MM.yyyy");
        case 6:
          if(start_time>finish_time) return QString(tr("Inv. Time"));
          val=finish_time-start_time;
          return DisplayTime::hour_min_sec(val);
        case 7:
            return group_place.value(query->value(rowid_column).toLongLong());
        case 9:
        case 10:
          val=query->value(column).toInt();
          if(val<0) return QVariant();
          return DisplayTime::hour_min_sec(val);
        case 12:
          //coefficient of leader's time
          return QString("%1")
              .arg((query->value(9).toFloat()-query->value(10).toFloat())/query->value(9).toFloat()*100,0,'f',2);
        case 20: return query->value(17).toString();
        case 21: return query->value(18).toString();
        case 22: return (query->value(21).toString());
        default:
          return query->value(column).toString();
        }
    }
  if(role==Qt::FontRole)
    if(group_place.value(query->value(rowid_column).toLongLong()).left(2)=="1(")
      return QFont("Arial",10,QFont::Bold);

  if(role==Qt::BackgroundRole)
    {
      if(column==0 && query->value(16).toInt()==1 && query->value(15).toInt()>0)
        return QColor(220,220,255);
      if(column==1 && query->value(19).toInt())
        return QColor(220,220,255);

      if(group_place.value(query->value(rowid_column).toLongLong()).left(2)=="1(")
        return QColor(255,255,220);
    }
  if(role==Qt::ToolTipRole && column==11)
    {
      QSqlQuery q;
      q.exec(QString("select group_concat(map_alias) from cp join course_data using(cp_id) join clscrs using (course_id) where class_id==%1 and cp_id not in (select cp_id from split where prot_id==%2) and map_alias NOT LIKE 'S%'")
             .arg(query->value(13).toLongLong())
             .arg(query->value(rowid_column).toLongLong()));
      q.next();
      if(!q.value(0).toString().isEmpty())
          return tr("Unvisited: ")+q.value(0).toString();
    }
  return QVariant();
}

bool FinishModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
  if(!index.isValid()) return false;
  int row=index.row();int column=index.column();
  if(role!=Qt::EditRole || column!=1) return false;
  bool res;
  query->seek(row);
  last_edited_rowid=query->value(rowid_column).toLongLong();
  last_edited_column=column;
  QString db_cmd;
  qint64 dt;
  dt=QDateTime::fromString(value.toString().simplified(),"hh:mm:ss dd.MM.yyyy")
      .toMSecsSinceEpoch()/1000;
  db_cmd=QString("update protocol set finish_time=%1 where prot_id==%2")
      .arg(dt)
      .arg(last_edited_rowid);

  qDebug() << db_cmd;
  res=query->exec(db_cmd);
  selectSoft();
  if(res)
    {
      dataChanged(createIndex(row,column,nullptr),createIndex(row,column,nullptr));
      ui_settings.finish_list_change_notify();
    }
  return true;
}


Qt::ItemFlags FinishModel::flags(const QModelIndex &index) const
{
  if(index.column()==1)
    return Qt::ItemIsEnabled|Qt::ItemIsSelectable|Qt::ItemIsEditable;
  else
    return Qt::ItemIsEnabled|Qt::ItemIsSelectable;
}

bool FinishModel::removeRow(int row)
{
  if(!query->seek(row)) return false;
  qint64 prot_id=query->value(rowid_column).toLongLong();
  OSqlTableModel::removeRow(row);
  query->exec(QString("update protocol set start_time=NULL,finish_time=NULL,total_time=NULL,score=NULL,penalty=NULL where prot_id==%1")
              .arg(prot_id));
  return true;
}

QString FinishModel::csvLine(const int row)
{
  QString line,s;
  int i;

  for(i=0;i<=column_count;i++)
    {
      if(i==2)
        {
          s=QString("%1;%2;")
              .arg(data(createIndex(row,17,nullptr),Qt::DisplayRole).toString())
              .arg(data(createIndex(row,18,nullptr),Qt::DisplayRole).toString());
          line.append(s);
          continue;
        }
      if(i==10 && ui_settings.account_teams)
        {
          line.append(QString("%1;")
              .arg(DisplayTime::hour_min_sec(data(createIndex(row,22,nullptr),Qt::DisplayRole).toUInt())));
          continue;
        }
      if(i==7)
        {
          s=data(createIndex(row,i,nullptr),Qt::DisplayRole).toString();
          s.truncate(s.indexOf('('));
          line.append(s);
        }
      else
        line.append(data(createIndex(row,i,nullptr),Qt::DisplayRole).toString());

      line.append(";");
    }
  line.append("\n");
  return line;
}

QString FinishModel::csvSplitsLine(const int row,const bool add_printout_info)
{
  QSqlQuery q;
  QString line;
  int t,t_prev=0,dist,total_dist=0;
  int speed;
  qint64 cp,cp_prev;

  line.append(data(createIndex(row,4,nullptr),Qt::DisplayRole).toString());
  line.append(";");
  line.append(data(createIndex(row,3,nullptr),Qt::DisplayRole).toString());
  line.append(";");
  line.append(data(createIndex(row,17,nullptr),Qt::DisplayRole).toString());
  line.append(";");
  line.append(data(createIndex(row,18,nullptr),Qt::DisplayRole).toString());
  line.append(";");
  line.append(data(createIndex(row,0,nullptr),Qt::DisplayRole).toString());
  line.append(";");

  query->seek(row);

  q.exec(QString("select cp_id from cp where race_id==%2 and map_alias LIKE 'S%'")
         .arg(ui_settings.current_race));
  q.next();cp_prev=q.value(0).toLongLong();

  q.exec(QString("select time,'('|| map_alias ||') ',cp.cp_id from split join cp on split.cp_id==cp.cp_id and split.prot_id==%1 order by time")
         .arg(query->value(rowid_column).toLongLong()));
  while(q.next())
    {
      t=q.value(0).toInt();
      cp=q.value(2).toLongLong();
      line.append(t>3600 ? DisplayTime::hour_min_sec(t) :
                           DisplayTime::min_sec(t));
      line.append(q.value(1).toString());
      line.append(DisplayTime::min_sec(t-t_prev));
      dist=static_cast<CPTab*>(tabCP)->distance(cp,cp_prev);
      speed=dist ? (t-t_prev)*1000/dist : 0;
      total_dist+=dist;
      line.append(QString(tr(" %1/km;"))
                  .arg(dist ? DisplayTime::min_sec(speed) : 0,1,'0'));
      t_prev=t;
      cp_prev=cp;
    }
  line.append(QString(tr("Avg speed: %1/km;"))
              .arg(total_dist ? DisplayTime::min_sec(t*1000/total_dist) : 0));
  line.append(QString(tr("Distance: %1 m")).arg(total_dist));
  if(add_printout_info)
  {
      line.append(QString(tr(";Result: %1;")).
                  arg(data(createIndex(row,9,nullptr),Qt::DisplayRole).toString()));
      QString distance_complete=
              (data(createIndex(row,7,nullptr),Qt::DisplayRole).toString().left(2)==QString("DQ")) ? QString("DQ") : QString("OK");
      line.append(QString(tr("Distance complete: %1;")).
                  arg(distance_complete));
      line.append(QString(tr("Controls taken: %1")).
                  arg(data(createIndex(row,11,nullptr),Qt::DisplayRole).toString()));
  }
  else
      line.append("\n");
  return line;
}



FinishTab::FinishTab(QWidget *parent) : QWidget(parent)
{
  QVector<int> w{80,80,200,120,60,50,80,70,60,80,90,70};
  view=new OTableView;
  model=new FinishModel(this);
  view->setSelectionMode(QAbstractItemView::ExtendedSelection);
  view->setSelectionBehavior(QAbstractItemView::SelectRows);
  view->horizontalHeader()->setStretchLastSection(true);
  view->setModel(model);
  view->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  view->setAlternatingRowColors(true);
  lTabH = new QHBoxLayout;
  lTabV=new QVBoxLayout;

  bReadTag=new QPushButton(QIcon(":/icons/download-chip.png"),"");
  bReadTag->setIconSize(QSize(48,20));
  bReadTag->setToolTip(tr("<font color='gray'>Download finished tag</font> <b>Ctrl+R</b>"));
  scReadTag=new QShortcut(QKeySequence(tr("Ctrl+R")),this);

  bRemove=new QPushButton(QIcon(":/icons/remove-protocol.png"),"");
  bRemove->setIconSize(QSize(48,20));
  bRemove->setToolTip(tr("Erase finish records and splits"));

  bRecalc=new QPushButton(QIcon(":/icons/update.png"),"");
  bRecalc->setIconSize(QSize(48,20));
  bRecalc->setToolTip(tr("<font color='gray'>Recalculate protocol</font> <b>F5</b>"));
  scRecalc=new QShortcut(QKeySequence(tr("F5")),this);

  bClassSelector=new QPushButton(QIcon(":/icons/filter.png"),"",this);
  bClassSelector->setIconSize(QSize(16,16));
  bClassSelector->setToolTip(tr("Filter visible classes"));
  bClassSelector->show();

  bFindCompetitor=new QPushButton(QIcon(":/icons/search.png"),"",this);
  bFindCompetitor->setIconSize(QSize(48,16));
  bFindCompetitor->setToolTip(tr("<font color='gray'>Search competitor</font> <b>Ctrl+F</b>"));
  bFindCompetitor->show();

  scFindCompetitor=new QShortcut(QKeySequence(tr("Ctrl+F")),this);

  lTabH->addWidget(bReadTag,1);
  lTabH->addStretch(2);
  lTabH->addWidget(bRemove,1);
  lTabH->addStretch(2);
  lTabH->addWidget(bRecalc,1);
  lTabV->addSpacing(16);
  lTabV->addWidget(view);
  lTabV->addLayout(lTabH);
  setLayout(lTabV);

  selector=new ClassSelector(this);
  menu=new QMenu(this);
  searchLine=new SearchLine(model,view,2,this);

  connect(scFindCompetitor,SIGNAL(activated()),searchLine,SLOT(popup()));
  connect(bFindCompetitor,SIGNAL(clicked()),searchLine,SLOT(popup()));
  connect(bClassSelector,SIGNAL(clicked()),SLOT(selectorPopup()));
  connect(view->horizontalHeader(), SIGNAL(sectionClicked(int)), model, SLOT(setSortColumn(int)));
  connect(view->horizontalHeader(), SIGNAL(sectionResized(int,int,int)),SLOT(columnResized(int,int,int)));
  connect(selector,SIGNAL(filterClause(QString)),model,SLOT(setFilterClause(QString)));
  connect(bReadTag,SIGNAL(clicked()),SLOT(read_tag()));
  connect(scReadTag,SIGNAL(activated()),SLOT(read_tag()));
  connect(bRemove,SIGNAL(clicked(bool)),SLOT(remove_record()));
  connect(bRecalc,SIGNAL(clicked()),SLOT(recalc_score()));
  connect(scRecalc,SIGNAL(activated()),SLOT(recalc_score()));

  connect(model,SIGNAL(newSplitsReady(qint64)),SLOT(autoPrintSplits(qint64)));

  connect(&ui_settings,SIGNAL(race_changed()),model,SLOT(resetFilterClause()));
  connect(&ui_settings,SIGNAL(race_changed()),model,SLOT(select()));
  connect(&ui_settings,SIGNAL(protocol_changed()),model,SLOT(select()));
  connect(&ui_settings,SIGNAL(classes_changed()),model,SLOT(select()));

  connect(view->selectionModel(),SIGNAL(currentChanged(QModelIndex,QModelIndex)),model,SLOT(currentChanged(QModelIndex,QModelIndex)));
  for(int i=0;i<w.size();i++) view->setColumnWidth(i,w.at(i));
  columnResized(0,0,0);
}


void FinishTab::read_tag()
{
  if(!ui_settings.current_race) return;

  if(bs->sendCmd(QByteArray::fromRawData("\x81",1),3000))
    connect(bs,SIGNAL(comm_result(QByteArray)),model,SLOT(splitsArrived(QByteArray)));
}

void FinishTab::remove_record()
{
  if(!ui_settings.current_race) return;
  if(!view->currentIndex().isValid()) return;
  if(QMessageBox::question(this,tr("Remove record"),
                  tr("Remove record and erase splits?\nAre you sure?"),
                  QMessageBox::Yes | QMessageBox::No)==QMessageBox::No)
    return;
  model->removeRow(view->currentIndex().row());
  model->select();
  ui_settings.finish_list_change_notify();
  ui_settings.protocol_change_notify();
  view->setFocus();
}

void FinishTab::edit_splits()
{
  if(!ui_settings.current_race) return;
  SplitsDialog se(model->getRowID(view->currentIndex()),this);
  se.exec();
  model->select();
}


void FinishTab::printSplits()
{
  if(!ui_settings.current_race) return;
  int row;

  if(request_for_autoprint)
    {
      request_for_autoprint=false;
      row=(model->index(arrived_protid,0)).row();
      if(row<0) return;
    }
  else
    {
      if(!view->currentIndex().isValid()) return;
      else row=view->currentIndex().row();
    }
  QSqlQuery q;
  QString db_cmd=QString("select event.name,race.name,race.location from race join event using (event_id) where race_id==%1")
      .arg(ui_settings.current_race);
  q.exec(db_cmd);
  q.next();
  splits_printer->printSplits(q.value(0).toString(),q.value(1).toString(),
                              q.value(2).toString(),
                              model->csvSplitsLine(view->currentIndex().row(),true));
}

void FinishTab::autoPrintSplits(qint64 prot_id)
{
  if(prot_id!=arrived_protid && ui_settings.auto_printout) request_for_autoprint=true;
  arrived_protid=prot_id;
  if(request_for_autoprint) printSplits();
}

QString FinishTab::indexListRowID()
{
  QModelIndexList idx=view->selectionModel()->selectedRows();

  QString list;
  for(QModelIndex& i:idx)
    list.append(QString("%1,").arg(model->getRowID(i)));
  if(!list.isEmpty()) list.chop(1);
  list.append(")");
  return list;
}

void FinishTab::setStartTimeForced()
{
  QString db_cmd="update protocol set start_time_selector=1 where prot_id in ("
      +indexListRowID();
  QSqlQuery q;
  q.exec(db_cmd);
  model->select();
}

void FinishTab::setFinishTimeForced()
{
  view->edit(static_cast<QAbstractItemModel*>(model)->index(view->selectionModel()->selectedRows().at(0).row(),1));
}

void FinishTab::clearStartTimeForced()
{
  QString db_cmd="update protocol set start_time_selector=0 where prot_id in ("
      +indexListRowID();
  QSqlQuery q;
  q.exec(db_cmd);
  model->select();
}



void FinishTab::clearFinishTimeForced()
{
  QString db_cmd="update protocol set finish_time=NULL where prot_id in ("
      +indexListRowID();
  QSqlQuery q;
  q.exec(db_cmd);
  model->select();
}

void FinishTab::setDQ()
{
  QString db_cmd="update protocol set score=-1 where prot_id in ("
      +indexListRowID();
  QSqlQuery q;
  q.exec(db_cmd);
  model->select();
}

void FinishTab::clearDQ()
{
  QString db_cmd="update protocol set score=0,penalty=NULL where prot_id in ("
      +indexListRowID();
  QSqlQuery q;
  q.exec(db_cmd);
  model->select();
}

void FinishTab::removeControlPoint()
{
  QString cp_name=QInputDialog::getText(
        this,tr("Remove control"),tr("Enter contol map designation: "));
  QString db_cmd=QString("delete from split where cp_id=(select cp_id from cp where race_id==%1 and map_alias=='%2') and prot_id in (")
      .arg(ui_settings.current_race)
      .arg(cp_name)
      +indexListRowID();
  QSqlQuery q;
  q.exec(db_cmd);
  model->select();
}

void FinishTab::addControlPoint()
{

}


void FinishTab::export_csv()
{
  if(!ui_settings.current_race) return;
  QSqlQuery q;
  QFile protocol,splits;
  q.exec(QString("select coalesce(event.name || ' - ' ,'') || race.name || '.csv' from race left join event using(event_id) where race_id==%1")
      .arg(ui_settings.current_race));
  q.next();
  QString filename = QFileDialog::getSaveFileName(this, tr("Export protocol to CSV"),
                                 q.value(0).toString(),
                                 tr("CSV data (*.csv)"));
  if(filename.isEmpty()) return;
  ui_settings.update_current_dir(filename);
  protocol.setFileName(filename);
  splits.setFileName(filename.left(filename.lastIndexOf('.'))+tr(" Splits.csv"));
  if(!protocol.open(QIODevice::WriteOnly)) return;
  if(!splits.open(QIODevice::WriteOnly)) {protocol.close();return;}
  if(ui_settings.account_teams)
    protocol.write(tr("Start;Finish;Name;Birthdate;Team;Class;Lap;Race time;Place;Total score;Total time;Team Time;Control taken;Inverse % to leader's time\n").toLocal8Bit());
  else
    protocol.write(tr("Start;Finish;Name;Birthdate;Team;Class;Lap;Race time;Place;Total score;Total time;Lag from leader;Control taken;Inverse % to leader's time\n").toLocal8Bit());
  for (qint64 it : place_order)
    {
      protocol.write(model->csvLine(model->index(it,0).row()).toLocal8Bit());
      splits.write(model->csvSplitsLine(model->index(it,0).row(),false).toLocal8Bit());
    }
  protocol.close();
  splits.close();

//save cards
  QString cards_filename=
      filename.left(filename.lastIndexOf('.'))+tr(" Cards.zip");
#ifdef Q_OS_LINUX
  QProcess gzip;
  gzip.execute("zip",{"-u9",cards_filename,"cards/*.*"});
#elif defined Q_OS_WIN
  QProcess gzip;
  gzip.execute("7z.exe",{"u",cards_filename,"cards/*.*"});
#endif
  gzip.waitForFinished();

}

void FinishModel::recalc_score()
{
  QSqlDatabase db=QSqlDatabase::database();
  QSqlQuery q;
  q.exec (QString("select protocol_closed from race where race_id==%1")
          .arg(ui_settings.current_race));
  q.next();
  if(q.value(0).toInt()) return;
  db.transaction();
  for(int i=0;i<rowCount(QModelIndex());i++)
    update_score(getRowID(QAbstractTableModel::index(i,0)));
  db.commit();
  selectSoft();
  ui_settings.protocol_change_notify();
}



void FinishModel::splitsArrived(QByteArray spl)
{
  disconnect(bs,SIGNAL(comm_result(QByteArray)),this,SLOT(splitsArrived(QByteArray)));
  if(spl.isEmpty()) return;
  QString cardnum(QString::asprintf("%02x%02x%02x%02x",
          (uint8_t)spl.at(0),(uint8_t)spl.at(1),(uint8_t)spl.at(2),(uint8_t)spl.at(3)));
  QFile raw("cards/"+cardnum+".bin");
  if(raw.open(QIODevice::WriteOnly))
    {
      for(int i=0;i<64;i++)
      {
        if((uint8_t)spl.at(i*17+16)==(uint8_t)EMFAUTH) raw.write("AAAAAAAAAAAAAAAA",16);
        else
        {
          if(spl.at(i*17+16)!=0) raw.write("RRRRRRRRRRRRRRRR",16);
          else
            raw.write(spl.constData()+i*17,16);
        }
      }
      raw.close();
    }

  //extract splits
  const CardData* c=reinterpret_cast<const CardData*>(spl.constData());
  QDateTime start_time=QDateTime::fromMSecsSinceEpoch(
        static_cast<qint64>(c->start_time)*1000);
  splits.clear();
  uint32_t s,s_time,s_cp;
  for(int i=0;i<56;i++)
  {
    //don't read sector trailer
    if((i&0x3)==3) continue;
    //if crc for string ok,get 4 28-bit adjacent split values
    if(crc16(c->page[i],16)) continue;
    for(int j=0;j<4;j++)
    {
      s=*(uint32_t*)(c->page[i]+(j*7>>1));
      if(!s) continue;
      if(j&0x1) s>>=4;
      s&=0xfffffff;
      s_cp=s&0xff;
      s_time=s>>8;
      splits.insert(s_time,s_cp);
    }
  }

  //write splits to protocol
  QSqlDatabase db=QSqlDatabase::database();

  QSqlQuery q;
  QMap<uint32_t,uint8_t>::const_iterator it=splits.begin();
  int last_cp=-1,row=-1;
  qint64 prot_id=0;

  q.exec(QString("select prot_id from protocol join team_member using(tm_id) join team using(team_id) join class using(class_id) where race_id==%1 and chip=='%2'")
         .arg(ui_settings.current_race).arg(cardnum));
  if(q.next())
    prot_id=q.value(0).toLongLong();
  else
    return;
  if(!prot_id) return;

  db.transaction();
  q.exec(QString("update protocol set start_time=%1 where prot_id==%2")
         .arg(c->start_time)
         .arg(prot_id));

  while (it!=splits.end())
    {
      if(last_cp==it.value()) {++it;continue;}
      last_cp=it.value();
      q.exec(QString("insert into split(prot_id,cp_id,time) select %4,cp_id,%1 from cp where int_number==%2 and race_id==%3")
             .arg(it.key())
             .arg(it.value())
             .arg(ui_settings.current_race)
             .arg(prot_id));
      ++it;
    }
  db.commit();
  update_score(prot_id);
  select();
  ui_settings.statusbar_message_notify(tr("Card %1 has arrived").arg(cardnum),4000);
  emit(newSplitsReady(prot_id));
  ui_settings.finish_list_change_notify();
}













void FinishTab::contextMenuEvent(QContextMenuEvent *event)
{
  QPoint pos=event->pos()-view->pos();
  if (event->reason()!=QContextMenuEvent::Mouse) {event->ignore();return;}
  if(!view->geometry().contains(event->pos())) return;

  QModelIndexList idx=view->selectionModel()->selectedRows();
  menu->clear();
  if(!idx.size()) return;
  menu->addAction(tr("Print splits"),this,SLOT(printSplits()));
  menu->addAction(tr("Use chip start time"),this,SLOT(clearStartTimeForced()));
  menu->addAction(tr("Use protocol start time"),this,SLOT(setStartTimeForced()));
  menu->addAction(tr("Use chip finish time"),this,SLOT(clearFinishTimeForced()));
  menu->addAction(tr("Set DQ Status"),this,SLOT(setDQ()));
  menu->addAction(tr("Clear DQ Status"),this,SLOT(clearDQ()));
  menu->addSeparator();
  menu->addAction(tr("Add control point"),this,SLOT(addControlPoint()));
  menu->actions().last()->setDisabled(true);
  menu->addAction(tr("Remove control point"),this,SLOT(removeControlPoint()));
  if(idx.size()==1)
    {
      menu->addSeparator();
      menu->addAction(tr("Edit split"),this,SLOT(edit_splits()));
      menu->addAction(tr("Set finish time"),this,SLOT(setFinishTimeForced()));
    }
  else
    {

    }
  //menu->move(event->globalPos());
  menu->popup(event->globalPos());
}


void FinishModel::update_score(const qint64 prot_id)
{
  QSqlQuery cclist,q;

  cclist.exec(QString("select disc_id, course_id,value_is_time,penalty_is_time,min_penalty*(1+(penalty_is_time!=0)*59),max_penalty*(1+(penalty_is_time!=0)*59),min_score*(1+(value_is_time!=0)*59),soft_order,overtime,overtime_penalty from clscrs join course using(course_id) where class_id==%1")
         .arg(static_cast<ProtocolTab*>(tabProtocol)->getClassOfCompetitor(prot_id)));
  int total_time=0,total_score=0,total_penalty=0;
  uint32_t finish_time;
  bool qualification=false,dq_anyway=false;

  //create view so that S1 (start) "split" will be included
  q.exec("drop view if exists v_split");
  q.exec(QString("create temp view v_split (cp_id,time) as select cp_id,0 from cp where race_id==%2 and map_alias LIKE 'S%' union select cp_id,time from split where prot_id==%1 order by time")
         .arg(prot_id)
         .arg(ui_settings.current_race));

  q.exec(QString("select case when start_time_selector and start_time_forced>0 then start_time-start_time_forced+max(time) else max(time) end,case when start_time_selector and start_time_forced>0 then start_time_forced else start_time end,finish_time from protocol,v_split where protocol.prot_id==%1")
         .arg(prot_id));
  q.next();
  if (q.value(2).toInt())
    {
      total_time=q.value(2).toInt()-q.value(1).toInt();
      finish_time=q.value(2).toUInt();
    }
  else
    {
      total_time=q.value(0).toInt();
      finish_time=q.value(1).toUInt()+total_time;
    }


  if(total_time<=0)
    {
      q.exec(QString("update protocol set total_time=%1,score=%2 where prot_id==%3")
             .arg(total_time)
             .arg(-1)
             .arg(prot_id));
      return;
    }



  //don't take into account previously marked as DQ
  if(query->seek(query_line.value(prot_id)))
    if(query->value(8).toInt()==-1)
      return;

  while(cclist.next())
    {
      int discipline=cclist.value(0).toLongLong();
      int score,penalty;
      q.exec(QString("select sum(value*(1+(value_is_time!=0)*59)) from course_data join course using(course_id) where course_id==%1 and cp_id in (select cp_id from v_split)")
             .arg(cclist.value(1).toLongLong()));
      q.next();
      score=q.value(0).toInt();
      q.exec(QString("select sum(penalty*(1+(penalty_is_time!=0)*59)) from course_data join course using(course_id) where course_id==%1 and cp_id not in (select cp_id from v_split)")
             .arg(cclist.value(1).toLongLong()));
      q.next();
      penalty=q.value(0).toInt();

      if(discipline==oscore_mod)
        {
          uint32_t overtime=cclist.value(8).toUInt();
          int overtime_penalty=cclist.value(9).toInt();
          score=0;penalty=0;
          query->seek(query_line.value(prot_id));

          //accumulate total values of all taken controls
          q.exec(QString("with t as (select count(tm_id)  as tot_members from team_member where team_id==%1)"
                         "select count(tm_id),tm_id,map_alias,course_data.value,protocol.score==-1 from t,split join protocol using(prot_id) join team_member using(tm_id) join cp using(cp_id)  join team using(team_id) join course_data using(cp_id) where team_id==%1 and course_id==%2 group by cp_id having count(tm_id)=tot_members or cast(map_alias as integer)<100 order by tm_id")
                 .arg(query->value(20).toLongLong())
                 .arg(cclist.value(1).toLongLong()));

          while(q.next())
            {
              //if any of team member has -1, DQ everyone
              if(q.value(4).toInt()==1) dq_anyway=true;
              score+=q.value(3).toInt();
            }

          //accumulate existing penalties
          q.exec(QString("select penalty from protocol join team_member using(tm_id) join team using (team_id) where team_id==%1")
                 .arg(query->value(20).toLongLong()));
          while(q.next())
            penalty+=q.value(0).toInt();


          score-=penalty;penalty=0;
          //calculate overtime penalty
          if(overtime<10*24*3600) //if overtime is personal
            {
              if(total_time>overtime)
                {
                  penalty=(total_time-overtime)*overtime_penalty/60;
                  if ((total_time-overtime)*overtime_penalty%60) ++penalty; //round to ceiling
                  if(total_time-overtime>15*60) dq_anyway=true;
                }
            }
          else
            {
              if(finish_time>overtime)
                {
                  penalty=(finish_time-overtime)*overtime_penalty/60;
                  if ((finish_time-overtime)*overtime_penalty%60) ++penalty;
                  if(finish_time-overtime>15*60) dq_anyway=true;
                }
            }

          if(score<0) { score=-1;dq_anyway=true; }
          total_score+=score;
          total_penalty+=penalty;
          qualification=true;
          continue;
        }

      if(discipline==single)
        {
          int ord=0,max_ord=1;
          q.exec("drop view if exists edge");
          q.exec(QString("create temp view edge as with t as (select min(ord) as minord,max(ord) as maxord from course_data where course_id==%1) select cp_id from course_data,t where course_id==%1 and (ord==t.minord or ord==t.maxord)")
                 .arg(cclist.value(1).toInt()));
          q.exec(QString("select count(*) from course_data where course_id==%1")
              .arg(cclist.value(1).toLongLong()));
          if(q.next()) max_ord=q.value(0).toInt();
          q.exec(QString("with t as (select min(time) as mintime,max(time) as maxtime from v_split where cp_id in (select cp_id from edge)) select v_split.cp_id,ord from course_data join v_split on v_split.cp_id==course_data.cp_id and course_data.course_id==%1,t where time>=t.mintime and time<=t.maxtime order by time")
                 .arg(cclist.value(1).toLongLong()));
          while (q.next())
              if(q.value(1).toInt()==ord+1) ++ord;
          if(ord!=max_ord)
            continue;
        }

      //consider unaccountable penalty
      penalty-=cclist.value(4).toInt();
      if(penalty<0) penalty=0;

      //check for max allowed penalty
      //and that minimum score has been achieved
      if(discipline==unordered)
        {
          if ((penalty<=cclist.value(5).toInt()) &&
              (score>=cclist.value(6).toInt()) )
            qualification=true;
        }
      else
        {
          qualification=true;
        }


      //check if score measured in time units
      if(cclist.value(2).toInt()==0)
        total_score+=score;
      else
        total_time-=score;

      //check if penalty measured in time units
      if(cclist.value(3).toInt()==0)
        total_score-=penalty;
      else
        total_time+=penalty;
    }



  if(!total_time)
    q.exec(QString("update protocol set total_time=NULL,score=NULL where prot_id==%3")
         .arg(prot_id));
  else
    q.exec(QString("update protocol set total_time=%1,score=%2,penalty=%3 where prot_id==%4")
       .arg(total_time)
       .arg(qualification & !dq_anyway ? total_score : -1)
       .arg(total_penalty)
       .arg(prot_id));
}











//====================== CLASS SELECTOR =============================//
ClassSelector::ClassSelector(QWidget *parent)
  :QGroupBox (parent)
{
  bOk=new QPushButton(tr("Apply"));
  toggleAll=new QCheckBox(tr("Enable/disable all"));
  setTitle(tr("Select classes"));
  setAutoFillBackground(true);
  connect(bOk,SIGNAL(clicked()),SLOT(apply()));
  connect(toggleAll,SIGNAL(stateChanged(int)),SLOT(allStateChanged(int)));
  lout=new QVBoxLayout;
  hide();
}

void ClassSelector::popup(const QPoint pos)
{
  q.exec("create temp table if not exists class_filter(class_id INTEGER UNIQUE ON CONFLICT IGNORE,name TEXT,show INTEGER DEFAULT 2)");
  q.exec(QString("delete from class_filter where class_id not in (select class_id from class where race_id==%1)")
      .arg(ui_settings.current_race));
  q.exec(QString("insert into class_filter (class_id,name)  select class_id,name from class where race_id==%1")
      .arg(ui_settings.current_race));
  q.exec("select class_id,name,show from class_filter");
  QCheckBox* cb;
  for (auto&x : choice)
    {
      lout->removeWidget(std::get<0>(x));
      delete std::get<0>(x);
    }
  choice.clear();
  lout->addWidget(toggleAll);
  toggleAll->setFocusPolicy(Qt::NoFocus);
  while(q.next())
    {
      cb=new QCheckBox(q.value(1).toString());
      cb->setCheckState(static_cast<Qt::CheckState>(q.value(2).toInt()));
      choice.push_back(std::tuple<QCheckBox*,qint64,QString> (cb,q.value(0).toLongLong(),q.value(1).toString()));
      lout->addWidget(cb);
      cb->setFocusPolicy(Qt::NoFocus);
    }
  lout->addWidget(bOk);
  bOk->setFocusPolicy(Qt::NoFocus);

  resize(200,100+30*choice.size());
  setLayout(lout);
  raise();
  move(pos);
  show();
  setFocus();
}


void ClassSelector::apply()
{
  QString db_cmd="update class_filter set show=%1 where class_id==%2";
  for(auto& x : choice)
    {
      q.exec(db_cmd
             .arg(static_cast<int>(std::get<0>(x)->checkState()))
             .arg(std::get<1>(x)));
    }
  q.exec("select group_concat(class_id) from class_filter where show!=0");
  if(q.next())
    emit(filterClause(QString(" and class_id in (%1) ")
                      .arg(q.value(0).toString())));
  else
    emit(filterClause(""));
  hide();
}

void ClassSelector::allStateChanged(int state)
{
  for(auto& x : choice) std::get<0>(x)->setCheckState(static_cast<Qt::CheckState>(state));
}
