#include "competitors.h"
#include "uisettings.h"
#include <QTableView>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QSqlDatabase>
#include <QSqlError>
#include <memory>
#include <QDebug>
#include <QHeaderView>
#include <QKeyEvent>



CompetitorsModel::CompetitorsModel(QObject* parent)
  : OSqlTableModel(6,6,"delete from competitor where comp_id==%1",parent)
{
  select();
}


void CompetitorsModel::selectSoft()
{
  QHash <qint64,QString> names;

  QString sort_clause;
  if(sort_column>=0 && sort_column<2) sort_clause_num=sort_column;
  if(sort_clause_num==0) sort_clause=" order by competitor.name";
  if(sort_clause_num==1) sort_clause=" order by birthdate";
  if(ui_settings.current_race)
    query->exec(QString("select competitor.name,birthdate,city,region,club,phone,competitor.comp_id,coalesce(pr_in_race,0) from competitor left join (select 1 as pr_in_race,team_member.comp_id from team_member join team using(team_id) join class using (class_id) where race_id==%1) using (comp_id)" + sort_clause)
              .arg(ui_settings.current_race));
  else query->exec(QString("select competitor.name,birthdate,city,region,club,phone,competitor.comp_id,0 from competitor" + sort_clause));
  /*
  QString name;
  QStringList nn;
  while(query->next())
    {
      name=query->value(0).toString();
      nn=name.split(' ');
      name=nn.at(1);name.append(" ");name.append(nn.at(0));
      names.insert(query->value(rowid_column).toLongLong(),name);
    }
  QHash<qint64,QString>::const_iterator x;
  for(x=names.constBegin();x!=names.constEnd();x++)
    query->exec(QString("update competitor set name='%1' where comp_id==%2")
                .arg(x.value())
                .arg(x.key()));

*/
}



QVariant CompetitorsModel::data(const QModelIndex &index, int role) const
{
  int col=index.column();
  int val;
  query->seek(index.row());
  switch(role)
  {
    case Qt::DisplayRole:
      if(col<=4)
        return query->value(col);
      break;
    case Qt::EditRole:
      return query->value(col);
    case Qt::ToolTipRole:
      if(col==5) return query->value(5);
      break;
    case Qt::CheckStateRole:
      if(col==5) return query->value(5).toString()!="";
      break;
    case Qt::BackgroundRole:
      val=query->value(7).toInt();
      return QColor(255-20*val,255,255-30*val);

  }
  return QVariant();
}

QVariant CompetitorsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if(orientation==Qt::Horizontal && role==Qt::DisplayRole)
    switch(section)
    {
    case 0: return QVariant(tr("Name"));
    case 1: return QVariant(tr("Birthdate"));
    case 2: return QVariant(tr("City"));
    case 3: return QVariant(tr("Region"));
    case 4: return QVariant(tr("Club"));
    case 5: return QVariant(tr("Phone"));
    }
  return QVariant();
}

Qt::ItemFlags CompetitorsModel::flags(const QModelIndex &index) const
{
  if(index.isValid())
    return Qt::ItemIsEnabled |Qt::ItemIsEditable | Qt::ItemIsSelectable;
  return Qt::NoItemFlags;
}

bool CompetitorsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
  bool res;
  QString field;
  query->seek(index.row());
  if(!index.isValid() || role!=Qt::EditRole) return false;
  switch (index.column())
  {
  case 0:
    field="name";break;
  case 1:
    field="birthdate";
    if (value.toInt()<1930 || value.toInt()>2100) return false;
    break;
  case 2:
    field="city";break;
  case 3:
    field="region";break;
  case 4:
    field="club";break;
  case 5:
    field="phone";break;
  default:
    return false;
  }
  last_edited_rowid=query->value(rowid_column).toInt();
  last_edited_column=index.column();
  if(value.toString().isEmpty()) return false;
  res=query->exec(QString("update competitor set %1='%2' where comp_id==%3 ")
         .arg(field)
         .arg(value.toString())
         .arg(query->value(rowid_column).toString()));
  selectSoft();
  if(res)
  dataChanged(index,index);

  return true;
}


bool CompetitorsModel::insertRow(const QString text)
{
  bool res;
  if(!text.contains(QRegExp("[0-9]{4}")))
  {
    res=query->exec(QString("insert into competitor(name) values('%1')")
                  .arg(text));
  }
  else
  {
    res=query->exec(QString("insert into competitor(name,birthdate) values('%1',%2)")
                  .arg(text.left(text.lastIndexOf(' ')))
                  .arg(text.mid(text.lastIndexOf(' ')+1))
                    );
  }
  notifyInsertedRow(res);
  return res;
}


//=========================== VIEW =====================================//



//=========================== TAB PAGE =====================================//


CompetitorsTab::CompetitorsTab(QWidget* parent) : QWidget(parent)
{
  QVector<int> w{170,80,100,75,90,50};
  comp = new OTableView;
  res = new QTableView;
  comp->setSelectionMode(QAbstractItemView::SingleSelection);
  model=new CompetitorsModel(this);
  comp->horizontalHeader()->setStretchLastSection(true);
  comp->horizontalHeader()->setSectionsClickable(true);
  comp->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  comp->setModel(model);
  comp->setValidator("[^0-9 ]+ [^0-9 ]+ [0-9]{4}$");
  comp->setTextHint(tr("Name Surname 4DigitBirthYear"));

  bAdd=new QPushButton(QIcon(":/icons/add-user.png"),tr("Add"));
  bRemove=new QPushButton(QIcon(":/icons/remove-user.png"),tr("Remove"));
  bAdd->setIconSize(QSize(32,16));
  bRemove->setIconSize(QSize(32,16));
  bAdd->setToolTip(tr("<font color='gray'>Add competitor</font><b> Ins</b>"));
  bRemove->setToolTip(tr("<font color='gray'>Remove competitor</font><b> Del</b>"));

  for(int i=0;i<6;i++) comp->setColumnWidth(i,w.at(i));

  spl=new QSplitter(Qt::Horizontal,this);
  layout= new QGridLayout;
  spl->addWidget(comp);
  spl->addWidget(res);
  spl->setStretchFactor(0,3);
  spl->setStretchFactor(1,1);

  layout->addWidget(spl,0,0,1,4);
  layout->addWidget(bAdd,1,0);
  layout->addWidget(bRemove,1,1);
  layout->setColumnStretch(2,2);
  layout->setColumnStretch(3,1);
  setLayout(layout);
  connect(comp->horizontalHeader(), SIGNAL(sectionClicked(int)), model, SLOT(setSortColumn(int)));
  connect(comp,SIGNAL(activated(QModelIndex)),comp,SLOT(edit(QModelIndex)));
  connect(bAdd,SIGNAL(clicked(bool)),SLOT(addCompetitor()));
  connect(bRemove,SIGNAL(clicked(bool)),SLOT(removeCompetitor()));
  connect(comp->selectionModel(),SIGNAL(currentChanged(QModelIndex,QModelIndex)),SLOT(selectionChanged(QModelIndex,QModelIndex)));
  connect(comp->selectionModel(),SIGNAL(currentChanged(QModelIndex,QModelIndex)),model,SLOT(currentChanged(QModelIndex,QModelIndex)));
  connect(&ui_settings,SIGNAL(race_changed()),model,SLOT(select()));
  connect(&ui_settings,SIGNAL(protocol_changed()),model,SLOT(select()));
}

void CompetitorsTab::loadCSV()
{
  QSqlQuery q;
  QSqlDatabase db=QSqlDatabase::database();
  QString filename;
  QStringList broken,valid;
  QStringList csvlist;


  if(ui_settings.current_race==0)
  {
    QMessageBox::warning(this,tr("Import competitors"),
                          tr("Race not chosen"),
                          QMessageBox::Ok);
    return;
  }

  filename = QFileDialog::getOpenFileName(this,
        tr("Chose competitors list"), ui_settings.current_dir, tr("CSV files (*.csv)"));
  if(filename.isEmpty()) return;
  ui_settings.update_current_dir(filename);
  QFile csvfile(filename);
  if (!csvfile.open(QIODevice::ReadOnly)) return;
  QTextStream ts(&csvfile);
  while (!ts.atEnd())
  {
      csvlist.append(ts.readLine().simplified());
  }
  for (QString &l : csvlist)
  {
    l.replace("; ",";");
    l.replace(" ;",";");
    if(!l.contains(QRegularExpression("^[^0-9 ]+ [^0-9 ]+( [^0-9 ]+)?;[0-9]{4};[^;]+;[^;]+;([^;]+)?$")))
      broken.push_back(l);
    else valid.push_back(l);
  }
  if(!broken.isEmpty())
  QMessageBox::warning(this,tr("Import competitors"),
                        tr("CSV contains ")+QString("%1/%2").arg(broken.size()).arg(csvlist.size()) + tr(" broken entries. Proper line template:\n\nName(2 words);Birtdate(4 digits);Class;Team\n\nJohn Smith;1968;M40;GoodYear"),
                        QMessageBox::Ok);
  qDebug() << sizeof(csvlist);
  if(!db.transaction())
  {
    QMessageBox::critical(this,tr("Import competitors"),
                          tr("Database error")+db.lastError().text(),
                          QMessageBox::Ok);
    return;
  }
  else
  {
    for(QString& s : valid)
      {
        csvlist=s.split(';');
        q.exec(QString("insert into competitor(name,birthdate,club) values ('%1','%2','%3')")
               .arg(csvlist.at(0))
               .arg(csvlist.at(1))
               .arg(csvlist.at(4)));
        q.exec(QString("insert into class(name,race_id) values ('%1',%2)")
               .arg(csvlist.at(2))
               .arg(ui_settings.current_race) );
        q.exec(QString("insert into team(name,class_id) select '%1',class_id from class where race_id==%2 and name=='%3'")
               .arg(csvlist.at(3))
               .arg(ui_settings.current_race)
               .arg(csvlist.at(2)) );
        if(q.exec(QString("insert into team_member(team_id,comp_id) select team.team_id,competitor.comp_id from competitor,class,team where team.class_id==class.class_id and class.name=='%1'  and class.race_id==%2 and team.name=='%3'  and competitor.name='%4' and competitor.birthdate=%5")
               .arg(csvlist.at(2))
               .arg(ui_settings.current_race)
               .arg(csvlist.at(3))
               .arg(csvlist.at(0))
               .arg(csvlist.at(1))) )

            q.exec(QString("insert into protocol (tm_id) values(%1)").arg(q.lastInsertId().toLongLong()));

      }

    db.commit();
  }
  model->select();
  ui_settings.classes_change_notify();
  ui_settings.teams_change_notify();
  ui_settings.protocol_change_notify();
}


void CompetitorsTab::keyPressEvent(QKeyEvent *event)
{

  if(comp->hasFocus())
  {
    switch(event->key())
    {
    case Qt::Key_Delete:
      removeCompetitor();
      break;
    case Qt::Key_Insert:
      addCompetitor();
      break;
    }

  }
  QWidget::keyPressEvent(event);

}


void CompetitorsTab::addCompetitor()
{
  comp->enterNewEntry();
}

void CompetitorsTab::removeCompetitor()
{
  if(!comp->currentIndex().isValid()) return;
  if(comp->currentIndex().column()==0)
    model->removeRow(comp->currentIndex().row());
}


void CompetitorsTab::selectionChanged(QModelIndex curr, QModelIndex prev)
{
  if(curr.column()==0) bRemove->setDisabled(false);
  else bRemove->setDisabled(true);
}

