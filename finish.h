#ifndef FINISH_H
#define FINISH_H

#include "osqltable.h"
#include <QPushButton>
#include <QLayout>
#include <QShortcut>
#include <QMenu>
#include <QCheckBox>
#include <QGroupBox>
#include "searchline.h"

#pragma pack(1)
struct CardData {
  uint32_t id;
  char pad0[13];
  uint32_t nonce[3];
  uint32_t start_time;
  char pad1;
  char service_sector_pad[34];
  char page[56][17];
};
#pragma pack()

class FinishModel: public OSqlTableModel
{
  Q_OBJECT
public:
  FinishModel(QObject* parent=0);
  QVariant data(const QModelIndex &index, int role) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;
  Qt::ItemFlags flags(const QModelIndex &index) const;
  bool insertRow(const QString text) {return true;}
  bool removeRow(int row);
  bool setData(const QModelIndex &index, const QVariant &value, int role);
  void selectSoft();
  QString csvLine(const int row);
  QString csvSplitsLine(const int row, const bool add_printout_info);
  void recalc_score();
signals:
  void newSplitsReady(qint64);
public slots:
  void setFilterClause(QString clause) {filter_clause=clause;select();}
  void resetFilterClause() {filter_clause="";}
private slots:
  void splitsArrived(QByteArray spl);
private:
  void update_score(const qint64 prot_id);
  QHash <uint64_t,QString> group_place;
  QMap<uint32_t,uint8_t>splits;
  QString filter_clause;
};



class ClassSelector : public QGroupBox
{
  Q_OBJECT
public:
  ClassSelector(QWidget *parent=0);

  QPushButton *bOk;
signals:
  void filterClause(QString);
public slots:
  void popup(const QPoint pos);
private:
  QSqlQuery q;
  QCheckBox* toggleAll;
  QVector <std::tuple<QCheckBox*,qint64,QString>> choice;
  QVBoxLayout* lout;
  void focusOutEvent(QFocusEvent *event) {hide();}
private slots:
  void allStateChanged(int state);
  void apply();
};


class FinishTab: public QWidget
{
  Q_OBJECT
public:
  FinishTab(QWidget* parent=0);
private:
  FinishModel *model;
  OTableView *view;

  QHBoxLayout* lTabH;
  QVBoxLayout* lTabV;

  QPushButton* bReadTag,*bRemove, *bRecalc, *bClassSelector,*bFindCompetitor;
  QShortcut* scReadTag, *scRecalc, *scFindCompetitor;
  QMenu* menu;
  SearchLine* searchLine;
  ClassSelector* selector;
private slots:
  void read_tag();
  void remove_record();
  void recalc_score() {model->recalc_score();}
  /*
  void check_protocol_closed();
  */
  void export_csv();
  void edit_splits();
  void printSplits();
  void autoPrintSplits(qint64 prot_id);
  void setStartTimeForced();
  void setFinishTimeForced();
  void clearStartTimeForced();
  void clearFinishTimeForced();
  void removeControlPoint();
  void addControlPoint();
  void columnResized(int idx,int old_size,int new_size)  {
    bClassSelector->move(
          view->columnViewportPosition(4)
          +view->pos().x()
          +view->columnWidth(4)/2
          -bClassSelector->width()/2,0);
    bFindCompetitor->move(
          view->columnViewportPosition(2)
          +view->pos().x()
          +view->columnWidth(2)/2
          -bFindCompetitor->width()/2,0);
    searchLine->move(QPoint(view->columnViewportPosition(2)+view->pos().x(),view->pos().y()));
  }
  void selectorPopup() {selector->popup(QPoint(view->columnViewportPosition(4),view->pos().y()));}
  void setDQ();
  void clearDQ();

private:
  void contextMenuEvent(QContextMenuEvent *event);
  QString indexListRowID();
  qint64 arrived_protid=-1; //latest arrived splits's prot_id
  bool request_for_autoprint=false;

  QVector<QPushButton**> buttons{&bReadTag};

};




#endif // FINISH_H
