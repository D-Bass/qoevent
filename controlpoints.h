#ifndef CONTROLPOINTS_H
#define CONTROLPOINTS_H

#include <QTableView>
#include <QLayout>
#include <QTextEdit>
#include <QGroupBox>
#include <QPushButton>
#include <QLineEdit>
#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>
#include <QIcon>
#include "osqltable.h"

extern QWidget* tabCP;

class ControlPoints
{
public:
  typedef struct
  {
    bool operator !() const {if((!x && !y)  && (!lat || !lon) ) return true;else return false;}
    double lat=0,lon=0;
    int x=0,y=0;
  } cp_point;
  ControlPoints();
  static int distance(cp_point p1,cp_point p2);
};


class CPModel: public OSqlTableModel
{
  Q_OBJECT
public:
  CPModel(QObject* parent=0);
  QVariant data(const QModelIndex &index, int role) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;
  Qt::ItemFlags flags(const QModelIndex &index) const;
  bool setData(const QModelIndex &index, const QVariant &value, int role);
  QString int_num(const QModelIndex &idx) const;
  bool insertRow(const QString text);
  int distance(const qint64 a,const qint64 b) const
  {return ControlPoints::distance(coordinates.value(a),coordinates.value(b));}
private:
  QHash<qint64,ControlPoints::cp_point>coordinates;
  QIcon geo_icon;
  const QStringList db_field{"map_alias","int_number","","value","penalty","desc"};
protected:
  void selectSoft();
};




class CPTab: public QWidget
{
  Q_OBJECT
public:
  CPTab(QWidget* parent=0);
  int distance(const qint64 a,const qint64 b) const
  {return model->distance(a,b);}
public slots:
  void setCtrlEnabled(bool val) {
    for(QPushButton** b : buttons)
      (*b)->setDisabled(!val);
  }
private:
  CPModel *model;
  OTableView *view;

  QHBoxLayout* lTabH;
  QVBoxLayout* lTabV;

  QGridLayout* lCP;
  QPushButton* bCPAdd;
  QPushButton* bCPRemove;
  QPushButton* bCPAssignValue;

  QGroupBox* grpState;
  QVBoxLayout* lState;
  QTextEdit* viewstate;
  QPushButton* bState;

  QLabel* lbBatch;
  QLabel* lbNow;
  QGroupBox* grpConfig;
  QGridLayout* lConfig;
  QCheckBox* chShutdown;
  QPushButton* bShutdown;
  QCheckBox* chSynch;
  QPushButton* bSynch;
  QCheckBox* chActiveTime;
  QSpinBox* sbActiveTime;
  QPushButton* bActiveTime;
  QLineEdit* leCPNum;
  QPushButton* bCPNum;
  QVector<QPushButton**> buttons{&bState,&bSynch,&bActiveTime,&bCPNum,&bShutdown};

  void keyPressEvent(QKeyEvent *event);
private slots:
  void addCP();
  void removeCP();
  void assignValuesCP();
  void serial_changed();
  void cp_int_num_update();
  void request_cp_state();
  void set_cp_time();
  void set_cp_mode();
  void set_cp_active_timeout();
  void shutdown_cp();
  void race_changed();
  void cp_state_arrived(QByteArray val);
};


#endif// CONTROLPOINTS
