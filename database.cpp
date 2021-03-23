#include <QSqlDatabase>
#include <QSqlQuery>
#include <QDebug>
#include <QSqlError>
#include <QFile>
#include <QProcess>
#include <QDateTime>
#include <QMessageBox>


QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");

bool init_database()
{
  bool res=true;
  QString bkfile(QString("ob%1.sqlite").arg(QDateTime::currentDateTime().toString ("dd-MM-yy_HH-mm")));
  QFile bk(bkfile);
  QFile dbase("o-base.sqlite");
  dbase.copy(bkfile);
#ifdef Q_OS_LINUX
  QProcess gzip;
  gzip.execute("zip",{"-u9","obackup.zip",bkfile});
#elif defined Q_OS_WIN
  QProcess gzip;
  gzip.execute("7za.exe",{"a","obackup.zip",bkfile});
#endif

  gzip.waitForFinished();
  if(gzip.exitStatus()==QProcess::NormalExit)
    if(gzip.exitCode()==0)
      bk.remove();


  QString db_cmd;
  QFile creation_list(":/base_creat.txt");
  QTextStream ts(&creation_list);
  creation_list.open(QIODevice::ReadOnly);
  QSqlQuery q(db);
  db.setDatabaseName("o-base.sqlite");
  if (!db.open()) return false;

   q.exec("pragma foreign_keys=on");
   if (db.tables(QSql::Tables).isEmpty())
   {
      while (!ts.atEnd())
      {
        if( !(db_cmd = ts.readLine() ).isEmpty() )
          if(!q.exec(db_cmd))
            res=false;
      }
   }
   if(!res) QMessageBox::warning(0,QObject::tr("Database error"),
                                 QObject::tr("Database not created properly"),
                                 QMessageBox::Ok);
   return res;
}

