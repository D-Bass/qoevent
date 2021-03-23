#ifndef SPOOL_H
#define SPOOL_H

#include <QPushButton>
#include <QDialog>
#include <QComboBox>
#include <QVBoxLayout>
#include <QListWidget>
#include <QTextCodec>
#include <Windows.h>
#include <winspool.h>



class PrinterSelector : public QDialog
{
  Q_OBJECT
public:
  PPRINTER_INFO_5 pinfo;
  PrinterSelector();
  QSize sizeHint() const {return QSize(300,300);}
  QVBoxLayout *layout;
  QListWidget *list;
  QPushButton *bOk,*bCancel;
private:

  //WinAPI
  unsigned char printer_info[4096];
  unsigned long buff_size;
  unsigned long num_printers;

private slots:
  void selectPrinter();
};


class SplitsPrinter : public QObject
{
  Q_OBJECT
public:
  SplitsPrinter(QObject *parent=0);
  void printSplits(const QString& event, const QString &race, const QString &location, const QString& splits);
private:

  char line[128];
  int line_size;
  DWORD spool_written;
  HANDLE printer;
  QTextCodec *codec;

  WCHAR doc_name[128];
  WCHAR doc_mode[5];
  DOC_INFO_1 doc_info{doc_name,NULL,doc_mode};
  WCHAR printer_name[128];
  WCHAR printer_port_name[128];

  QByteArray cmd_SetCodepage {"\x1b\x74\x2e",3};//2e 17
  QByteArray cmd_DoubleSize  {"\x1b\x21\x30",3};
  QByteArray cmd_DoubleHeight{"\x1b\x21\x10",3};
  QByteArray cmd_NormalSize  {"\x1b\x21\x00",3};
  QByteArray cmd_DoubleStrikeOn {"\x1b\x47\x1",3};
  QByteArray cmd_DoubleStrikeOff{"\x1b\x47\x0",3};
  QByteArray cmd_FeedLine{"\xa"};

  void printLine(const QByteArray& qline);

  void sendPrinterCommand(QByteArray &cmd)
  { WritePrinter(printer,cmd.data(),cmd.size(),&spool_written); }



};

extern SplitsPrinter *splits_printer;

#endif // SPOOL_H
