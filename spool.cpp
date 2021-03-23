#include "spool.h"
#include <QDebug>
#include <QPrinter>
#include "uisettings.h"
#include <QMessageBox>

PrinterSelector::PrinterSelector() :
  pinfo(reinterpret_cast<PPRINTER_INFO_5>(printer_info))
{
  layout=new QVBoxLayout(this);

  list=new QListWidget;
  bOk=new QPushButton(tr("Select printer"));
  bCancel=new QPushButton(tr("Cancel"));

    if(EnumPrinters(PRINTER_ENUM_LOCAL,
                  NULL,
                  5,
                  printer_info,
                  sizeof(printer_info),
                  &buff_size,
                  &num_printers))
  for(unsigned int i=0;i<num_printers;i++)
    {
      qInfo() << QString::fromWCharArray((pinfo+i)->pPortName);
      list->addItem(QString::fromWCharArray((pinfo+i)->pPrinterName));
    }

  layout->setContentsMargins(20,20,20,20);
  layout->addWidget(list,10);
  layout->addSpacing(30);
  layout->addWidget(bOk,1);
  layout->addWidget(bCancel,1);

  setWindowTitle(tr("Splits printout"));
  setWindowModality(Qt::WindowModal);
  connect(bCancel,SIGNAL(clicked(bool)),SLOT(close()));
  connect(bOk,SIGNAL(clicked()),SLOT(selectPrinter()));
  show();
}

void PrinterSelector::selectPrinter()
{
  if(list->currentIndex()==QModelIndex()) return;
  ui_settings.printer_name=QString::fromWCharArray((pinfo+list->currentRow())->pPrinterName);
  ui_settings.printer_port_name=QString::fromWCharArray((pinfo+list->currentRow())->pPortName);
  close();
}

SplitsPrinter *splits_printer;

SplitsPrinter::SplitsPrinter(QObject *parent) :
  QObject(parent),doc_name(L"Splits"),doc_mode(L"RAW")
{
  codec=QTextCodec::codecForName("Windows-1251");
}


void SplitsPrinter::printLine(const QByteArray& qline)
{
  strncpy(line,qline.data(),qline.size());
  line[(line_size=qline.size())]=0xa;
  WritePrinter(printer,line,++line_size,&spool_written);
}



void SplitsPrinter::printSplits(const QString& event,const QString& race,const QString& location, const QString& splits)
{
  printer_name[ui_settings.printer_name.toWCharArray(printer_name)]=0;
  printer_port_name[ui_settings.printer_port_name.toWCharArray(printer_port_name)]=0;
  if(!OpenPrinter(printer_name,&printer,NULL))
    {
      QMessageBox::warning(0,QObject::tr("Open printer"),
                           QObject::tr("Error opening printer"),
                           QMessageBox::Ok);
      return;
    }

  if(!StartDocPrinter(printer,1,reinterpret_cast<LPBYTE>(&doc_info))) {QMessageBox::warning(0,QObject::tr("Open printer"),
                                                                                            QObject::tr("Error opening doc"),
                                                                                             QMessageBox::Ok);return;}
  StartPagePrinter(printer);
  sendPrinterCommand(cmd_SetCodepage);
  sendPrinterCommand(cmd_DoubleHeight);
  printLine(event.toLocal8Bit());
  printLine(race.toLocal8Bit());
  sendPrinterCommand(cmd_NormalSize);
  sendPrinterCommand(cmd_FeedLine);
  sendPrinterCommand(cmd_DoubleStrikeOn);
  printLine(location.toLocal8Bit());
  sendPrinterCommand(cmd_DoubleStrikeOff);
  sendPrinterCommand(cmd_FeedLine);

  QStringList data=splits.split(';');
  printLine(QString(tr("Class: %1\nTeam: %2")).arg(data.at(0)).arg(data.at(1)).toLocal8Bit());
  data.removeFirst();data.removeFirst();
  printLine(data.takeFirst().toLocal8Bit());
  sendPrinterCommand(cmd_FeedLine);
  data.removeFirst();
  QString controls_taken=data.takeLast();
  QString distance_complete=data.takeLast();
  QString total_time=data.takeLast();
  QString distance=data.takeLast();
  QString avg_speed=data.takeLast();

  printLine(QString(tr("START - %1")).arg(data.takeFirst()).toLocal8Bit());
  int i=1;
  for(QString& s : data)
    printLine(QString("%1. %2").arg(i++).arg(s).toLocal8Bit());
  sendPrinterCommand(cmd_FeedLine);
  printLine(avg_speed.toLocal8Bit());
  printLine(distance.toLocal8Bit());
  printLine(controls_taken.toLocal8Bit());
  qDebug() << total_time.toLocal8Bit();
  printLine(total_time.toLocal8Bit());
  printLine(distance_complete.toLocal8Bit());

  for(i=0;i<4;i++) sendPrinterCommand(cmd_FeedLine);
  EndPagePrinter(printer);
  EndDocPrinter(printer);
  ClosePrinter(printer);
}


