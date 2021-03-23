#include "gpxreader.h"
#include <QFile>
#include <QFileDialog>
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include "uisettings.h"
#include <QMessageBox>



GPXReader::GPXReader(QObject* parent)
  :QObject(parent) {}

GPXReader gpxreader;

void GPXReader::readFile()
{
  QVector <cp_point> gpxdata;
  QSqlQuery q;
  QSqlDatabase db=QSqlDatabase::database();
  cp_point p;
  int level_inside_wpt=0;
  QString filename;

  if(ui_settings.current_race==0)
  {
    QMessageBox::warning(nullptr,tr("Adding control points from GPX"),
                          tr("Race not chosen"),
                          QMessageBox::Ok);
    return;
  }
  filename = QFileDialog::getOpenFileName(nullptr,
        tr("Open GPX"), ui_settings.current_dir, tr("GPX files (*.gpx)"));

  if(filename.isEmpty())return;
  ui_settings.update_current_dir(filename);
  QFile file(filename);
  file.open(QIODevice::ReadOnly);
  QXmlStreamReader xml(&file);
  xml.setNamespaceProcessing(false);
  while(!xml.atEnd())
  {
    xml.readNext();
    if(xml.error())
    {
      QMessageBox::warning(nullptr,tr("Adding control points"),
                                tr("Reading XML failed: ").arg(xml.errorString()),
                                 QMessageBox::Ok);
      return;
    }
    if(xml.isStartElement())
    {
      if(level_inside_wpt)
      {
        ++level_inside_wpt;
        if(xml.name()=="desc")
          p.desc=xml.readElementText();
        if(xml.name()=="name")
          p.name=xml.readElementText();
      }
      if(xml.name()=="wpt")
      {
        p.lat=p.lon=0;p.name=p.desc="";
        p.lat = xml.attributes().value("lat").toDouble();
        p.lon = xml.attributes().value("lon").toDouble();
        level_inside_wpt=1;
      }

    }

    if(xml.isEndElement())
    {
      if(level_inside_wpt)
        if(!--level_inside_wpt)
          gpxdata.push_back(p);
    }
  }
  qDebug() << gpxdata.size();
  bool res=true;
  if(ui_settings.current_race)
  {
    db.transaction();

    for(auto& x : gpxdata)
      res&=q.exec(QString("insert into cp(race_id,map_alias,lat,lon,desc)"
                   " values(%1,'%2',%3,%4,'%5')")
           .arg(ui_settings.current_race)
           .arg(x.name)
           .arg(x.lat,0,'f',7)
           .arg(x.lon,0,'f',7)
           .arg((x.desc).replace(QChar('\''),QChar('_'))) );

    if(q.exec(QString(tr("insert into course(name,race_id) values('Default',%1)"))
           .arg(ui_settings.current_race)))
      q.exec(QString("insert into course_data(course_id,cp_id)  select %1,cp_id from cp where race_id==%2")
             .arg(q.lastInsertId().toLongLong())
             .arg(ui_settings.current_race));

    db.commit();
  }
  ui_settings.cp_change_notify();
  ui_settings.courses_change_notify();
}
