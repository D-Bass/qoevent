#include "protocol.h"
#include "comboxeditor.h"
#include <QHeaderView>
#include <QDebug>
#include "uisettings.h"
#include <QContextMenuEvent>
#include <QSqlError>
#include "comm.h"
#include <QMessageBox>
#include <QDateTime>
#include <QFileDialog>


QWidget* tabProtocol;


ProtocolModel::ProtocolModel(QObject* parent)
  : OSqlTableModel(7,14,"",parent)
{
  select();
}



void ProtocolModel::selectSoft()
{
  QString sort_clause,show_unfinished_clause;
  if(sort_column>=0 && sort_column<3) sort_clause_num=sort_column;
  switch(sort_clause_num)
    {
    case 0: sort_clause=" order by 1,2,3";break;
    case 1: sort_clause=" order by 2,3,1";break;
    case 2: sort_clause=" order by 3,2,1";break;
    default:
      sort_clause=" order by 1,2,3";

    }

  if(show_unfinished_only)
    {
      show_unfinished_clause=" and start_time is NULL";
      sort_clause=" order by chip is NOT NULL DESC,3";
    }
  query->exec(QString("select class.name,team.name,competitor.name,birthdate,lap,number,start_time_forced,chip,start_time,start_time_selector,class.class_id,team.team_id,team_member.tm_id,competitor.comp_id,protocol.prot_id from protocol join team_member using(tm_id) join competitor using(comp_id) join team using(team_id) join class using(class_id) where class.race_id==%1"
                      +show_unfinished_clause + sort_clause)
              .arg(ui_settings.current_race));
  int started=0,finished=0,total=0;
  while(query->next())
    {
      ++total;
      if(query->value(8).toInt()) ++finished;
      if(query->value(7).toString()!="") ++started;
    }
  emit summaryUpdated(QString(tr("Started: %1  Finished: %2  Unregistered: %3")
                      .arg(started).arg(finished)).arg(total-started));
}



QVariant ProtocolModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if(role!=Qt::DisplayRole || orientation!=Qt::Horizontal) return QVariant();
  return header_name.at(section);
}

QVariant ProtocolModel::data(const QModelIndex &index, int role) const
{
  int row=index.row();int column=index.column();
  query->seek(row);
  if(role==Qt::DisplayRole || role==Qt::EditRole)
    {
      if(column==6)  //invisible column, for start_time data
        {
          QDateTime dt;
          if(query->value(6).toInt()<=0) return QVariant();
          dt=QDateTime::fromMSecsSinceEpoch(query->value(6).toLongLong()*1000);
          if(role==Qt::DisplayRole)
            return dt.toString("hh:mm:ss dd.MM");
          else
            return dt.toString("hh:mm:ss dd.MM.yyyy");

        }
       return query->value(column);
    }
  if(role==Qt::BackgroundRole)
    {
      //show started with green, finished with red
      if(query->value(7).toString()!="")
        {
          if(query->value(8).toInt()!=0)
            return QColor(255,180,180);
          else return QColor(180,255,180);
        }
    }
  return QVariant();
}

Qt::ItemFlags ProtocolModel::flags(const QModelIndex &index) const
{
  int col=index.column();
  if(col<=2 || col>=4 && col<=6)
      return Qt::ItemIsEnabled|Qt::ItemIsSelectable|Qt::ItemIsEditable;
  return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}



bool ProtocolModel::setData(const QModelIndex &index, const QVariant &value, int role)
{

  if(!index.isValid()) return false;
  int row=index.row();int column=index.column();
  if(role!=Qt::EditRole) return false;
  bool res;
  query->seek(row);
  last_edited_rowid=query->value(rowid_column).toLongLong();
  last_edited_column=column;
  QString db_cmd;
  qint64 dt;
  switch(column)
    {
    case 0:
      db_cmd=QString("update team set %1=%2 where team_id==%3")
          .arg(db_field.at(column))
          .arg(value.toString())
          .arg(query->value(11).toString());
      break;
    case 1:
    case 2:
    case 5:
      db_cmd=QString("update team_member set %1='%2' where tm_id==%3")
          .arg(db_field.at(column))
          .arg(value.toString())
          .arg(query->value(12).toString());
      break;
    case 4:
      db_cmd=QString("update protocol set %1='%2' where prot_id==%3")
          .arg(db_field.at(column))
          .arg(value.toString())
          .arg(last_edited_rowid);
      break;
    case 6:
      dt=QDateTime::fromString(value.toString().simplified(),"hh:mm:ss dd.MM.yyyy")
          .toMSecsSinceEpoch()/1000;
      db_cmd=QString("update protocol set start_time_forced=%1,start_time_selector=%2 where prot_id==%3")
          .arg(dt)
          .arg(dt>0)
          .arg(last_edited_rowid);
      break;
    default: return false;
    }

  qDebug() << db_cmd;
  res=query->exec(db_cmd);
  selectSoft();
  if(res)
    {
      dataChanged(createIndex(row,column,nullptr),createIndex(row,column,nullptr));
      ui_settings.protocol_change_notify();
    }

  return true;
}


bool ProtocolModel::insertRow(const QString text)
{
  QSqlQuery q;
  QStringList val=text.split(';',QString::SkipEmptyParts);
  for(auto& l : val) l=l.simplified();
  if(val.size()!=4) return false;
  q.exec(QString("insert into team (name,class_id)  select '%3',class_id from class where race_id==%1 and name=='%2'")
         .arg(ui_settings.current_race)
         .arg(val.at(2))
         .arg(val.at(3)));
  if (q.exec(QString("insert into team_member (team_id,comp_id) values((select team_id from team join class using(class_id) where team.name=='%1' and class.name=='%2' and race_id==%3),(select comp_id from competitor where name=='%4' and birthdate==%5))")
         .arg(val.at(3))
         .arg(val.at(2))
         .arg(ui_settings.current_race)
         .arg(val.at(0))
         .arg(val.at(1))))
  {
        notifyInsertedRow(
              q.exec("insert into protocol(tm_id) select last_insert_rowid()"));

  }
  return true;
}


bool ProtocolModel::removeRow(int row)
{
  if(!query->seek(row)) return false;
  qint64 tm_id=query->value(12).toLongLong();
  qint64 team_id=query->value(11).toLongLong();
  beginRemoveRows(QModelIndex(),row,row);
  bool res=query->exec(QString("delete from team_member where tm_id==%1")
              .arg(tm_id));
  //try to delete team (successful if no more ref in team_member)
  query->exec(QString("delete from team where team_id==%1")
              .arg(team_id));

  selectSoft();
  if(res) {

    endRemoveRows();
  }
  return true;
}

qint64 ProtocolModel::getRowID(const QModelIndex &index) const
{
  if(!query->seek(index.row())) return 0;
  switch (index.column())
    {
    case 0: return query->value(10).toLongLong();
    case 1: return query->value(11).toLongLong();
    case 2: return query->value(13).toLongLong();
    default: return query->value(rowid_column).toLongLong();
    }
//  return OSqlTableModel::getRowID(index);
}

QModelIndex ProtocolModel::indexFromID(qint64 id)
{
  if (!query_line.contains(id)) return QModelIndex();
  return createIndex(query_line.value(id),4,nullptr);
}

qint64 ProtocolModel::getClassID(const qint64 rowid) const
{
  if(query->seek(query_line.value(rowid)))
    return query->value(10).toLongLong();
  else
    return 0;
}

bool ProtocolModel::chipAssigned(const QModelIndex &index) const
{
  query->seek(index.row());
  return (query->value(7).toString()!="");
}


//============================== VIEW ======================================//

ComboxEditor comp_editor(QString("select name||' '||birthdate,comp_id from competitor order by name"));
ComboxEditor class_protocol_editor(QString("select name,class_id from class where race_id==%1 order by name"));
ComboxEditor team_editor(QString("select team.name||' ('||class.name||')',team_id from team join class using(class_id) where race_id==%1 order by team.name"));

//============================== TAB =========================================//


ProtocolTab::ProtocolTab(QWidget *parent) : QWidget(parent)
{
  QVector<int> w{70,150,200,70,70,70,200};
  view=new OTableView;
  model=new ProtocolModel(this);
  view->setSelectionMode(QAbstractItemView::ExtendedSelection);
  view->setSelectionBehavior(QAbstractItemView::SelectRows);
  //view->setSelectionBehavior(QAbstractItemView::SelectItems);
  view->horizontalHeader()->setStretchLastSection(true);
  view->setModel(model);
  view->setValidator("^\\w+ \\w+;\\d{4};[^;]+;[^;]+$");
  view->setTextHint(tr("Competitor Name;Birthdate;Class;TeamName"));

  view->setItemDelegateForColumn(0,&class_protocol_editor);
  view->setItemDelegateForColumn(1,&team_editor);
  view->setItemDelegateForColumn(2,&comp_editor);
  view->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

  for(int i=0;i<7;i++) view->setColumnWidth(i,w.at(i));
  lTabH = new QHBoxLayout;
  lTabV=new QVBoxLayout;

  info=new QLabel;
  bAdd= new QPushButton(QIcon(":/icons/add-team.png"),tr("Add team"));
  bRemove= new QPushButton(QIcon(":/icons/remove-file.png"),tr("Remove"));
  bAdd->setIconSize(QSize(48,20));
  bRemove->setIconSize(QSize(48,20));

  bShowUnfinished=new QPushButton(QIcon(":/icons/run.png"),"");
  bShowUnfinished->setIconSize(QSize(48,20));
  bShowUnfinished->setCheckable(true);
  bShowUnfinished->setToolTip(tr("<font color='gray'>Show unfinished</font> <b>F2</b>"));
  scShowUnfinished=new QShortcut(QKeySequence("F2"),this);
  scFindCompetitor=new QShortcut(QKeySequence(tr("Ctrl+F")),this);
  scBindToCard=new QShortcut(QKeySequence(tr("Ctrl+B")),this);

  bClearTag= new QPushButton(QIcon(":/icons/assign-chip.png"),"");
  bClearTag->setIconSize(QSize(48,20));
  bClearTag->setToolTip(tr("<font color='gray'>Check and assign tag</font> <b>Ctrl+B</b>"));

  bFindCompetitor=new QPushButton(QIcon(":/icons/search.png"),"",this);
  bFindCompetitor->setIconSize(QSize(48,16));
  bFindCompetitor->setToolTip(tr("<font color='gray'>Search competitor</font> <b>Ctrl+F</b>"));
  bFindCompetitor->show();

  bUnAssignTag=new QPushButton(QIcon(":/icons/unassign-chip.png"),"");
  bUnAssignTag->setIconSize(QSize(48,20));
  bUnAssignTag->setToolTip(tr("Unassign tag from protocol entry"));

  lTabH->addWidget(bAdd,1,Qt::AlignLeft);
  lTabH->addWidget(bRemove,1,Qt::AlignLeft);
  lTabH->addStretch(2);
  lTabH->addWidget(bShowUnfinished);
  lTabH->addStretch(2);
  lTabH->addWidget(bClearTag,1,Qt::AlignCenter);
  lTabH->addWidget(bUnAssignTag,1,Qt::AlignCenter);
  lTabV->addSpacing(16);
  lTabV->addWidget(info);
  lTabV->addWidget(view);
  lTabV->addLayout(lTabH);
  setLayout(lTabV);

  menu=new QMenu(this);

  searchLine=new SearchLine(model,view,2,this);

  connect(scFindCompetitor,SIGNAL(activated()),searchLine,SLOT(popup()));
  connect(bFindCompetitor,SIGNAL(clicked()),searchLine,SLOT(popup()));

  connect(view->horizontalHeader(),SIGNAL(sectionClicked(int)),model,SLOT(setSortColumn(int)));
  connect(view->horizontalHeader(), SIGNAL(sectionResized(int,int,int)),SLOT(columnResized(int,int,int)));
  connect(bAdd,SIGNAL(clicked()),SLOT(add_team()));
  connect(bRemove,SIGNAL(clicked()),SLOT(remove_competitor()));
  connect(bClearTag,SIGNAL(clicked()),SLOT(clear_tag()));
  connect(bShowUnfinished,SIGNAL(toggled(bool)),model,SLOT(show_unfinished(bool)));
  connect(scShowUnfinished,SIGNAL(activated()),SLOT(toggle_unfinished()));
  connect(scBindToCard,SIGNAL(activated()),SLOT(clear_tag()));
  connect(bUnAssignTag,SIGNAL(clicked()),SLOT(unassign_tag()));
  connect(&ui_settings,SIGNAL(race_changed()),model,SLOT(select()));
  connect(&ui_settings,SIGNAL(race_changed()),SLOT(check_protocol_closed()));
  connect(&ui_settings,SIGNAL(classes_changed()),model,SLOT(select()));
  connect(&ui_settings,SIGNAL(finish_list_changed()),model,SLOT(select()));
  connect(view,SIGNAL(activated(QModelIndex)),view,SLOT(edit(QModelIndex)));
  connect(view->selectionModel(),SIGNAL(currentChanged(QModelIndex,QModelIndex)),model,SLOT(currentChanged(QModelIndex,QModelIndex)));
  connect(model,SIGNAL(summaryUpdated(QString)),info,SLOT(setText(QString)));
  columnResized(0,0,0);
}

void ProtocolTab::contextMenuEvent(QContextMenuEvent *event)
{
  if (event->reason()!=QContextMenuEvent::Mouse) {event->ignore();return;}
  QModelIndexList idx=view->selectionModel()->selectedRows();
  menu->clear();
  if(!idx.size()) return;
  menu->addAction(tr("Set start times"),this,SLOT(setStartTimes()));
  menu->addAction(tr("Clear start times"),this,SLOT(clearStartTimes()));
  menu->popup(event->globalPos());
}

void ProtocolTab::add_team()
{
  if(!ui_settings.current_race) return;
  QSqlQuery q;
  QStringList list;
  q.exec(QString(tr("select cname||';'||bdate||';'||clsname||';'||tname from (select name as cname,birthdate as bdate from competitor where comp_id not in( select comp_id from team_member join team using(team_id) join class using(class_id) where race_id==%1)),(select name as clsname from  class where race_id==%1),(select team.name as tname from team join class using(class_id) where race_id==%1 union select 'individual')"))
         .arg(ui_settings.current_race));
  while(q.next()) list.append(q.value(0).toString());
  if(comp!=nullptr) delete comp;
  comp=new QCompleter(list,this);
  view->setCompleter(comp);
  view->enterNewEntry();
}

void ProtocolTab::remove_competitor()
{
  if(!ui_settings.current_race) return;
  if(!view->currentIndex().isValid()) return;
  model->removeRow(view->currentIndex().row());
  ui_settings.protocol_change_notify();
}



void ProtocolTab::clear_tag()
{
  QModelIndex vi;
  if(!(vi=view->currentIndex()).isValid()) return;
  if(!ui_settings.current_race) return;
  //if not performed previous request yet
  if(model->competitor_waiting_for_card) return;
  //if chip already present
  if(model->chipAssigned(view->currentIndex())) return;
  model->competitor_waiting_for_card=model->OSqlTableModel::getRowID(view->currentIndex());
  if(model->competitor_waiting_for_card)
  {
    if(bs->sendCmd(QByteArray::fromRawData("\x80",1),2500))
      connect(bs,SIGNAL(comm_result(QByteArray)),model,SLOT(clear_tag_arrived(QByteArray)));
  }
}


void ProtocolTab::check_protocol_closed()
{
  QSqlQuery q;
  if(!ui_settings.current_race) return;
  q.exec(QString("select protocol_closed from race where race_id==%1").arg(ui_settings.current_race));
  if(!q.next()) return;
  int closed=q.value(0).toInt();
  for(QPushButton** b : buttons)
    (*b)->setDisabled(closed);
}

void ProtocolTab::unassign_tag()
{
  if(!ui_settings.current_race) return;
  if(QMessageBox::question(this,tr("Unassign tag"),
                  tr("Are you sure?"),
                  QMessageBox::Yes | QMessageBox::No)==QMessageBox::No)
    return;
  model->unassign_tag(view->currentIndex());
}





void ProtocolModel::unassign_tag(const QModelIndex idx)
{
  if(!idx.isValid()) return;
  //if not performed previous request yet
  if(competitor_waiting_for_card) return;
  //if chip already present
  if(!chipAssigned(idx)) return;
  bool res=query->exec (QString("update protocol set chip=NULL where prot_id==%1")
                   .arg(OSqlTableModel::getRowID(idx)));
  selectSoft();
  if(res) dataChanged(createIndex(idx.row(),4),createIndex(idx.row(),4));
}




void ProtocolModel::clear_tag_arrived(QByteArray tag)
{
  QSqlQuery q;
  QString card_id;
  disconnect(bs,SIGNAL(comm_result(QByteArray)),this,SLOT(clear_tag_arrived(QByteArray)));
  if(tag.isEmpty())
  {
    QMessageBox::warning(0,tr("Card ID"),
                          tr("Card not present"),
                          QMessageBox::Ok);
  }
  else
  {
    if (tag.at(1)!=0)
      QMessageBox::warning(0,tr("Card ID"),
                            tr("Card error"),
                            QMessageBox::Ok);
    else
    {
      card_id=QString::asprintf("%02x%02x%02x%02x",
                       (uint8_t)tag.at(2),(uint8_t)tag.at(3),(uint8_t)tag.at(4),(uint8_t)tag.at(5));
      bool res=q.exec(QString("update protocol set chip='%1' where prot_id==%2")
          .arg(card_id)
          .arg(competitor_waiting_for_card));

      QModelIndex idx=indexFromID(competitor_waiting_for_card);
      last_edited_rowid=competitor_waiting_for_card;
      last_edited_column=2;
      selectSoft();
      if(idx.isValid()) dataChanged(index(competitor_waiting_for_card,0),
                                    index(competitor_waiting_for_card,column_count-1));
      if(!res) QMessageBox::warning(0,tr("Card assign error"),
                                    tr("Card not assigned (already assigned?): ")+q.lastError().text(),
                                    QMessageBox::Ok);
    }
  }
  competitor_waiting_for_card=0;
}



//======================== START TIMES FORM ===============================//


void ProtocolTab::setStartTimes()
{
  StartTimesSetter ts(model,view->selectionModel()->selectedRows());
  ts.exec();
}

void ProtocolTab::clearStartTimes()
{
  QModelIndexList idx=view->selectionModel()->selectedRows();
  QString db_cmd;
  for(QModelIndex& i:idx)
    db_cmd.append(QString("%1,").arg(model->OSqlTableModel::getRowID(i)));
  if(db_cmd.isEmpty()) return;
  db_cmd.chop(1);
  db_cmd.append(")");
  QSqlQuery q;
  db_cmd="update protocol set start_time_forced=0,start_time_selector=0 where prot_id in ("+db_cmd;
  q.exec(db_cmd);
  model->select();
  ui_settings.protocol_change_notify();
}



StartTimesSetter::StartTimesSetter(OSqlTableModel * const prot_model, const QModelIndexList &idx_list, QWidget *parent)
  :QDialog(parent), idx(idx_list),model(prot_model)
{
  lout=new QFormLayout;
  de=new QDateTimeEdit(QDateTime::currentDateTime());
  te=new QTimeEdit;
  te->setDisplayFormat("mm:ss");
  de->setDisplayFormat("hh:mm:ss dd.MM.yyyy");
  bOk=new QPushButton("OK");
  lout->addRow(tr("Start time:"),de);
  lout->addRow(tr("Time step:"),te);
  lout->addWidget(bOk);
  setLayout(lout);
  setWindowTitle(tr("Protocol start times"));
  connect(bOk,SIGNAL(clicked(bool)),SLOT(setStartTimes()));
}

void StartTimesSetter::setStartTimes()
{
  qint64 start_time=de->dateTime().toMSecsSinceEpoch()/1000;
  int time_step=te->time().msecsSinceStartOfDay()/1000;
  qint64 rowid;
  QSqlDatabase db(QSqlDatabase::database());
  db.transaction();
  QSqlQuery q;
  for(QModelIndex i : idx)
    {
      rowid=model->OSqlTableModel::getRowID(i);
      q.exec(QString("update protocol set start_time_forced=%1,start_time_selector=1 where prot_id==%2")
             .arg(start_time)
             .arg(rowid));
      start_time+=time_step;
    }
  db.commit();
  model->select();
  ui_settings.protocol_change_notify();
  close();
}


