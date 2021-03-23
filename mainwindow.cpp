#include <QMainWindow>
#include <QDir>
#include <QMenuBar>
#include <memory>
#include "database.h"
#include <QStatusBar>
#include <QMessageBox>
#include <QFileDialog>
#include "defaultclasses.h"
#include "events.h"
#include "races.h"
#include "gpxreader.h"
#include "iofreader.h"
#include "competitors.h"
#include "classes.h"
#include "uisettings.h"
#include "mainwindow.h"
#include "protocol.h"
#include "finish.h"
#include "service.h"
#include "spool.h"
#include <QTimer>

MainWindow::MainWindow()
{
  init_database();
  setWindowIcon(QIcon(":/icons/micon.gif"));
  setWindowTitle("QoManager");
  mMain=menuBar();
  mFile=mMain->addMenu(tr("File"));
  mEvents=mMain->addMenu(tr("Events"));
  mRaces=mMain->addMenu(tr("Races"));
  mClasses=mMain->addMenu(tr("Classes"));
  mProtocol=mMain->addMenu(tr("Protocol"));
  mCP=mMain->addMenu(tr("Control Points"));
  mService=mMain->addMenu(tr("Service"));

  aLoadIOF=mFile->addAction(tr("Import IOF XML"));
  aSelectPrinter=mFile->addAction(tr("Select printer fo splits printout"));
  aExit=mFile->addAction(tr("Exit"));

  aListEvents=mEvents->addAction(tr("List of Events"));

  aRaceClearProtocol=mRaces->addAction(tr("Clear the race protocol"));
  aRaceRemove=mRaces->addAction(tr("Remove the whole race"));

  aClassesLoadRace=mClasses->addAction(tr("Import classes from race"));
  aClassLoadDefault=mClasses->addAction(tr("Load default classes"));
  aClassSetDefault=mClasses->addAction(tr("Setup default classes"));

  aCompetitorsFromCSV=mProtocol->addAction(tr("Import competitors from CSV"));
  aProtocolSaveCSV=mProtocol->addAction(tr("Export protocol to CSV"));
  aProtocolRecalc=mProtocol->addAction(tr("Recalculate protocol"));
  aSplitsPrint=mProtocol->addAction(tr("Print splits after readout"));
  aSplitsPrint->setCheckable(true);aSplitsPrint->setChecked(ui_settings.auto_printout);
  aAccountTeams=mProtocol->addAction(tr("Group result by teams"));
  aAccountTeams->setCheckable(true);
  aProtocolClose=mProtocol->addAction(tr("Close Protocol"));

  aCPFromRace=mCP->addAction(tr("Import control points from race"));
  aCPFromGPX=mCP->addAction(tr("Import control points from GPX"));

  aService=mService->addAction(QIcon(":/icons/settings.png"),tr("Control Maintance"));

  stBar=statusBar();

  cWidget=new QWidget;
  setCentralWidget(cWidget);

  mainLayout=new QVBoxLayout;

  cWidget->setLayout(mainLayout);
  tab=new QTabWidget(cWidget);
  tabCP=new CPTab;
  tabCompetitors=new CompetitorsTab;
  tabClasses=new ClassesTab;
  tabProtocol=new ProtocolTab;
  tabRaces=new RaceTab;
  tabFinish=new FinishTab;
  tab->addTab(tabCompetitors,tr("Competitors"));
  tab->addTab(tabCP,tr("Control points"));
  tab->addTab(tabClasses,tr("Classes/Courses"));
  tab->addTab(tabRaces,tr("Races"));
  tab->addTab(tabProtocol,tr("Protocol"));
  tab->addTab(tabFinish,tr("Finish"));
  mainLayout->addWidget(tab);
  connect(aSelectPrinter,SIGNAL(triggered()),SLOT(spool_dialog()));
  connect(aExit,SIGNAL(triggered()),SLOT(close()));
  connect(aClassSetDefault,SIGNAL(triggered()),SLOT(edit_def_class()));
  connect(aClassLoadDefault,SIGNAL(triggered(bool)),SLOT(load_def_class()));
  connect(aListEvents,SIGNAL(triggered()),SLOT(edit_events()));
  connect(aRaceRemove,SIGNAL(triggered()),tabRaces,SLOT(remove_race()));
  connect(aRaceClearProtocol,SIGNAL(triggered()),tabRaces,SLOT(clear_race_protocol()));
  connect(aCPFromGPX,SIGNAL(triggered()),&gpxreader,SLOT(readFile()));
  connect(aLoadIOF,SIGNAL(triggered()),SLOT(load_iof()));
  connect(aCompetitorsFromCSV,SIGNAL(triggered()),tabCompetitors,SLOT(loadCSV()));
  connect(aCPFromRace,SIGNAL(triggered()),SLOT(cp_from_race()));
  connect(aClassesLoadRace,SIGNAL(triggered()),SLOT(classes_from_race()));
  connect(aProtocolClose,SIGNAL(triggered()),tabRaces,SLOT(close_protocol()));
  connect(aProtocolRecalc,SIGNAL(triggered()),tabFinish,SLOT(recalc_score()));
  connect(aProtocolSaveCSV,SIGNAL(triggered()),tabFinish,SLOT(export_csv()));
  connect(aAccountTeams,SIGNAL(triggered(bool)),&ui_settings,SLOT(account_teams_changed(bool)));
  connect(aSplitsPrint,SIGNAL(toggled(bool)),SLOT(set_printout(bool)));
  connect(aService,SIGNAL(triggered()),SLOT(service_dialog()));

  connect(&ui_settings,SIGNAL(statusbar_message(QString,int)),stBar,SLOT(showMessage(QString,int)));
  this->show();
  bs=new BaseStation(this);
  splits_printer=new SplitsPrinter(this);
  bs->init();
}

void MainWindow::edit_def_class()
{
  edit_default_classes();
}

void MainWindow::load_def_class()
{
  if(!ui_settings.current_race) {QMessageBox::warning(this,tr("Load default classes"),
                            tr("Race not chosen"),
                             QMessageBox::Ok);return;}
  if(!load_default_classes()) QMessageBox::warning(this,tr("Load default classes"),
                              tr("Failed"),
                             QMessageBox::Ok);
}

void MainWindow::edit_events()
{
  events.edit();
}

void MainWindow::set_printout(bool v) {ui_settings.auto_printout=v;}

void MainWindow::load_iof()
{
  iof_ret ret;
  if(!ui_settings.current_race) {QMessageBox::critical(this,tr("Import IOF XML"),
                            tr("Race not chosen"),
                             QMessageBox::Ok);return;}
  QString filename;
  filename = QFileDialog::getOpenFileName(this,
        tr("Open XML"), ui_settings.current_dir, tr("IOF XML files (*.xml)"));
  ui_settings.update_current_dir(filename);

  if(filename.isEmpty()) return;
  QFile file(filename);
  if(!file.open(QIODevice::ReadOnly)) return;
  IOFReader parser(&file);
  ret=parser.readFile();
  statusBar()->showMessage(QString(tr("Imported control points/courses: ")+QString("%1/%2").arg(std::get<0>(ret)).arg(std::get<1>(ret))));
  ui_settings.courses_change_notify();
  ui_settings.classes_change_notify();
  ui_settings.clscrs_change_notify();
  ui_settings.cp_change_notify();
}


void MainWindow::cp_from_race()
{
  static_cast<RaceTab*>(tabRaces)->import_from_race(RaceTab::ImportCP);
}

void MainWindow::classes_from_race()
{
  static_cast<RaceTab*>(tabRaces)->import_from_race(RaceTab::ImportClasses);
}


void MainWindow::service_dialog()
{
  ServiceDialog dlg;
  dlg.exec();
}

void MainWindow::spool_dialog()
{
  PrinterSelector selector;
  selector.exec();
}
