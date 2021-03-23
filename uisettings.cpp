#include "uisettings.h"
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QFile>
#include <QDebug>

UISettings::UISettings(QObject* parent)
  : QObject(parent), current_event(0),current_race(0)
{
  QFile settings("settings.ini");
  if(!settings.open(QIODevice::ReadOnly)) return;
  QXmlStreamReader xml(&settings);

  while(!xml.atEnd())
  {
    xml.readNext();
    if(xml.error())
    {
      qDebug() << xml.errorString();
    }
    if(xml.tokenType()==QXmlStreamReader::StartElement)
      {
        if(xml.name()=="Printer")
          {
            printer_name=xml.attributes().value("name").toString();
            printer_port_name=xml.attributes().value("port").toString();
          }
        if(xml.name()=="AutoPrintout")
          auto_printout=xml.attributes().value("val").toInt();
      }
  }
}


UISettings::~UISettings()
{
  QFile settings("settings.ini");
  if(!settings.open(QIODevice::WriteOnly)) return;
  QXmlStreamWriter xml(&settings);
  xml.writeStartDocument();
  xml.writeStartElement("Settings");
  xml.writeStartElement("Printer");
  xml.writeAttribute("name",printer_name);
  xml.writeAttribute("port",printer_port_name);
  xml.writeEndElement();
  xml.writeStartElement("AutoPrintout");
  xml.writeAttribute("val",QString("%1").arg(auto_printout));
  xml.writeEndElement();
  xml.writeEndElement();
  xml.writeEndDocument();
}

UISettings ui_settings;
