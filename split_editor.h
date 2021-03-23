#ifndef SPLIT_EDITOR_H
#define SPLIT_EDITOR_H
#include <osqltable.h>
#include <QDialog>
#include <QLayout>
#include <QTextEdit>

class SplitsModel: public OSqlTableModel
{
  Q_OBJECT
public:
  SplitsModel(const qint64 prot_rowid,QObject* parent=0);
  QVariant data(const QModelIndex &index, int role) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;
  bool setData(const QModelIndex &index, const QVariant &value, int role);
  bool insertRow(const QString text);
  Qt::ItemFlags flags(const QModelIndex &index) const;
  void selectSoft();
  void decodeCardData(QTextEdit *text);
private:
  qint64 prot_id;
  char page[64][16];
  QVector<QPair<uint32_t,uint8_t>> splits[2]{QVector<QPair<uint32_t,uint8_t>>(60),
                                            QVector<QPair<uint32_t,uint8_t>>(60)};
};

class SplitsDialog : public QDialog
{
  Q_OBJECT
public:
  SplitsDialog(const qint64 splits_prot_id,QWidget *parent=0);
  SplitsModel* model;
  OTableView* view;
  QTextEdit* tCard;
  QGridLayout *lout;
private:
  void addEntry();
  void removeEntry();
protected:
  void keyPressEvent(QKeyEvent *event);
};


#endif // SPLIT_EDITOR_H
