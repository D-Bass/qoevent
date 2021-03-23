#ifndef COURSES_H
#define COURSES_H
#include "osqltable.h"
#include "controlpoints.h"
#include <QStyledItemDelegate>

class CoursesModel: public OSqlTableModel
{
  Q_OBJECT
public:
  CoursesModel(QObject* parent=0);
  ~CoursesModel() {delete seq_txt;}
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;
  QVariant data(const QModelIndex &index, int role) const;
  bool setData(const QModelIndex &index, const QVariant &value, int role);
  bool insertRow(const QString text);
  Qt::ItemFlags flags(const QModelIndex &index) const;
  void selectSoft();
private:
  QVariant distance(const qint64 rowid) const;
  QString* seq_txt;

  const QStringList header_name{
    tr("Name"),tr("Controls order"),tr("Dist"),tr("Soft\norder"),tr("OvTime\nlimit"),
    tr("OvTime\npenalty"),tr("Min\nPen"),tr("Max\nPen"),tr("Min\nScr"),tr("Val\nTU"),
    tr("Pnt\nTU"),tr("Max\nPunch\nInterv")
  };
  const QStringList header_tootip{
    tr("Name of course"),tr("Set of controls (for unordered) or mandatory punch sequence"),
    tr("Total distance(m) for sequence"),tr("Sequence can start with any control"),
    tr("Overtime limit:\nHH:MM for personal\nDD.MM.YYYY HH:MM for everyone"),
    tr("Overtime penalty for every minute"),tr("Allowed unaccoutable penalty"),
    tr("Maximun penalty for qualification"),tr("Minimum score for qualification"),
    tr("Control value is in time units"),tr("Control penalty is in time units"),
    tr("Maximum punch interval(sec) for group punches")
  };
  const QStringList db_field{
    "name","","","soft_order","overtime","overtime_penalty",
    "min_penalty","max_penalty","min_score","value_is_time",
    "penalty_is_time","max_punch_interval"};
};


class CoursesView : public OTableView
{
public:
  CoursesView(QWidget* parent=0) :  OTableView(parent) {}
};




class CourseEditor : public QLineEdit
{
  Q_OBJECT
public:
  CourseEditor(qint64 course=0,QWidget* parent=0)
    : QLineEdit(parent),pal(palette()),course_id(course) {}
  bool validate();
private:
  QPalette pal;
  void keyPressEvent(QKeyEvent *event);
  qint64 course_id;
  QStringList vlist;
};


class CourseDelegate : public QStyledItemDelegate
{
public:
  CourseDelegate(QObject* parent=0);
  QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
  void setEditorData(QWidget *editor, const QModelIndex &index) const;
  void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;
  bool eventFilter(QObject *editor, QEvent *event);
};






class PricesModel: public QAbstractTableModel
{
  Q_OBJECT
public:
  PricesModel(QObject* parent=0);
  ~PricesModel() {delete query;}
  QVariant data(const QModelIndex &index, int role) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;
  int columnCount(const QModelIndex &parent) const {return 4;}
  int rowCount(const QModelIndex &parent) const;
  void selectSoft(const qint64 course);
  Qt::ItemFlags flags(const QModelIndex &index) const;
  bool setData(const QModelIndex &index, const QVariant &value, int role);
public slots:
  void select(const qint64 course)
  { beginResetModel();selectSoft(course);endResetModel(); }
private:
  QSqlQuery* query;
  qint64 current_course=0;
  const QStringList header_name { tr("Ctrl"),tr("Scr"),tr("Pnt"),tr("Cutoff") };
  const QStringList header_tootip{
    tr("Control name on map"),tr("Score to value (may be negative)"),
    tr("Penalty of missing control (may be negative)"),tr("Time cutoff checkpoint")
  };
  const QStringList db_field{ "","value","penalty","cutoff" };
};


class PricesView : public QTableView
{
  Q_OBJECT
public:
  PricesView(QWidget* parent=0) :  QTableView(parent) {}
};



#endif // COURSES_H
