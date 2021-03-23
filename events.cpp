#include "events.h"
#include "uisettings.h"
#include <QDebug>
#include <QHeaderView>
#include <QColor>
#include <QMessageBox>
#include <QInputDialog>

Events events;

EventListModel::EventListModel(QObject *parent) :
  OSqlTableModel(6,0,"delete from event where event_id==%1",parent)
{
  select();
}



void EventListModel::selectSoft()
{
  //final order parameter to insure that newly created record would locate at row 0
  query->exec("select event.event_id,event.name,count(race_id),strftime('%d.%m.%Y',date(min(race.date),'unixepoch')) || ' - ' || strftime('%d.%m.%Y',date(max(race.date),'unixepoch')),group_concat(race.location,', ')"
              ", event.multirace from event left join race using(event_id) group by event.event_id order by min(race.date),event.event_id desc");
}

Qt::ItemFlags EventListModel::flags(const QModelIndex &index) const
{
  if (!index.isValid()) return Qt::NoItemFlags;
  if(index.column()==1)
    return Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren;
  else
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren;
}

QVariant EventListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if(orientation==Qt::Horizontal && role==Qt::DisplayRole)
    switch(section)
    {
    case 1: return QVariant(tr("Name"));
    case 2: return QVariant(tr("Races count"));
    case 3: return QVariant(tr("Dates"));
    case 4: return QVariant(tr("Locations"));
    case 5: return QVariant(tr("Multirace"));
    }
  return QVariant();
}

QVariant EventListModel::data(const QModelIndex &index, int role) const
{

  if (!index.isValid()) return QVariant();
  int row=index.row();int col=index.column();

  if (!query->seek(row)) return QVariant();

  if(role==Qt::BackgroundRole && query->value(3)=="")
    return QColor(255,255,200);

  if(role==Qt::CheckStateRole && col==5)
    return query->value(5).toBool();

  if(role==Qt::TextAlignmentRole && col==2)
    return QVariant(Qt::AlignHCenter);

  if( (role==Qt::DisplayRole || role==Qt::EditRole) && col!=5)
  {
    if (col==2 && query->value(2)==0)
        return "";
    return query->value(col).toString();
  }
  return QVariant();
}

bool EventListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
  if (!index.isValid() || role!=Qt::EditRole) return false;
  if (!query->seek(index.row())) return false;
  QString db_cmd(QString("update event set name='%1' where event_id==%2").arg(value.toString()).arg(query->value(0).toString()));
  qDebug()<<db_cmd;
  query->exec(db_cmd);
  selectSoft();
  ui_settings.events_dirty=true;
  emit resizeRow(index.row());
  return true;
}


void EventListModel::addRow(const QString name,const bool multirace)
{
  query->exec(QString("insert into event (name,multirace) values('%1','%2')")
              .arg(name)
              .arg(multirace));
  select();
}


//======================== EVENT LIST ==========================//








EventList::EventList(QWidget *parent)
  : QDialog(parent)
{
  QVector<int> w{0,250,120,120,180,40};
  layoutV=new QVBoxLayout;
  layoutH=new QHBoxLayout;
  view=new EventListView;
  model=new EventListModel(this);
  view->setModel(model);
  layoutV->addWidget(view);
  layoutV->addLayout(layoutH);
  layoutH->addWidget(bAdd=new QPushButton(tr("Create new event")));
  layoutH->addWidget(bRemove=new QPushButton(tr("Erase event")));
  layoutH->addWidget(bSwitch=new QPushButton(tr("Switch to event")));
  setLayout(layoutV);
  view->horizontalHeader()->setStretchLastSection(true);
  view->setColumnWidth(1,200);
  view->setWordWrap(true);
  view->setSelectionBehavior(QAbstractItemView::SelectRows);
  view->hideColumn(0);
  for(int i=1;i<6;i++) view->setColumnWidth(i,w.at(i));
  view->resizeRowsToContents();
  setModal(true);
  setWindowTitle(tr("Event list"));
  setWindowIcon(QIcon(":/micon.gif"));
  connect(bSwitch,SIGNAL(clicked(bool)),SLOT(switch_event()));
  connect(bRemove,SIGNAL(clicked(bool)),SLOT(remove_event()));
  connect(bAdd,SIGNAL(clicked(bool)),SLOT(create_event()));
  connect(model,SIGNAL(resizeRow(int)),view,SLOT(resizeRowToContents(int)));
}

void EventList::switch_event()
{

  if(!view->currentIndex().isValid()) return;
  int b=QMessageBox::information(this,tr("Event change"),
                                 tr("Switch to selected event?"),
                                 QMessageBox::Ok | QMessageBox::Cancel);
  if (b!=QMessageBox::Ok) return;
  ui_settings.set_current_event(model->getRowID(view->currentIndex()));
  qDebug() << ui_settings.current_event;
}

void EventList::remove_event()
{
  if(!view->currentIndex().isValid()) return;
  int b=QMessageBox::warning(this,tr("Event remove"),
                             tr("Are you sure to remove event?"),
                             QMessageBox::Ok | QMessageBox::Cancel);
  if (b!=QMessageBox::Ok) return;
  if(!model->removeRow(view->currentIndex().row()))
    QMessageBox::information(this,tr("Event remove"),
                             tr("Failed operation \n(event contains races?)"),
                             QMessageBox::Ok);
}


void EventList::create_event()
{
  QInputDialog dlg(this,Qt::Dialog);
  QString name;
  dlg.resize(400,300);
  dlg.setModal(true);
  dlg.setLabelText(tr("Event Name"));
  dlg.setWindowTitle(tr("Event creation"));
  dlg.setInputMode(QInputDialog::TextInput);

  if (dlg.exec() && (name=dlg.textValue())!="")
  {
    int m=QMessageBox::question(this,tr("Create event"),
                               tr("Multirace? (intra-race score calculation)"),
                               QMessageBox::Yes | QMessageBox::No);
    if(m==QMessageBox::Yes) m=1;else m=0;
    model->addRow(name,m);
    view->setCurrentIndex(model->lastInserted());
  }
}


//=========================== EVENTS ===============================//






void Events::edit()
{
  EventList list;
  list.exec();
  ui_settings.events_change_notify();
  qDebug() << "list vse";
}
