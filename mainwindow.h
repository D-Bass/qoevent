#include <QMainWindow>
#include <QMenuBar>
#include <memory>
#include <QDebug>
#include <QLayout>
#include "comm.h"


class MainWindow : public QMainWindow {
  Q_OBJECT
public:
  MainWindow();
  QSize sizeHint() const {return QSize(1200,500);}
  void show_status(QString& text);
private:
  QAction *aExit,*aSelectPrinter,*aLoadIOF;
  QAction *aListEvents;
  QAction *aRaceRemove,*aRaceClearProtocol;
  QAction *aClassesLoadRace,*aClassLoadDefault,*aClassSetDefault;
  QAction *aCompetitorsFromCSV,*aProtocolClose,*aProtocolSaveCSV,*aProtocolRecalc,
          *aSplitsPrint,*aAccountTeams;
  QAction *aCPFromRace,*aCPFromGPX,*aCPSave,*aService;
  QMenuBar* mMain;
  QMenu *mFile,*mEvents,*mRaces,*mClasses,*mProtocol,*mCP,*mService;
  QTabWidget* tab;
  QWidget* tabCompetitors;
  QWidget* tabClasses;
  QWidget* tabRaces;
  QWidget* tabFinish;

  QVBoxLayout* mainLayout;
  QStatusBar* stBar;
  QWidget* cWidget;


private slots:
  void load_iof();
  void edit_def_class();
  void load_def_class();
  void edit_events();
  void cp_from_race();
  void classes_from_race();
  void service_dialog();
  void set_printout(bool v);
  void spool_dialog();
};
