#include <QTableView>
#include <QLayout>
#include <QPushButton>
#include <QSqlQuery>
#include "osqltable.h"
#include <QDialog>


class EventListView : public QTableView
{
public:
  EventListView(QWidget *parent=0) : QTableView(parent) {};
  bool selection_present() {return !selectedIndexes().isEmpty();}
};

class EventListModel : public OSqlTableModel
{
  Q_OBJECT
public:
  EventListModel(QObject *parent =0);
  QVariant data(const QModelIndex &index, int role) const;
  bool setData(const QModelIndex &index, const QVariant &value, int role);
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;
  Qt::ItemFlags flags(const QModelIndex &index) const;
  void addRow(const QString name,const bool multirace);
  bool insertRow(const QString text) {return true;}
signals:
  void resizeRow(int row);
protected:
  void selectSoft();
};


class EventList : public QDialog
{
  Q_OBJECT
public:
  EventList(QWidget* parent=0);
  QSize sizeHint() const {return QSize(800,400);}
  EventListView* view;
  EventListModel* model;
private:
  QVBoxLayout *layoutV;
  QHBoxLayout *layoutH;
  QPushButton *bAdd,*bRemove,*bSwitch;
private slots:
  void switch_event();
  void remove_event();
  void create_event();
};



class Events
{
public:
  void edit();

};

extern Events events;
