#include <QTableView>
#include <QLayout>
#include <QSqlQuery>
#include <QEvent>
#include <QPushButton>
#include <QSplitter>
#include "osqltable.h"


class CompetitorsModel: public OSqlTableModel
{
  Q_OBJECT
public:
  CompetitorsModel(QObject* parent=0);
  QVariant data(const QModelIndex &index, int role) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;
  bool setData(const QModelIndex &index, const QVariant &value, int role);
  Qt::ItemFlags flags(const QModelIndex &index) const;
  bool insertRow(const QString text);
private:
  void selectSoft();
};



class CompetitorsTab: public QWidget
{
  Q_OBJECT
public:
  CompetitorsTab(QWidget* parent=0);
public slots:
  void loadCSV();
private:
  void keyPressEvent(QKeyEvent *event);
  CompetitorsModel *model;
  QTableView *res;
  OTableView* comp;
  QPushButton* bAdd;
  QPushButton* bRemove;
  QGridLayout* layout;
  QSplitter* spl;
private slots:
  void addCompetitor();
  void removeCompetitor();
  void selectionChanged(QModelIndex curr,QModelIndex prev);
};
