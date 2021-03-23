#include <QLayout>
#include <QPushButton>
#include <QSplitter>
#include "controlpoints.h"
#include "disciplines.h"
#include "osqltable.h"
#include "courses.h"


class ClassesModel: public OSqlTableModel
{
  Q_OBJECT
public:
  ClassesModel(QObject* parent=0);
  QVariant data(const QModelIndex &index, int role) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;
  bool setData(const QModelIndex &index, const QVariant &value, int role);
  bool insertRow(const QString text);
  Qt::ItemFlags flags(const QModelIndex &index) const;
  void selectSoft();
};





class CCModel: public OSqlTableModel
{
  Q_OBJECT
public:
  CCModel(QObject* parent=0);
  QVariant data(const QModelIndex &index, int role) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;
  Qt::ItemFlags flags(const QModelIndex &index) const;
  bool setData(const QModelIndex &index, const QVariant &value, int role);
  bool insertRow(const QString text) {return true;}
  bool insertRow(qint64 cls, qint64 crs);
  void selectSoft();
private:
  QStringList db_field{"class_id","disc_id","lap","course_id"};
};





class ClassesTab : public QWidget
{
  Q_OBJECT
public:
  ClassesTab(QWidget *parent = 0);

private:
  OTableView* clsview;
  CoursesView* crsview;
  OTableView* ccview;
  QTableView* prview;
  ClassesModel* clsmodel;
  CoursesModel* crsmodel;
  CCModel* ccmodel;
  PricesModel* prmodel;

  QGridLayout* lClass;
  QPushButton* bClassAdd;
  QPushButton* bClassRemove;

  QGridLayout *lCourse;
  QPushButton* bCourseAdd;
  QPushButton* bCourseRemove;

  QGridLayout *lCC;
  QPushButton* bCCAdd;
  QPushButton* bCCRemove;

  QGridLayout* lTab;
  QHBoxLayout* lGrp;

  void disable_buttons(bool d);
public slots:
  void classes_update() {clsmodel->select();}
  void courses_update() {crsmodel->select();}
  void clscrs_update() {ccmodel->select();}
  void race_changed();

protected slots:
  void add_class();
  void remove_class();
  void add_course();
  void remove_course();
  void add_cc();
  void remove_cc();
  void course_selection_changed();
  void course_data_changed(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles);
protected:
  void keyPressEvent(QKeyEvent *event);
};

