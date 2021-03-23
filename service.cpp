#include "service.h"
#include "comm.h"
#include <QFileDialog>
#include "uisettings.h"
#include "QMessageBox"
#include <QDebug>
#include <QDateTime>
#include <QSqlQuery>

ServiceDialog::ServiceDialog(QWidget *parent)
  :QDialog(parent)
{
  QList<QSerialPortInfo> ports=QSerialPortInfo::availablePorts();
  setWindowTitle(tr("Service Menu"));
  bFirmwareCP=new QPushButton(tr("Update control firmware"));
  bFirmwareBS=new QPushButton(tr("Update base station firmware"));
  bRebootCP=new QPushButton(tr("Reboot control station via bootloader"));
  bChecksumsBS=new QPushButton(tr("Show BS checksums"));
  bLogDebug=new QPushButton(tr("Get control inner state debug"));
  bLogSplits=new QPushButton(tr("Get splits from backup memory"));
  bTextLog=new QPushButton(tr("Apply log for current race"));
  cbPorts=new QComboBox();
  for (QSerialPortInfo& l : ports)
    cbPorts->addItem(l.portName());
  label=new QLabel;
  progress=new QProgressBar;
  progress->setRange(0,100);
  lay=new QVBoxLayout;
  lay->addWidget(bFirmwareCP,1);
  lay->addWidget(bFirmwareBS,1);
  lay->addWidget(bChecksumsBS,1);
  lay->addWidget(bLogDebug,1);
  lay->addWidget(bRebootCP,1);
  lay->addWidget(bLogSplits,1);
  lay->addWidget(bTextLog);
  lay->addSpacing(30);
  if(!bs->isPortAvailable())
    lay->addWidget(cbPorts);
  lay->addWidget(label);
  lay->addWidget(progress,1);
  lay->setMargin(15);
  lay->setContentsMargins(10,10,10,10);
  setLayout(lay);
  connect(bChecksumsBS,SIGNAL(clicked()),SLOT(getBSChecksums()));
  connect(bFirmwareBS,SIGNAL(clicked()),SLOT(updateBS()));
  connect(bFirmwareCP,SIGNAL(clicked()),SLOT(updateCP()));
  connect(bRebootCP,SIGNAL(clicked()),SLOT(rebootCP()));
  connect(bLogDebug,SIGNAL(clicked()),SLOT(getDebugLog()));
  connect(bLogSplits,SIGNAL(clicked()),SLOT(getSplitsLog()));
  connect(bTextLog,SIGNAL(clicked()),SLOT(giveTextLog()));
}


bool ServiceDialog::openFile(const QString& title,const QString& type)
{
  if(!bs->isPortAvailable())
  if(!bs->openPort(cbPorts->currentText())) return false;
  QString filename=QFileDialog::getOpenFileName(this,title,
                                    ui_settings.current_dir,
                                    type);
  if(filename.isEmpty()) return false;
  file.setFileName(filename);
  if(!file.open(QIODevice::ReadOnly)) return false;
  file.read(page.data,4);
  page_size=QString::fromLatin1(page.data,4).toInt()+20;
  Q_ASSERT(page_size<=sizeof(page));
  return true;
}


void ServiceDialog::getDebugLog()
{
  file.setFileName("cshwlog.bin");
  if(!file.open(QIODevice::WriteOnly)) return;
  bs->sendCmd(QByteArray::fromRawData("\x55",1),16000);
  state=waiting_log;
  connect(bs,SIGNAL(comm_result(QByteArray)),SLOT(firmware_handler(QByteArray)));
  setCtrlEnabled(false);
}

void ServiceDialog::getSplitsLog()
{
  file.setFileName("backup.bin");
  if(!file.open(QIODevice::WriteOnly)) return;
  bs->sendCmd(QByteArray::fromRawData("\x58",1),140000);
  state=waiting_log;
  connect(bs,SIGNAL(comm_result(QByteArray)),SLOT(firmware_handler(QByteArray)));
  setCtrlEnabled(false);
}

void ServiceDialog::updateCP()
{
  if(!openFile(tr("Open Control firmware"),tr("Control station firmware (*.csf)"))) return;
  connect(bs,SIGNAL(comm_result(QByteArray)),SLOT(firmware_handler(QByteArray)));
  state=write_page;
  firmware_type=fw_cp;
  progress_cnt=4;
  setCtrlEnabled(false);
  bs->sendCmd(QByteArray::fromRawData("\x57",1),2000);
}

void ServiceDialog::updateBS()
{
  if(!openFile(tr("Open BS firmware"),tr("Base station firmware (*.msf)"))) return;
  connect(bs,SIGNAL(comm_result(QByteArray)),SLOT(firmware_handler(QByteArray)));
  state=write_page;
  firmware_type=fw_bs;
  progress_cnt=4;
  setCtrlEnabled(false);
  bs->sendCmd(QByteArray::fromRawData("\x85",1),1000);
}



void ServiceDialog::getBSChecksums()
{
  if(!openFile(tr("Open BS firmware"),tr("Base station firmware (*.msf)"))) return;
  progress_cnt=0;
  connect(bs,SIGNAL(comm_result(QByteArray)),SLOT(firmware_handler(QByteArray)));
  state=req_bldr_checksums;  
  setCtrlEnabled(false);
  bs->sendCmd(QByteArray::fromRawData("\x85",1),1000);
}


void ServiceDialog::abort_connection()
{
  state=finished;
  disconnect(bs,SIGNAL(comm_result(QByteArray)),this,SLOT(firmware_handler(QByteArray)));
  file.close();
  bs->closePort();
  setCtrlEnabled(true);
  QTimer::singleShot(1000,bs,SLOT(init()));
}

QString ServiceDialog::checksum_format(const QByteArray arr,const int size)
{
  QString chs;
  const uint8_t* v=reinterpret_cast<const unsigned char*>(arr.constData());
  for(int i=0;i<size;i+=2)
    chs.append(QString::asprintf("%x \%x\n",(*(uint32_t*)(v+i*4)),(*(uint32_t*)(v+(i+1)*4))));
  return chs;
}

void ServiceDialog::firmware_handler(QByteArray ack)
{
  switch(state)
    {
    case req_bldr_checksums:
      page.cmd='X';
      bs->sendCmd(QByteArray::fromRawData(reinterpret_cast<const char*>(&page),4+page_size),2000);
      state=ack_bldr_checksums;
      break;
    case ack_bldr_checksums:
      if(ack.isEmpty() || ack.size()!=8) {abort_connection();break;}
      label->setText(QString("BootLoader checksum: \n")+checksum_format(ack,2));
      page.cmd='Y';
      bs->sendCmd(QByteArray::fromRawData(reinterpret_cast<const char*>(&page),4+page_size),2000);
      state=ack_app_checksums;
      break;
    case ack_app_checksums:
      if(ack.isEmpty() || ack.size()!=24) {abort_connection();break;}
      label->setText(label->text()+"\nApplication checksum: \n"+checksum_format(ack,6));
      page.cmd='Z';
      bs->sendCmd(QByteArray::fromRawData(reinterpret_cast<const char*>(&page),4+page_size),500);
      state=exit_bldr;
      break;


    case ack_page:

      progress_cnt+=page_size;
      progress->setValue(progress_cnt*100/file.size());

      if(firmware_type==fw_bs)
        {
          if(ack.isEmpty()) {abort_connection();break;}
          qDebug() << ack.at(0);


          //if end of file or feof from station
          if(file.atEnd() || ack.at(0)=='F')
            {

              page.cmd='Z'; //exit bootloader
              bs->sendCmd(QByteArray::fromRawData(reinterpret_cast<const char*>(&page),4+page_size),800);
              state=exit_bldr;
              break;
            }
        }
      else
        {
          if(file.atEnd())
            {
              page.cmd='Z'; //exit bootloader
              bs->sendCmd(QByteArray::fromRawData(reinterpret_cast<const char*>(&page),4+page_size),500);
              state=exit_bldr;
              break;
            }
        }


    case write_page:
      if(firmware_type==fw_cp)
        bs->setPortBaudrate(QSerialPort::Baud4800);
      file.read(page.data,page_size);
      page.cmd='W';
      bs->sendCmd(QByteArray::fromRawData(
                    reinterpret_cast<const char*>(&page),
                    4+page_size),
                    firmware_type==fw_bs ? 500 : (progress_cnt<file.size()-1024 ? 800 : 2000));
      state=ack_page;
      break;


    case exit_bldr:
      if(firmware_type==fw_bs)
        {
          abort_connection();
          if(progress_cnt==file.size())
            QMessageBox::information(0,tr("Firmware"),
                                tr("Firmware update complete"),
                                QMessageBox::Ok);
        }
      else
        {
          //first disconnect then close transparent!
          disconnect(bs,SIGNAL(comm_result(QByteArray)),this,SLOT(firmware_handler(QByteArray)));
          file.close();
          setCtrlEnabled(true);
          bs->enableHeaders();
          bs->sendCmd(QByteArray::fromRawData("\x84",1),0);
          state=finished;
        }
      break;

    case  waiting_log:
      if(!ack.isEmpty())
        {
          file.write(ack);
          break;
        }
      file.close();
      setCtrlEnabled(true);
      disconnect(bs,SIGNAL(comm_result(QByteArray)),this,SLOT(firmware_handler(QByteArray)));
      break;
    case finished:
    default:
      abort_connection();
    }
}


void ServiceDialog::rebootCP()
{
  bs->sendCmd(QByteArray::fromRawData("\x56",1),800);
}


void ServiceDialog::setCtrlEnabled(const bool val)
{
  for(QPushButton** b : buttons)
    (*b)->setDisabled(!val);
}



void ServiceDialog::giveTextLog()
{
  unsigned char rec[8];
  if(!ui_settings.current_race) return;
  QString filename=QFileDialog::getOpenFileName(this,tr("Open binary log"),
                                    ui_settings.current_dir,
                                    tr("Binary log gile (*.bin)"));
  if(filename.isEmpty()) return;
  file.setFileName(filename);
  QFile tlog("backup.txt");
  QString line;
  file.open(QIODevice::ReadOnly);
  uint32_t time;
  QString card;
  while(file.read(1)!=QByteArray::fromRawData("\x55",1));
    if(file.atEnd())
      {
        file.close();
        return;
      }
  tlog.open(QIODevice::WriteOnly | QIODevice::Text);
  QSqlQuery q;
  q.exec(QString("select chip,competitor.name,start_time from protocol join team_member using(tm_id) join competitor using(comp_id) join team using(team_id) join class using(class_id) where race_id==%1")
         .arg(ui_settings.current_race));
  while(!file.atEnd())
    {
      file.read(reinterpret_cast<char*>(rec),8);
      if(*reinterpret_cast<uint32_t*>(rec)==0xffffffff) continue;
      line=QDateTime::fromTime_t(*reinterpret_cast<uint32_t*>(rec)).toString("\ndd.MM.yyyy hh:mm:ss : ");
      card=QString::asprintf("%02x%02x%02x%02x",
                        rec[4],rec[5],rec[6],rec[7]);
      line.append(card);
      q.seek(-1);
      while(q.next())
        if (q.value(0).toString()==card)
          line.append(QString(tr(" : %1, Start: %2"))
              .arg(q.value(1).toString())
              .arg(QDateTime::fromTime_t(q.value(2).toUInt()).toString("dd.MM.yyyy hh:mm:ss")));

      tlog.write(line.toLocal8Bit());
    }
  file.close();
  tlog.close();
}

