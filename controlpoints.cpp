#include "controlpoints.h"
#include "comm.h"
#include <QHeaderView>
#include <cmath>
#include <QDebug>
#include "uisettings.h"
#include <QKeyEvent>
#include <QDateTime>
#include <QTimeZone>

QWidget* tabCP;

QHash<int,QString> map_alias_of_int_num;
//QHash<uint8_t,qint64> rowid_of_int_num;

CPModel::CPModel(QObject* parent)
  : OSqlTableModel(6,6,"delete from cp where cp_id==%1",parent),
    geo_icon(":/icons/globus.png")
{
  select();
}


void CPModel::selectSoft()
{
  ControlPoints::cp_point p;
  query->exec(QString("select map_alias,int_number,(x or y) or (lat and lon) ,value,penalty,desc,cp_id,lat,lon,x,y from cp where race_id==%1 order by map_alias")
              .arg(ui_settings.current_race));
  map_alias_of_int_num.clear();
  coordinates.clear();
  //rowid_of_int_num.clear();
  while(query->next()) {
    map_alias_of_int_num.insert(query->value(1).toInt(),query->value(0).toString());
    p.lat=query->value(7).toDouble();
    p.lon=query->value(8).toDouble();
    p.x=query->value(9).toDouble();
    p.y=query->value(10).toDouble();
    coordinates.insert(query->value(rowid_column).toLongLong(),p);
    //rowid_of_int_num.insert(query->value(1).toString(),query->value(rowid_column).toLongLong());
  }
}


QVariant CPModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if(role!=Qt::DisplayRole || orientation!=Qt::Horizontal) return QVariant();
  switch(section)
    {
    case 0: return QString(tr("Map name"));
    case 1: return QString(tr("CP ID"));
    case 2: return QString(tr("GeoRef"));
    case 3: return QString(tr("Value"));
    case 4: return QString(tr("Penalty"));
    case 5: return QString(tr("Description"));
    }
  return QVariant();
}

QVariant CPModel::data(const QModelIndex &index, int role) const
{
  int row=index.row();int column=index.column();
  query->seek(row);
  if(role==Qt::DisplayRole || role==Qt::EditRole)
    if(column!=2)
      return query->value(column).toString();
  if(role==Qt::ToolTipRole && column==5) return query->value(4).toString();
  if(role==Qt::DecorationRole && column==2 && query->value(2).toInt())
    return geo_icon;
  return QVariant();
}

Qt::ItemFlags CPModel::flags(const QModelIndex &index) const
{
  if(index.column()==2)
    return Qt::ItemIsEnabled;
  else
    return Qt::ItemIsEnabled|Qt::ItemIsEditable|Qt::ItemIsSelectable;
}


bool CPModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
  if(!index.isValid()) return false;
  int row=index.row();int column=index.column();
  if(role!=Qt::EditRole) return false;
  bool res;
  query->seek(row);
  last_edited_rowid=query->value(rowid_column).toLongLong();
  last_edited_column=column;
  QString db_cmd=QString("update cp set %1='%2' where cp_id==%3")
      .arg(db_field.at(column))
      .arg(value.toString())
      .arg(last_edited_rowid);

    if(column==1 && (value.toInt()<1 || value.toInt()>254)) return false;

  res=query->exec(db_cmd);
  selectSoft();
  if(res) dataChanged(index,index,{Qt::DisplayRole});
  return true;
}


bool CPModel::insertRow(const QString text)
{
  bool res;
  res=query->exec(QString("insert into cp(map_alias,race_id) values('%1',%2)")
     .arg(text)
     .arg(ui_settings.current_race));
  notifyInsertedRow(res);
  return res;
}



QString CPModel::int_num(const QModelIndex& idx) const
{
  query->seek(idx.row());
  if(query->value(0).toString().contains("S"))
    return "S";
  return query->value(1).toString();
}






//============================== TAB =========================================//




CPTab::CPTab(QWidget *parent) : QWidget(parent)
{
  QVector<int> w{80,60,65,65,65};
  view=new OTableView;
  model=new CPModel(this);
  view->setSelectionMode(QAbstractItemView::SingleSelection);
  view->horizontalHeader()->setStretchLastSection(true);
  view->setModel(model);
  view->setValidator("[0-9A-Za-z_]+");
  view->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  for(int i=0;i<5;i++) view->setColumnWidth(i,w.at(i));
  lTabH = new QHBoxLayout;
  lTabV=new QVBoxLayout;
  bCPAdd=new QPushButton(QIcon(":/icons/add-control-point.png"),"");
  bCPRemove=new QPushButton(QIcon(":/icons/remove-control-point.png"),"");
  bCPAssignValue=new QPushButton(QIcon(":/icons/assign-value.png"),"");
  bCPAdd->setIconSize(QSize(32,24));
  bCPRemove->setIconSize(QSize(32,24));
  bCPAssignValue->setIconSize(QSize(40,24));
  bCPAdd->setToolTip(tr("<font color='gray'>New control point</font><b> Ins</b>"));
  bCPRemove->setToolTip(tr("<font color='gray'>Remove control point</font><b> Del</b>"));
  bCPAssignValue->setToolTip(tr("Assign \"Value\" to all courses"));




  lCP=new QGridLayout;
  lCP->addWidget(view,0,0,1,3);
  lCP->setColumnStretch(2,5);
  lCP->addWidget(bCPAdd,1,0,1,1,Qt::AlignLeft);
  lCP->addWidget(bCPRemove,1,1,1,1,Qt::AlignLeft);
  lCP->addWidget(bCPAssignValue,1,2,1,1,Qt::AlignLeft);

  grpState=new QGroupBox(tr("Control station"));
  lState=new QVBoxLayout;
  viewstate=new QTextEdit;
  bState=new QPushButton(tr("Get State"));
  lState->addWidget(viewstate);
  lState->addWidget(bState,0,Qt::AlignHCenter);
  bState->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Fixed);
  lState->setSizeConstraint(QLayout::SetMinimumSize);
  grpState->setLayout(lState);

  grpConfig=new QGroupBox;
  lConfig=new QGridLayout;
  chShutdown=new QCheckBox(tr("Shutdown"));
  bShutdown=new QPushButton(tr("Shutdown Now"));
  chSynch=new QCheckBox(tr("Synch PC clock"));
  bSynch=new QPushButton(tr("Synch now"));
  chActiveTime=new QCheckBox(tr("Active time"));
  sbActiveTime=new QSpinBox;
  bActiveTime=new QPushButton(tr("Set active time"));
  leCPNum=new QLineEdit;
  bCPNum=new QPushButton(tr("<-Set CP internal number"));
  lbBatch=new QLabel(tr("Batch settings"));
  lbNow=new QLabel(tr("Immediate"));
  lbBatch->setFont(QFont("Tahoma",10,QFont::Bold));
  lbNow->setFont(QFont("Tahoma",10,QFont::Bold));
  sbActiveTime->setRange(1,65535);
  //lConfig->addWidget(lbBatch,0,0,Qt::AlignHCenter);
  lConfig->addWidget(lbNow,0,2,Qt::AlignHCenter);
  //lConfig->addWidget(chSynch,1,0);
  lConfig->addWidget(bSynch,1,2);
  //lConfig->addWidget(chActiveTime,2,0);
  lConfig->addWidget(sbActiveTime,2,1);
  lConfig->addWidget(bActiveTime,2,2);
  //lConfig->addWidget(chShutdown,3,0);
  lConfig->addWidget(leCPNum,4,1);
  lConfig->addWidget(bCPNum,4,2);
  lConfig->addWidget(bShutdown,3,2);
  lConfig->setVerticalSpacing(10);
  grpConfig->setLayout(lConfig);



  lTabV->addWidget(grpState);
  lTabV->addWidget(grpConfig);
  lTabH->addLayout(lCP,2);
  lTabH->addLayout(lTabV,1);
  setLayout(lTabH);
  setCtrlEnabled(false);
  connect(&ui_settings,SIGNAL(race_changed()),SLOT(race_changed()));
  connect(&ui_settings,SIGNAL(cp_changed()),model,SLOT(select()));
  connect(&ui_settings,SIGNAL(serial_changed()),SLOT(serial_changed()));
  connect(view,SIGNAL(activated(QModelIndex)),view,SLOT(edit(QModelIndex)));
  connect(bState,SIGNAL(clicked()),SLOT(request_cp_state()));
  connect(bSynch,SIGNAL(clicked()),SLOT(set_cp_time()));
  connect(bActiveTime,SIGNAL(clicked()),SLOT(set_cp_active_timeout()));
  connect(bShutdown,SIGNAL(clicked()),SLOT(shutdown_cp()));
  connect(bCPNum,SIGNAL(clicked()),SLOT(set_cp_mode()));
  connect(bCPAdd,SIGNAL(clicked()),SLOT(addCP()));
  connect(bCPRemove,SIGNAL(clicked()),SLOT(removeCP()));
  connect(bCPAssignValue,SIGNAL(clicked()),SLOT(assignValuesCP()));

  connect(view->selectionModel(),SIGNAL(currentChanged(QModelIndex,QModelIndex)),SLOT(cp_int_num_update()));
  connect(view->selectionModel(),SIGNAL(currentChanged(QModelIndex,QModelIndex)),model,SLOT(currentChanged(QModelIndex,QModelIndex)));
  connect(model,SIGNAL(dataChanged(QModelIndex,QModelIndex,QVector<int>)),SLOT(cp_int_num_update()));
  race_changed();
}

void CPTab::keyPressEvent(QKeyEvent *event)
{

  if(view->hasFocus())
  {
    switch(event->key())
    {
    case Qt::Key_Delete:
      removeCP();
      break;
    case Qt::Key_Insert:
      addCP();
      break;
    }

  }
  QWidget::keyPressEvent(event);
}


void CPTab::addCP()
{
  if(ui_settings.current_race)
  view->enterNewEntry();
}

void CPTab::removeCP()
{
  if(!ui_settings.current_race) return;
  if(!view->currentIndex().isValid()) return;
  model->removeRow(view->currentIndex().row());
}

void CPTab::assignValuesCP()
{
  QSqlQuery q;
  q.exec(QString("update course_data set value=(select cp.value from cp where cp.cp_id==course_data.cp_id) where cp_id in (select cp_id from cp where race_id==%1)")
         .arg(ui_settings.current_race));
  q.exec(QString("update course_data set penalty=(select cp.penalty from cp where cp.cp_id==course_data.cp_id) where cp_id in (select cp_id from cp where race_id==%1)")
         .arg(ui_settings.current_race));
  ui_settings.courses_change_notify();
}


void CPTab::race_changed()
{
  if(ui_settings.current_race)
  {
    model->select();
    bCPAdd->setDisabled(false);
    bCPRemove->setDisabled(false);
    bCPAssignValue->setDisabled(false);
  }
  else
  {
    bCPAdd->setDisabled(true);
    bCPRemove->setDisabled(true);
    bCPAssignValue->setDisabled(true);
  }
}



void CPTab::cp_int_num_update()
{
  leCPNum->setText(model->int_num(view->currentIndex()));
}





//====================== CP COMMANDS AND STATUS ======================//

void CPTab::request_cp_state()
{
  if(bs->sendCmd(QByteArray::fromRawData("\x52",1),1000))
    connect(bs,SIGNAL(comm_result(QByteArray)),SLOT(cp_state_arrived(QByteArray)));
}


void CPTab::set_cp_time() {
  int i;
  QByteArray cmd=QByteArray::fromRawData("\x50",1);
  unsigned char t[28];
  QString s=QDateTime::currentDateTimeUtc().toString("ss;mm;hh;dd;00;MM;yy");
  s=s.replace(';',"").toLatin1();
  memcpy(t,s.data(),28);
  for(i=0 ; i<28 ; i+=2) t[i]-=0x30;
  for(i=0 ; i<28 ; i+=4) cmd.append(t[i]*16+t[i+2]);
  bs->sendCmd(cmd,1000);
}


void CPTab::set_cp_mode() {
  //such a contruction to force deep copy
  QByteArray cmd=QByteArray::fromRawData("\x51\0\0",3);
  if(leCPNum->text()=="S") cmd[1]=MODE_START;
  else {
    cmd[1]=MODE_CP;
    cmd[2]=leCPNum->text().toInt();
    if((uint8_t)cmd[2]==0 || (uint8_t)cmd[2]>254) return;
  }
  bs->sendCmd(cmd,1000);
}

void CPTab::set_cp_active_timeout() {
  QByteArray cmd=QByteArray::fromRawData("\x53\0\0",3);
  *(uint16_t*)(cmd.data()+1)=sbActiveTime->value();
  bs->sendCmd(cmd,1000);
}

void CPTab::shutdown_cp() {
  bs->sendCmd(QByteArray::fromRawData("\x54",1),1000);
}

void CPTab::cp_state_arrived(QByteArray val)
{
  disconnect(bs,SIGNAL(comm_result(QByteArray)),this,SLOT(cp_state_arrived(QByteArray)));
  if(val.isEmpty()) {viewstate->setPlainText(tr("CP didn't answer"));return;}
  const uint8_t* v=reinterpret_cast<const unsigned char*>(val.constData());
  QString text;
  if(v[2]==MODE_CP) {
    text.append(tr("MODE: Control point\n"));
    text.append(QString(tr("CP internal number: %1\n")).arg(v[3]));
  }
  if(val.at(2)==MODE_START)text.append(tr("MODE: Start\n"));
  text.append(tr("Time/Date: "));
  text.append(
        QDateTime::fromMSecsSinceEpoch((quint64)(*(uint32_t*)(v+4))*1000,QTimeZone::systemTimeZone())
        .toString("hh:mm:ss dd.MM.yyyy")
        );
  text.append(QString(tr("\nVoltage: %1V\n")).arg(*(uint16_t*)(v+8)*0.01));
  text.append(QString(tr("Temp sensor: %1\n")).arg(*(uint16_t*)(v+10)));
  text.append(QString(tr("Active mode timeout:  %1\n")).arg(*(uint16_t*)(v+14)));
  text.append(QString(tr("Software version: %1\n")).arg(*(uint16_t*)(v+12)));
  text.append(QString(tr("Clock PPM counter: %1\n")).arg(*(int32_t*)(v+16)));
  text.append(QString(tr("Clock correction counter: %1\n")).arg(*(int32_t*)(v+20)));
  text.append(QString(tr("Serial #: %1\n")).arg(*(int32_t*)(v+24)));
  viewstate->setPlainText(text);
}


void CPTab::serial_changed()
{
  setCtrlEnabled(ui_settings.serial_ready);
}

//============================ CP HELPER ===========================//
int ControlPoints::distance(cp_point p1, cp_point p2)
{
  const double pi_mult=3.14159265358/180.0;
  if(!p1 || !p2) return 0;
  if(p2.lat==0)
    return sqrt(pow(p2.x-p1.x,2)+pow(p2.y-p1.y,2));
  p1.lat*=pi_mult;p1.lon*=pi_mult;
  p2.lat*=pi_mult;p2.lon*=pi_mult;
  double dlon = p1.lon - p2.lon;
  double dlat = p1.lat - p2.lat;
  double a = sqrt(pow(sin(dlat/2),2) + cos(p1.lat) * cos(p2.lat) * pow(sin(dlon/2),2));
  a=asin(a)*2*6371000;
  return a;
}
