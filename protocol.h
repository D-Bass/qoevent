#include <QLayout>
#include <QTextEdit>
#include <QPushButton>
#include <QLineEdit>
#include <QSpinBox>
#include <QLabel>
#include <QStyledItemDelegate>
#include <QMenu>
#include <QDialog>
#include <QFormLayout>
#include <QTimeEdit>
#include "osqltable.h"
#include <QShortcut>
#include "searchline.h"


extern QWidget* tabProtocol;

class StartTimesSetter : public QDialog
{
  Q_OBJECT
public:
  StartTimesSetter(OSqlTableModel* const prot_model, const QModelIndexList& idx_list,QWidget *parent=0);
private slots:
  void setStartTimes();
private:
  QDateTimeEdit* de;
  QTimeEdit* te;
  QFormLayout *lout;
  QPushButton* bOk;
  QModelIndexList idx;
  OSqlTableModel* model;
};

class ProtocolModel: public OSqlTableModel
{
  Q_OBJECT
public:
  ProtocolModel(QObject* parent=0);
  QVariant data(const QModelIndex &index, int role) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;
  Qt::ItemFlags flags(const QModelIndex &index) const;
  bool setData(const QModelIndex &index, const QVariant &value, int role);
  QModelIndex indexFromID(const qint64 id);
  bool chipAssigned(const QModelIndex &index) const;
  bool insertRow(const QString text);
  bool removeRow(int row);
  void selectSoft();
  qint64 getRowID(const QModelIndex &index) const;
  qint64 getClassID(const qint64 rowid) const;
  void unassign_tag(const QModelIndex idx);
  qint64 competitor_waiting_for_card=0;
  enum sql_column_name{class_id,team_id,comp_id,};
public slots:
  void show_unfinished(bool checked) {show_unfinished_only=checked;select();}
signals:
  void summaryUpdated(const QString& summary);
private slots:
  void clear_tag_arrived(QByteArray tag);

private:
  bool show_unfinished_only=false;
  const QStringList db_field{"class_id","team_id","comp_id","","lap",
                             "number","start_time_forced"};
  const QStringList header_name{
    tr("Class"),tr("Team"),tr("Name"),tr("Birthdate"),tr("Lap"),
    tr("Number"),tr("Start time")};

};




class ProtocolTab: public QWidget
{
  Q_OBJECT
public:
  ProtocolTab(QWidget* parent=0);
  ~ProtocolTab() {if (comp) delete comp;}
  qint64 getClassOfCompetitor(const qint64 rowid) const
    {return model->getClassID(rowid);}
private:
  ProtocolModel *model;
  OTableView *view;
  QHBoxLayout* lTabH;
  QVBoxLayout* lTabV;

  QLabel* info;
  QPushButton* bAdd;
  QPushButton* bRemove;
  QPushButton* bAssignLaps;
  QPushButton* bClearTag;
  QPushButton* bUnAssignTag;
  QPushButton* bShowUnfinished;
  QPushButton* bFindCompetitor;
  QShortcut*   scBindToCard,*scShowUnfinished,*scFindCompetitor;
  QCompleter* comp=nullptr;
  QMenu* menu;
  SearchLine* searchLine;
private slots:
  void add_team();
  void remove_competitor();
  void clear_tag();
  void check_protocol_closed();
  void unassign_tag();
  void toggle_unfinished() {bShowUnfinished->setChecked(bShowUnfinished->isChecked()^0x1);}
  void setStartTimes();
  void clearStartTimes();
  void columnResized(int idx,int old_size,int new_size)  {
    bFindCompetitor->move(
          view->columnViewportPosition(2)
          +view->pos().x()
          +view->columnWidth(2)/2
          -bFindCompetitor->width()/2,0);
    searchLine->move(QPoint(view->columnViewportPosition(2)+view->pos().x(),view->pos().y()));
  }
private:
  void contextMenuEvent(QContextMenuEvent *event);
  QVector<QPushButton**> buttons{&bAdd,&bRemove,&bClearTag,&bUnAssignTag,&bShowUnfinished};
};
