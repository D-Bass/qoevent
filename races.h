#include <QTreeView>
#include <QSqlQueryModel>
#include <QSqlQuery>
#include <QPushButton>
#include <QFormLayout>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QComboBox>
#include <QDateEdit>
#include <QLayout>
#include <QCheckBox>
#include <QSpinBox>
#include <QDialog>
#include <config.h>


struct RaceIndex
{
  RaceIndex() {}
  RaceIndex(int32_t qline,int32_t rows,RaceIndex*parent,RaceIndex* child)
    : parent(parent),child(child),qline(qline),rows(rows) {}
  RaceIndex* parent;
  RaceIndex* child;
  int32_t qline;    ///query line
  int32_t rows;     ///
};

class RaceListModel : public QAbstractItemModel
{
  Q_OBJECT
public:
  RaceListModel(QObject* parent);
  ~RaceListModel() {delete query;}
  QModelIndex parent(const QModelIndex &child) const;
  QModelIndex index(int row, int column, const QModelIndex &parent) const;
  QModelIndex indexFromEvent(const qint64 event_id) const;
  QModelIndex indexFromRace(const qint64 race_id) const;
  int rowCount(const QModelIndex &parent) const;
  int columnCount(const QModelIndex &parent) const;
  QVariant data(const QModelIndex &index, int role) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;
  Qt::ItemFlags flags(const QModelIndex &index) const;
  qint64 EventFromRace(QModelIndex& index) const;
  void select();
  QVariantList dataToList(const QModelIndex & index) const;
  qint64 getRaceID(const QModelIndex& index) const;
  bool removeRow(const QModelIndex &row);
  void setEventFilter(qint64 event_id) {event_id_filter=event_id;}

  RaceIndex* active_race;

private:
  void check_active_race(RaceIndex* const candidate);
  qint64 event_id_filter;
  QSqlQuery* query;
  QVector<RaceIndex> idx_root;
  QVector<RaceIndex> idx;   //query seek argument for row# [0]->Event1-row0 [1]->Event2-row0;
};


class RaceDataForm : public QDialog
{
  Q_OBJECT
public:
  QSize sizeHint() const {return QSize(600,250);}
  RaceDataForm(bool createNew,QWidget *parent=0);
  ~RaceDataForm();
private:
  bool createNew;
  QFormLayout *fl;
  QLineEdit *leName,*leLocation;
  QComboBox *cbEvent;
  QPlainTextEdit *teOfficials;
  QDateEdit *deDate;
  QComboBox *cbFinishSortCriteria;
  QHBoxLayout *flRaceButtons;
  QPushButton *bOk,*bCancel;
  QSqlQuery* query;
private slots:
  void form_result();
  void clear_warning();
};

class RaceTab : public QWidget
{
  Q_OBJECT
public:
  RaceTab(QWidget* parent=0);
  enum Import {ImportCP,ImportClasses};
  void import_from_race(Import what);
private:
  QVBoxLayout *layoutV;
  QHBoxLayout *layoutH;
  RaceDataForm *fRace;
  QTreeView * view;
  RaceListModel* model;
  QPushButton *bAdd,*bEdit,*bActivate;
  QCheckBox *chFilter;
  void setup_columns();
  void edit_race(bool create_new);
private slots:
  void clear_race_protocol();
  void remove_race();
  void edit_race_dialog();
  void create_race_dialog();
  void update_page();
  void activate_race();  
  void close_protocol();

};
