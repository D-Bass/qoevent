#include "split_editor.h"
#include "uisettings.h"
#include "displaytime.h"
#include <QKeyEvent>
#include "displaytime.h"
#include <QFile>
#include <QScrollBar>
extern "C" {
#include "crc.h"
}

SplitsModel::SplitsModel(const qint64 prot_rowid, QObject *parent) :
  OSqlTableModel(2,2,"delete from split where rowid==%1",parent),
  prot_id(prot_rowid)
{
  select();
}

void SplitsModel::selectSoft()
{
  query->exec(QString("select map_alias,time,split.rowid,chip from split left join cp using(cp_id),protocol where protocol.prot_id==%1 and split.prot_id==%1 order by time")
              .arg(prot_id));
}


QVariant SplitsModel::data(const QModelIndex &index, int role) const
{
  if(role!=Qt::DisplayRole && role!=Qt::EditRole) return QVariant();
  query->seek(index.row());
  switch(index.column())
    {
    case 0:
      return query->value(0).toString();
    case 1:
      return DisplayTime::min_sec(query->value(1).toInt());
    }
  return QVariant();
}



QVariant SplitsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if(orientation==Qt::Horizontal && role==Qt::DisplayRole)
    switch (section)
      {
      case 0: return (tr("Map Name"));
      case 1: return (tr("Time"));
      }
  return QVariant();
}

Qt::ItemFlags SplitsModel::flags(const QModelIndex &index) const
{
  if(index.parent().isValid()) return 0;
  return Qt::ItemIsEnabled |Qt::ItemIsEditable|Qt::ItemIsSelectable;
}


bool SplitsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
  if(role!=Qt::EditRole) return false;
  query->seek(index.row());
  last_edited_rowid=query->value(rowid_column).toInt();
  last_edited_column=index.column();
  QString db_cmd,val=value.toString();
  if(index.column()==0) db_cmd=QString("update split set cp_id=coalesce((select cp_id from cp where race_id==%1 and map_alias=='%2'),(select cp_id from split where rowid==%3)) where rowid==%3")
      .arg(ui_settings.current_race)
      .arg(val)
      .arg(last_edited_rowid);
  if(index.column()==1)
    {
      int min=val.left(val.indexOf(':')).toInt();
      int sec=val.mid(val.indexOf(':')+1).toInt();
      db_cmd=QString("update split set time=%1 where rowid==%2")
          .arg(min*60+sec)
          .arg(last_edited_rowid);
    }
  bool res=query->exec(db_cmd);
  selectSoft();
  if(res) dataChanged(index,index);
  return true;
}


bool SplitsModel::insertRow(const QString text)
{
  QStringList val=text.split(' ');
  if(val.size()!=2) return false;
  QSqlQuery q;
  q.exec(QString("select cp_id from cp where race_id==%1 and map_alias=='%2'")
         .arg(ui_settings.current_race)
         .arg(val.at(0)));
  if(!q.next()) return false;
  if(q.value(0).toString()=="") return false;

  bool res=query->exec(QString("insert into split(prot_id) values(%1)")
                       .arg(prot_id));
  notifyInsertedRow(res);
  QModelIndex idx=lastInserted();
  if(res)
    {
      setData(createIndex(idx.row(),0,nullptr),val.at(0),Qt::EditRole);
      setData(createIndex(idx.row(),1,nullptr),val.at(1),Qt::EditRole);
    }
  return true;
}


extern QHash<int,QString> map_alias_of_int_num;

void SplitsModel::decodeCardData(QTextEdit* text)
{
  query->seek(0);
  QFile file("cards/"+query->value(3).toString()+".bin");
  if(!file.open(QIODevice::ReadOnly)) return;
  file.read(page[0],sizeof(page));
  file.close();

  uint32_t s,s_time;
  uint8_t s_cp,adr;
  splits[0].clear();splits[1].clear();
  for(int j=0;j<2;j++)
    for(int i=0;i<28;i++)
      {

        //don't read sector trailer
        if((i&0x3)==3) continue;
        adr=4+28*j+i;
        //if crc for string ok,get 4 28-bit adjacent split values
        if(crc16(page[adr],16)) continue;
        for(int k=0;k<4;k++)
          {
            s=*(uint32_t*)(page[adr]+(k*7>>1));
            if(!s) continue;
            if(k&0x1) s>>=4;
            s&=0xfffffff;
            s_cp=s&0xff;
            s_time=s>>8;
            splits[j].push_back(qMakePair(s_time,s_cp));
          }
      }
  for(int i=0;i<2;i++)
    {
      text->append(tr("Data from area %1: ").arg(i));
      for(auto& it : splits[i])
        {
          text->append(QString("%1(%2); ")
                       .arg(DisplayTime::min_sec(it.first))
                       .arg(map_alias_of_int_num.value(it.second)));
        }
      text->append((""));
    }
}
















SplitsDialog::SplitsDialog(const qint64 splits_prot_id, QWidget *parent) :
  QDialog(parent)
{
  QVector<int> w{80,80};
  model=new SplitsModel(splits_prot_id,this);
  view=new OTableView;
  tCard=new QTextEdit;
  tCard->setLineWrapMode(QTextEdit::WidgetWidth);
  tCard->setFontPointSize(10);
  tCard->setReadOnly(true);
  model->decodeCardData(tCard);
  view->setSelectionMode(QAbstractItemView::SingleSelection);
  view->setTextHint("MapAlias min:sec");
  view->setModel(model);
  for(int i=0;i<w.size();i++) view->setColumnWidth(i,w.at(i));
  lout=new QGridLayout(this);
  lout->addWidget(view, 0,0);
  lout->addWidget(tCard,0,1);
  lout->setColumnStretch(0,1);
  lout->setColumnStretch(1,1);
  view->show();
  tCard->verticalScrollBar()->setValue(0);
  resize(400,400);
  setFixedWidth(400);
  setWindowTitle(tr("Split Editor"));
  connect(view,SIGNAL(activated(QModelIndex)),view,SLOT(edit(QModelIndex)));
  connect(view->selectionModel(),SIGNAL(currentChanged(QModelIndex,QModelIndex)),model,SLOT(currentChanged(QModelIndex,QModelIndex)));
}



void SplitsDialog::keyPressEvent(QKeyEvent *event)
{

  if(view->hasFocus())
  {
    switch(event->key())
    {
    case Qt::Key_Delete:
      removeEntry();
      break;
    case Qt::Key_Insert:
      addEntry();
      break;
    }

  }

}


void SplitsDialog::addEntry()
{
  if(ui_settings.current_race)
  view->enterNewEntry();
}

void SplitsDialog::removeEntry()
{
  if(!ui_settings.current_race) return;
  if(!view->currentIndex().isValid()) return;
  model->removeRow(view->currentIndex().row());
}
