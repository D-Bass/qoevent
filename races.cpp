#include "races.h"
#include "uisettings.h"
#include <QAction>
#include <QSqlError>
#include <QDebug>
#include <QDateTime>
#include <QMessageBox>

struct
{
  QVariant event_id;
  QVariant race_id;
  QVariant name;
  QVariant location;
  QVariant date;
  QVariant officials;
  QVariant sort_order;
} race_data;


RaceListModel::RaceListModel(QObject *parent)
  : QAbstractItemModel(parent),
    event_id_filter(-1),idx_root(1024),idx(8192)
{
  query=new QSqlQuery();
  setEventFilter(ui_settings.current_event);
  select();
}

void RaceListModel::check_active_race(RaceIndex* const candidate)
{
  if(ui_settings.current_race==query->value(1).toInt())
    active_race=candidate;
}

void RaceListModel::select()
{
  qint64 new_id,id=-1;
  int32_t rows=0;
  int32_t line=0;
  active_race=nullptr;
  RaceIndex* parent=nullptr;
  idx_root.clear();
  idx.clear();
  beginResetModel();
  if(event_id_filter==0)
    query->exec("select eventid,race.race_id,eventname,race.name,race.location,strftime('%d.%m.%Y',race.date,'unixepoch'),count(prot_id),race.date,race.officials,race.sort_order from race left join (select event.event_id as eventid,event.name as eventname,min(race.date) as minracedate from event join race using(event_id) group by event.event_id) on race.event_id==eventid left join class using(race_id) left join team using(class_id) left join team_member using (team_id) left join protocol using(tm_id) group by race.race_id order by minracedate,eventid,race.date");
  else
    query->exec(QString("select eventid,race.race_id,eventname,race.name,race.location,strftime('%d.%m.%Y',race.date,'unixepoch'),count(prot_id),race.date,race.officials,race.sort_order from race left join (select event.event_id as eventid,event.name as eventname,min(race.date) as minracedate from event join race using(event_id) group by event.event_id) on race.event_id==eventid left join class using(race_id) left join team using(class_id) left join team_member using (team_id) left join protocol using(tm_id) group by race.race_id having eventid=='%1' order by minracedate,eventid,race.date").arg(event_id_filter));
  while(query->next())
  {
    new_id=query->value(0).toInt();
    if(new_id!=id)
    {
      if(new_id==0)
      {

        check_active_race(idx_root.end());
        idx_root.push_back(RaceIndex(line,0,nullptr,nullptr));
        id=-1;
        parent=nullptr;
      }
      else
      {
        if(parent) parent->rows=rows;
        rows=1;
        parent=idx_root.end();
        idx_root.push_back(RaceIndex(line,0,nullptr,idx.end()));

        check_active_race(idx.end());
        idx.push_back(RaceIndex(line,0,parent,nullptr));
        id=new_id;
      }
    }
    else {
      check_active_race(idx.end());
      idx.push_back(RaceIndex(line,0,parent,nullptr));
      ++rows;
    }

    ++line;
  }
  if(parent) parent->rows=rows;
  endResetModel();
}

QModelIndex RaceListModel::parent(const QModelIndex &child) const
{
  RaceIndex* i=static_cast<RaceIndex*> (child.internalPointer());
  if(i->parent==nullptr)
    return QModelIndex();
  else
    return createIndex( i->parent - idx_root.begin(),0,i->parent);
}

QModelIndex RaceListModel::index(int row, int column, const QModelIndex &parent) const
{
  if(idx_root.isEmpty()) return QModelIndex();
  if(parent==QModelIndex())
    return createIndex(row,column,(void*)(idx_root.begin()+row));
  else
    return createIndex(row,column,static_cast<RaceIndex*>(parent.internalPointer())->child+row);
}

QModelIndex RaceListModel::indexFromEvent(const qint64 event_id) const
{
  QVector<RaceIndex>::const_iterator i;
  for(i=idx_root.begin();i!=idx_root.end();i++)
  {
    query->seek(i->qline);
    if(query->value(0).toInt()==event_id && i->rows)
      return createIndex(i-idx_root.begin(),0,(void*)i);
  }
  return QModelIndex();
}

QModelIndex RaceListModel::indexFromRace(const qint64 race_id) const
{
  QVector<RaceIndex>::const_iterator i;
  for(i=idx_root.begin();i!=idx_root.end();i++)
  {
    query->seek(i->qline);
    if(query->value(1).toInt()==race_id && !i->rows)
      return createIndex(i-idx_root.begin(),0,(void*)i);
  }
  for(i=idx.begin();i!=idx.end();i++)
  {
    query->seek(i->qline);
    if(query->value(1).toInt()==race_id && !i->rows)
      return createIndex(i-i->parent->child,0,(void*)i);
  }
  return QModelIndex();
}

int RaceListModel::rowCount(const QModelIndex &parent) const
{
  if(!parent.isValid()) return idx_root.size();
  else
    return (static_cast<RaceIndex*>(parent.internalPointer()))->rows;
}

int RaceListModel::columnCount(const QModelIndex &parent) const
{
  return 4;
}


QVariant RaceListModel::data(const QModelIndex &index, int role) const
{
  RaceIndex* i=static_cast<RaceIndex*>(index.internalPointer());
  query->seek(i->qline);
  if(role==Qt::BackgroundColorRole)
  {
    if(i->rows)
      return QColor(255,255,200);
    else return (i==active_race ? QColor(255,200,200) : QColor(255,255,255));
  }

  if(role==Qt::DisplayRole)
  {
    if(i->rows)
      return query->value(2).toString();
    else return query->value(3+index.column()).toString();
  }
  return QVariant();
}


QVariantList RaceListModel::dataToList(const QModelIndex & index) const
{
  QVariantList list;
  RaceIndex* i=static_cast<RaceIndex*>(index.internalPointer());
  if(!i->rows)
  {
    query->seek(i->qline);
    list << query->value(0) << query->value(1) << query->value(3)
         << query->value(4) << query->value(7) << query->value(8)
         << query->value(9);
  }
  return list;
}

Qt::ItemFlags RaceListModel::flags(const QModelIndex &index) const
{
  return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant RaceListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if(orientation==Qt::Horizontal && role==Qt::DisplayRole)
  switch (section)
  {
    case 0: return QVariant(tr("Name"));
    case 1: return QVariant(tr("Location"));
    case 2: return QVariant(tr("Date"));
    case 3: return QVariant(tr("Competitors"));
  }
  return QVariant();
}

bool RaceListModel::removeRow(const QModelIndex &row)
{
  RaceIndex* idx=static_cast<RaceIndex*>(row.internalPointer());
  if(!query->seek(idx->qline)) return false;
  QString db_cmd=QString("delete from race where race_id=='%1'").arg(query->value(1).toString());
  qDebug() << db_cmd;
  return query->exec(db_cmd);
}

qint64 RaceListModel::getRaceID(const QModelIndex& index) const
{
  RaceIndex* idx=static_cast<RaceIndex*>(index.internalPointer());
  if(!query->seek(idx->qline)) return -1;
  return query->value(1).toInt();
}

qint64 RaceListModel::EventFromRace(QModelIndex& index) const
{
  RaceIndex* idx=static_cast<RaceIndex*>(index.internalPointer());
  if (idx->rows) return 0;    //we need race on input, not event
  if(!query->seek(idx->qline)) return 0;
  return query->value(0).toInt();
}

//============================= ADD/EDIT FORM ===========================//





RaceDataForm::RaceDataForm(bool createNew,QWidget *parent)
  : QDialog(parent),createNew(createNew)
{
  fl=new QFormLayout(this);

  QDateTime dtemp;
  dtemp.setMSecsSinceEpoch(race_data.date.toLongLong()*1000);

  leName=new QLineEdit;
  leLocation=new QLineEdit;
  deDate=new QDateEdit;
  deDate->setDisplayFormat("dd.MM.yyyy");
  teOfficials=new QPlainTextEdit;
  cbEvent=new QComboBox;
  cbEvent->setMaximumWidth(530);
  cbEvent->addItem("");
  cbFinishSortCriteria=new QComboBox;
  cbFinishSortCriteria->addItems({tr("Sort by score first"),tr("Sort by total time first")});

  query=new QSqlQuery();
  query->exec("select event.event_id,event.name,min(race.date) from event left join race using(event_id) group by event.name order by 3");
  while(query->next())
    cbEvent->addItem(query->value(1).toString());

  if(!createNew)
  {
    deDate->setDate(dtemp.date());
    leName->setText(race_data.name.toString());
    leLocation->setText(race_data.location.toString());
    teOfficials->setPlainText(race_data.officials.toString());
    cbFinishSortCriteria->setCurrentIndex(race_data.sort_order.toInt());
    if(race_data.event_id.toInt()!=0)
    {
      query->seek(-1);
      while(query->next())
        if(query->value(0)==race_data.event_id.toInt())
          cbEvent->setCurrentIndex(query->at()+1);
    }
  }
  else
  {
    deDate->setDate(QDate::currentDate());
    teOfficials->setPlainText(tr("Organizer: \nMain secretary: \nMain judge:\n"));
  }

  flRaceButtons=new QHBoxLayout;
  bOk=new QPushButton(tr("OK"));
  bCancel=new QPushButton(tr("Cancel"));
  bOk->setMinimumWidth(200);
  bCancel->setMinimumWidth(200);
  flRaceButtons->addWidget(bOk,1,Qt::AlignCenter);
  flRaceButtons->addWidget(bCancel,1,Qt::AlignCenter);
  setModal(true);
  setWindowTitle(createNew ? tr("Add race") : tr("Edit race"));
  fl->addRow(tr("Event:"),cbEvent);
  fl->addRow(tr("Name:"),leName);
  fl->addRow(tr("Race location:"),leLocation);
  fl->addRow(tr("Date:"),deDate);
  fl->addRow(tr("Finish result\nsort order:"),cbFinishSortCriteria);
  fl->addRow(tr("Officials:"),teOfficials);
  fl->setLabelAlignment(Qt::AlignLeft);
  fl->setSpacing(10);
  fl->addRow(flRaceButtons);
  setContentsMargins(10,10,10,10);
  flRaceButtons->setContentsMargins(0,20,0,0);
  fl->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
  connect(bCancel,SIGNAL(clicked()),SLOT(reject()));
  connect(bOk,SIGNAL(clicked()),SLOT(form_result()));
  connect(leName,SIGNAL(textEdited(QString)),SLOT(clear_warning()));
}

RaceDataForm::~RaceDataForm()
{
  delete query;
  qDebug() << "query dtor";
}


void RaceDataForm::form_result()
{
  QDateTime d1,d2;
  d1.setTimeSpec(Qt::UTC);
  d1.setTime_t(0);
  d2=deDate->dateTime();
  d2.setTimeSpec(Qt::UTC);

  qDebug() << d1;
  qDebug() << d2;
  race_data.name=leName->text();
  if (race_data.name==QString())
    {leName->setPlaceholderText(tr("Name is empty"));return;}
  race_data.location=leLocation->text().isEmpty() ? "NULL" : leLocation->text();
  race_data.date=d1.secsTo(d2);
  race_data.officials=teOfficials->document()->toPlainText();
  race_data.sort_order=cbFinishSortCriteria->currentIndex();
  race_data.event_id=QString("NULL");
  if(cbEvent->currentIndex())
    if(query->seek(cbEvent->currentIndex()-1))
      race_data.event_id=query->value(0);
  accept();
}

void RaceDataForm::clear_warning()
{
  leName->setPlaceholderText("");
}




//====================== TAB ==========================//




RaceTab::RaceTab(QWidget *parent) : QWidget(parent)
{
  QVector<int> w{150,150,90,50};
  bAdd=new QPushButton(QIcon(":/icons/landscape.png"),tr("Add new"));
  bEdit=new QPushButton(QIcon(":/icons/edit-pencil.png"),tr("Edit"));
  bActivate=new QPushButton(QIcon(":/icons/check.png"),tr("Make active"));
  bAdd->setIconSize(QSize(32,20));
  bEdit->setIconSize(QSize(32,20));
  bActivate->setIconSize(QSize(32,20));

  chFilter=new QCheckBox(tr("Current event only"));
  model=new RaceListModel(this);
  view=new QTreeView;
  view->setModel(model);
  view->setSelectionBehavior(QAbstractItemView::SelectRows);
  for(int i=0;i<4;i++) view->setColumnWidth(i,w.at(i));
  model->select();
  setup_columns();
  layoutV=new QVBoxLayout;
  layoutH=new QHBoxLayout;
  layoutH->addWidget(bAdd);
  layoutH->addWidget(bEdit);
  layoutH->addWidget(bActivate);
  layoutV->addWidget(chFilter);
  layoutV->addWidget(view);
  layoutV->addLayout(layoutH);
  layoutH->setAlignment(Qt::AlignLeft);
  setLayout(layoutV);
  connect(bAdd,SIGNAL(clicked()),SLOT(create_race_dialog()));
  connect(bEdit,SIGNAL(clicked()),SLOT(edit_race_dialog()));
  connect(view,SIGNAL(doubleClicked(QModelIndex)),SLOT(edit_race_dialog()));
  connect(bActivate,SIGNAL(clicked()),SLOT(activate_race()));
  connect(chFilter,SIGNAL(stateChanged(int)),SLOT(update_page()));
  connect(&ui_settings,SIGNAL(events_changed()),SLOT(update_page()));
  connect(&ui_settings,SIGNAL(protocol_changed()),SLOT(update_page()));
}

void RaceTab::setup_columns()
{
  //span event name to all columns
  for(int i=0;i<model->rowCount(QModelIndex());i++) {
      if(model->rowCount(model->index(i,0,QModelIndex())))
          view->setFirstColumnSpanned(i,QModelIndex(),true);
    }
  view->expandAll();

}

void RaceTab::edit_race(bool create_new)
{
  RaceDataForm rdf(create_new);
  if(rdf.exec())
  {
    QSqlQuery q;
    QString db_cmd;
    if(create_new)
      db_cmd=QString(
        "insert into race (name,event_id,location,date,officials,sort_order) "
        "values ('%1','%2','%3','%4','%5',%6)")
        .arg(race_data.name.toString())
        .arg(race_data.event_id.toString())
        .arg(race_data.location.toString())
        .arg(race_data.date.toString())
        .arg(race_data.officials.toString())
        .arg(race_data.sort_order.toString());
    else
      db_cmd=QString(
        "update race set name='%1', event_id='%2', location='%3', date='%4', officials='%5' ,sort_order=%6 "
        "where race_id=%7")
        .arg(race_data.name.toString())
        .arg(race_data.event_id.toString())
        .arg(race_data.location.toString())
        .arg(race_data.date.toString())
        .arg(race_data.officials.toString())
        .arg(race_data.sort_order.toString())
        .arg(race_data.race_id.toString());
    db_cmd.replace("'NULL'","NULL");
    qDebug() << db_cmd;
    if(!q.exec(db_cmd)) QMessageBox::information(this,create_new ? tr("Add race") : tr("Edit race"),
                                                 tr("Failed operation: ")+q.lastError().text(),
                                                 QMessageBox::Ok);
    model->select();
    setup_columns();
  }
}


void RaceTab::create_race_dialog()
{
  edit_race(true);
  view->setCurrentIndex(model->indexFromEvent(race_data.event_id.toInt()));
}

void RaceTab::edit_race_dialog()
{
  QModelIndex vi=view->currentIndex();
  if(!vi.isValid()) return;
  qint64 race_id=model->getRaceID(vi);

  if(!vi.isValid()) return;
  QVariantList list=model->dataToList(vi);
  if (list.isEmpty()) return;
  race_data.event_id=list.takeFirst();
  race_data.race_id=list.takeFirst();
  race_data.name=list.takeFirst();
  race_data.location=list.takeFirst();
  race_data.date=list.takeFirst();
  race_data.officials=list.takeFirst();
  race_data.sort_order=list.takeFirst();
  edit_race(false);
  vi=model->indexFromRace(race_id);
  qDebug() << vi;
  view->setCurrentIndex(vi);
}


void RaceTab::remove_race()
{
  QModelIndex vi=view->currentIndex();
  if (!vi.isValid()) return;
  qint64 event_id=model->EventFromRace(vi);
  qint64 race_id=model->getRaceID(vi);
  RaceIndex* idx=static_cast<RaceIndex*>(vi.internalPointer());
  if(idx->rows) return; //if (index points at event, not race)
  QSqlQuery q;

  if(QMessageBox::question(this,tr("Remove race"),
                  tr("Wipe out the whole race including protocol?"),
                  QMessageBox::Yes | QMessageBox::No)==QMessageBox::No)
     return;

  q.exec(QString("delete from team_member where team_member.team_id in (select team_id from team join class using(class_id) where race_id==%1) or team_member.team_id is NULL").arg(race_id));
  q.exec(QString("delete from team where team_id in (select team_id from team join class using(class_id) where race_id==%1)").arg(race_id));
  q.exec(QString("delete from class  where race_id==%1").arg(race_id));
  q.exec(QString("delete from course where race_id==%1").arg(race_id));
  q.exec(QString("delete from cp where race_id==%1").arg(race_id));


  if(!model->removeRow(vi))
    QMessageBox::information(this,tr("Remove race"),
                             tr("Failed operation. Clear class/course assignment table by hand"),
                             QMessageBox::Ok);
  else {
    if(ui_settings.current_race==race_id)
    ui_settings.set_current_race(0);
    ui_settings.races_change_notify();
    ui_settings.classes_change_notify();
    ui_settings.courses_change_notify();
    ui_settings.cp_change_notify();
    }
  model->select();
  setup_columns();
  view->setCurrentIndex(model->indexFromEvent(event_id));
}


void RaceTab::clear_race_protocol()
{
  qint64 race_id;
  QModelIndex vi=view->currentIndex();
  if (!vi.isValid()) return;
  RaceIndex* idx=static_cast<RaceIndex*>(vi.internalPointer());
  if(idx->rows) return; //if (index points at event, not race)
  if ((race_id=model->getRaceID(vi))==-1) {
      QMessageBox::information(this,tr("Clear protocol"),
                                tr("Failed to get race ID"),
                                 QMessageBox::Ok);
      return;
  }

  if(QMessageBox::question(this,tr("Remove protocol"),
                  tr("Are you sure to erase all protocol data?"),
                  QMessageBox::Yes | QMessageBox::No)==QMessageBox::No)
    return;

  QSqlQuery q;
  bool res=q.exec(QString("delete from team_member where team_member.team_id in (select team_id from team join class using(class_id) where race_id==%1) or team_member.team_id is NULL").arg(race_id));
  res&=q.exec(QString("delete from team where team_id in (select team_id from team join class using(class_id) where race_id==%1)").arg(race_id));

  model->select();
  setup_columns();
  ui_settings.protocol_change_notify();
}


void RaceTab::update_page()
{
  if(chFilter->isChecked() && ui_settings.current_event!=0)
    model->setEventFilter(ui_settings.current_event);
  else
    model->setEventFilter(0);
  model->select();
  setup_columns();
}


void RaceTab::activate_race()
{
  QModelIndex vi=view->currentIndex();
  if(!vi.isValid()) return;
  RaceIndex* idx=static_cast<RaceIndex*>(view->currentIndex().internalPointer());
  if(idx->rows) return;

  ui_settings.set_current_race(model->getRaceID(vi));
  qDebug() << "acticate race" << ui_settings.current_race;
  model->select();
  setup_columns();
  view->setCurrentIndex(vi);
  qDebug() << "ui notify";
  ui_settings.races_change_notify();
}


void RaceTab::import_from_race(Import what)
{
  if(!ui_settings.current_race) {QMessageBox::warning(this,tr("Import from race"),
                            tr("No active race"),
                             QMessageBox::Ok);return;}
  RaceIndex* idx=static_cast<RaceIndex*>(view->currentIndex().internalPointer());
  if(idx->rows) return;
  qint64 from=model->getRaceID(view->currentIndex());
  if(from==ui_settings.current_race) return;
  QSqlQuery q;

  if(what==ImportCP)
  {
    if(QMessageBox::question(this,tr("Import cp from race"),
                    tr("Import cp FROM selected race INTO active race?"),
                    QMessageBox::Yes | QMessageBox::No)==QMessageBox::Yes)
      if(!q.exec(QString("insert into cp (race_id,int_number,value,map_alias,lat,lon,x,y,desc)  select %1,int_number,value,map_alias,lat,lon,x,y,desc from cp where race_id==%2")
           .arg(ui_settings.current_race)
           .arg(from)))
          QMessageBox::information(this,tr("Import CP error"),
                                   tr("Database error(same cp exists?): ")+q.lastError().text(),
                                   QMessageBox::Ok);
    ui_settings.cp_change_notify();
  }
  if(what==ImportClasses)
  {
    if(QMessageBox::question(this,tr("Import classes from race"),
                    tr("Import classes FROM selected race INTO active race?"),
                    QMessageBox::Yes | QMessageBox::No)==QMessageBox::Yes)
      if(!q.exec(QString("insert into class (race_id,name) select %1,name from class where race_id==%2")
           .arg(ui_settings.current_race)
           .arg(from)))
          QMessageBox::information(this,tr("Import classes error"),
                                   tr("Database error(same classes exists?):")+q.lastError().text(),
                                   QMessageBox::Ok);
    ui_settings.classes_change_notify();
  }
}

void RaceTab::close_protocol()
{
  QModelIndex vi=view->currentIndex();
  if (!vi.isValid()) return;
  qint64 race_id=model->getRaceID(vi);
  RaceIndex* idx=static_cast<RaceIndex*>(vi.internalPointer());
  if(idx->rows) return; //if (index points at event, not race)

  if(QMessageBox::question(this,tr("Close protocol"),
                  tr("Protect protocol from futher changes?"),
                  QMessageBox::Yes | QMessageBox::No)==QMessageBox::No)
    return;
  QSqlQuery q;
  q.exec(QString("update race set protocol_closed=1 where race_id==%1").arg(race_id));
  ui_settings.races_dirty=true;
  ui_settings.races_change_notify();
}
