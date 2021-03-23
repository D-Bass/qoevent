#include "iofreader.h"
#include "gpxreader.h"
#include "uisettings.h"
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QMessageBox>
#include <QInputDialog>



IOFReader::IOFReader(QFile* file) :
  xml(file)
{}


QHash<QString,int> xhash{
  {"Map",1},
  {"Control",2},
  {"StartPoint",2},
  {"FinishPoint",2},
  {"Course",3},
  {"ClassCourseAssignment",4},
  {"TeamCourseAssignment",5}
};


QXmlStreamAttributes IOFReader::last_attr{};


iof_ret IOFReader::readFile()
{
  QVector<cp_point> cp;
  QVector<course> crs;
  QVector<classcourse_t> clscrs;
  cp_point p;
  course c;
  classcourse_t cc;
  QString path;
  QString name;
  xtt type;

  int handler=0;
  while(!xml.atEnd())
  {
    xml.readNext();
    if(xml.error())
    {
      QMessageBox::warning(nullptr,QObject::tr("Import IOF XML"),
                                QObject::tr("Reading XML failed: ").arg(xml.errorString()),
                                 QMessageBox::Ok);
      return iof_ret{0,0};
    }

    if(!xml.isCharacters())
      name=xml.name().toString();

    if(!(xml.isCharacters() && xml.isWhitespace()))
    qDebug() << xml.name().toString() << xml.attributes().size() << xml.text();

    if(xml.isStartElement())
    {
      path.append("/" + name);
      if(!handler)
        if(xhash.contains(name))
          handler=xhash.value(name);
    }
    if(xml.isEndElement())
    {
      path.truncate(path.lastIndexOf('/',-1));
    }
    type=xml.tokenType();

    if(type==xtt::StartElement ||
       type==xtt::EndElement ||
       (type==xtt::Characters && !xml.isWhitespace()))
    {
      qDebug() << path << handler;
      switch(handler)
      {
      case 1:
        if(name=="Scale" && type==xtt::Characters)
            map_scale=xml.text().toFloat();
        if (type==xtt::EndElement)
          if(xhash.value(name)==handler) handler=0;
        break;
      case 2: //control
        if(type==xtt::Characters)
        {
          if(name=="Id" || name=="ControlCode" || name=="StartPointCode"||
             name=="FinishPointCode") p.name=xml.text().toString().simplified();
        }
        if(type==xtt::StartElement)
        {
          if(name=="Position")
          {
            p.lon=xml.attributes().value("lng").toDouble();
            p.lat=xml.attributes().value("lat").toDouble();
          }
          if(name=="MapPosition")
          {
            p.x=xml.attributes().value("x").toFloat();
            p.y=xml.attributes().value("y").toFloat();
          }
        }
        if(type==xtt::EndElement)
          if(xhash.value(name)==handler) {
            cp.push_back(p);handler=0;
            qDebug() << p.name << p.x << p.y << p.lat << p.lon << p.desc;
          }

        break;
      case 3: //course
        if(type==xtt::Characters)
        {
          if(name=="Name" || name=="CourseName") c.name=xml.text().toString();
          if(name=="CourseFamily") c.family=xml.text().toString();
          if(name=="Control" || name=="ControlCode" || name=="StartPointCode" ||
             name=="FinishPointCode") c.sequence.append(xml.text().toString().simplified()+",");
        }
        if(type==xtt::EndElement)
        {
          if(xhash.value(name)==handler)
            {
              handler=0;
              c.sequence.truncate(c.sequence.size()-1);
              crs.push_back(c);
              qDebug() << c.name << c.sequence;
              c.sequence.clear();
            }
        }

        break;
      case 4: //classcourseassignment
        if(type==xtt::Characters)
          {
            if(name=="ClassName") cc.cls=xml.text().toString();
            if(name=="CourseName") cc.crs=xml.text().toString();
          }
        if(type==xtt::EndElement)
          if(xhash.value(name)==handler)
            {
              handler=0;
              clscrs.push_back(cc);
            }
        break;
      case 5: //teamcourseassignment
        if(type==xtt::EndElement)
          if(xhash.value(name)==handler) handler=0;
        break;
      default:
        break;
      }
    }
  }

  QSqlQuery q;
  QSqlDatabase db=QSqlDatabase::database();

  while(map_scale==0)
  {
    map_scale=QInputDialog::getInt(0,
                         QString("Map scale unknown"),
                         QString("Map scale: "),
                         5000,2500,100000,500);
  }
  map_scale*=.001;
  for(cp_point& p : cp)
    {p.x*=map_scale;p.y*=map_scale;}
  if(ui_settings.current_race)
  {
    QStringList c_list;
    bool res=true;
    qint64 course_id;
    db.transaction();
    for(cp_point& p : cp)
      res&=q.exec(QString("insert into cp(race_id,map_alias,x,y,lat,lon)"
                   " values(%1,'%2',%3,%4,%5,%6)")
           .arg(ui_settings.current_race)
           .arg(p.name)
           .arg(p.x)
           .arg(p.y)
           .arg(p.lat,0,'f',7)
           .arg(p.lon,0,'f',7));
    for(course& c : crs)
    {
      res&=q.exec(QString("insert into course(race_id,name)"
                   " values(%1,'%2')")
           .arg(ui_settings.current_race)
           .arg(c.name));
      course_id=q.lastInsertId().toLongLong();
      if(course_id)
        {
          Q_ASSERT(course_id<((long long int)1)<<32);
          c_list=c.sequence.split(',');
          for(int i=0,ord=0;i<c_list.size();i++)
            {
              res&=q.exec(QString("insert into course_data (course_id,ord,cp_id) select %1,%2,cp.cp_id from cp where cp.map_alias=='%3' and cp.race_id==%4")
                          .arg(course_id)
                          .arg(++ord)
                          .arg(c_list.at(i))
                          .arg(ui_settings.current_race));
            }
        }
    }

    for(classcourse_t& cl : clscrs)
      {
        q.exec(QString("insert into class(race_id,name) values (%1,'%2')")
                    .arg(ui_settings.current_race)
                    .arg(cl.cls));
        q.exec(QString("insert into clscrs(disc_id,class_id,course_id) select 1,class_id,course_id from class,course where class.name=='%1' and course.name=='%2' and class.race_id==%3 and course.race_id==%3")
                    .arg(cl.cls)
                    .arg(cl.crs)
                    .arg(ui_settings.current_race));
      }

    if(!res)
      QMessageBox::critical(nullptr,QObject::tr("Adding control points"),
                              QObject::tr("Database error raised: %1").arg(q.lastError().text()),
                              QMessageBox::Ok);
    db.commit();
  }
  return iof_ret{cp.size(),crs.size()};
}
