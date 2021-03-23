#include <QDebug>
#include "comm.h"
#include "uisettings.h"
#include <QMessageBox>
#include <cstring>

extern "C"
{
#include "crc.h"
}

BaseStation *bs;


/*
enum COMMANDS_CP {
  CP_SET_TIMEDATE=0x50,
  CP_SET_MODE=0x51,
  CP_GET_STATE=0x52,
  CP_SET_ACTIVE_TIMEOUT=0x53,
  CP_POWERDOWN=0x54,
  CP_GET_ERROR_LOG=0x55,
  CP_REBOOT=0x56,
  CP_ENTER_BOOTLOADER=0x57,
  CP_GET_BACKUP=0x58
};

enum COMMANDS_MS {
  //fill mifare with 1's, check, fill with 0's, check and prepare for start
  MS_PREPARE_TAG=0x80,
  //read card if present
  MS_READ_TAG=0x81,
  MS_ENTER_BOOTLOADER=0x85,
  MS_SET_TIME=0x82,
  MS_EN_CP_COMM=0x83,
  MS_DIS_CP_COMM=0x84
};

};*/




qint64 BaseStation::assemble_packet(const QByteArray src)
{
  int size=src.size();
  memcpy(tx_data,synch,sizeof(synch));
  memcpy(tx_data+sizeof(synch)+1,src.constData(),size);
  tx_data[sizeof(synch)]=size+2;
  *(uint16_t*)(tx_data+sizeof(synch)+1+size)=crc16(tx_data+sizeof(synch),1+size);
  return sizeof(synch)+1+size+2;
}




void PortProbe::findStation()
{
  state=try_next_port;
  connect(bs,SIGNAL(comm_result(QByteArray)),SLOT(handler(QByteArray)));
  portlist=QSerialPortInfo::availablePorts();
  portinfo=portlist.begin();
  handler(QByteArray());
}

void PortProbe::abort(const bool success)
{
  state=finish;
  disconnect(bs,SIGNAL(comm_result(QByteArray)),this,SLOT(handler(QByteArray)));
  if(!success)
    {
      QMessageBox::warning(0,tr("Searching base station"),
                        tr("Base station not found"),
                        QMessageBox::Ok);
      bs->closePort();
      ui_settings.serial_ready=false;
    }
  else
    ui_settings.serial_ready=true;
  ui_settings.serial_change_notify();
}

void PortProbe::handler(QByteArray recv)
{
  switch(state)
    {
    case waiting_ack:
      if(!recv.isEmpty())
        {
          abort(true);
          break;
        }
      else
        bs->closePort();
        ++portinfo;
    case try_next_port:
      if(portinfo!=portlist.end())
        {
          if(bs->openPort(portinfo->portName()))
            bs->sendCmd(QByteArray::fromRawData("\x82\x20\x26\x22\x20\x01\x02\x17",8),500);
          state=waiting_ack;
        }
      else
          abort(false);
      break;
    }
}



//=========================== BASE STATION ==========================//

BaseStation::BaseStation(QObject* parent)
  : QObject(parent)
{  
  cmd_tmr.setSingleShot(true);
  cmd_tmr.setTimerType(Qt::CoarseTimer);
  tmr.setSingleShot(true);
  tmr.setTimerType(Qt::CoarseTimer);
  mapper=new QSignalMapper(this);
  connect(&port,SIGNAL(readyRead()),mapper,SLOT(map()));
  connect(&tmr,SIGNAL(timeout()),mapper,SLOT(map()));
  connect(&cmd_tmr,SIGNAL(timeout()),mapper,SLOT(map()));
  mapper->setMapping(&port,rxready);
  mapper->setMapping(&tmr,tmr_to);
  mapper->setMapping(&cmd_tmr,cmd_tmr_to);
}



//============================== SERIAL HANDLER ================================//





void BaseStation::serial_handler(int event)
{
  if(event==cmd_tmr_to && state!=finish && headers_enabled)
    {
      qDebug() << "cmd_tmr timeout" <<headers_enabled;
      state=close_transparent;
    }
  switch(state)
    {
    case idle:
      port.readAll();
      qDebug() << "Non requested data arrived";
      tmr.stop();cmd_tmr.stop();
      break;

    case open_transparent:
      port.setBaudRate(QSerialPort::Baud2400);
      port.write("\x0\x0",2);port.flush();
      tmr.start(50);
      state=write_cp;
      break;
    case write_cp:
      port.write(tx_data,assemble_packet(curr_cmd));
      port.flush();
      if(curr_cmd.at(0)==0x52 || curr_cmd.at(0)==0x55)
        {
          tmr.start(curr_cmd.at(0)==0x52 ? 1000 : 5000);
          state=read_cp;
          break;
        }
      if(curr_cmd.at(0)==0x58)
        {
          tmr.start(3000);
          state=read_cp;
          break;
        }
      if(curr_cmd.at(0)==0x57)
        {
          cmd_tmr.stop();
          state=reopen_transparent;
          tmr.start(200);
          break;
        }
      else
        {
          tmr.start(500);
          state=close_transparent;
          break;
        }

    case reopen_transparent:
      if(event==rxready) break; //ignore garabage on rx
      port.write(tx_data,assemble_packet(QByteArray::fromRawData("\x83",1)));
      qDebug() << "reopen" << cmd_tmr.remainingTime();
      tmr.start(300);
      cmd_tmr.stop();
      state=write_cp_firmware;
      break;
    case write_cp_firmware:
      if(event==rxready) break; //ignore garabage on rx
      disableHeaders();
      emit comm_result(rx_data);
      break;
    case read_cp:
      if(event==tmr_to)
        {
          if( (uint8_t)curr_cmd.at(0)==0x55 || (uint8_t)curr_cmd.at(0)==0x58 )
            rx_data=port.readAll();
          if(!rx_data.isEmpty()) emit comm_result(rx_data);
          rx_data.clear();
        }
      else
        {
          if((uint8_t)curr_cmd.at(0)==0x55 || (uint8_t)curr_cmd.at(0)==0x58)
            {
              emit comm_result(port.readAll());
              tmr.start(3000);
              break;
            }
          else
            {
              tmr.stop();
              p.parse(port.readAll());
              if(!p.isEmpty())
                {
                  reply_valid=true;
                  rx_data=p.pop();
                }
            }
        }
      state=close_transparent; //jump to the next line immediatly redneck style watchout
    case close_transparent:
      port.setBaudRate(QSerialPort::Baud2400);
      port.write(tx_data,assemble_packet(QByteArray::fromRawData("\x84",1)));
      port.flush();
      tmr.start(300);
      state=finish;
      break;
    case read_bs:
      if((uint8_t)curr_cmd.at(0)==0x85)
        {
          state=read_bs_bldr;
          rx_data.clear();
          emit comm_result(rx_data);
        }
      else
        {
          p.parse(port.readAll());
          if((uint8_t)curr_cmd.at(0)==0x80)  //if clearing the tag;
            {
              if(!p.isEmpty())
                {
                  reply_valid=true;
                  rx_data=p.pop();
                  state=finish;
                  tmr.start(0);
                }
            }
          if((uint8_t)curr_cmd.at(0)==0x81)
            {
              while(!p.isEmpty())
                {
                  rx_data=p.pop();
                  if((uint8_t)rx_data.at(1)==(uint8_t)EMFPRESENT)
                    {
                      state=finish;tmr.start(0);
                      break;
                    }
                  if((uint8_t)rx_data.at(1)==0xff)
                    {
                      reply_valid=true;
                      rx_data=QByteArray::fromRawData(card[0],sizeof(card));
                      state=finish;tmr.start(0);
                      break;
                    }
                  card[(uint8_t)rx_data.at(2)][16]=rx_data.at(1);
                  memcpy(card[(uint8_t)rx_data.at(2)],rx_data.constData()+3,rx_data.at(0)-4);
                }
            }
          if((uint8_t)curr_cmd.at(0)==0x82)
            {
              rx_data=p.pop();
              if(!rx_data.isEmpty()) reply_valid=true;
              state=finish;
              tmr.start(0);
            }
        }


      break;
    case read_bs_bldr:

      rx_data.clear();
      rx_data=port.readAll();
      emit comm_result(rx_data);
      break;

    case finish:
      port.setBaudRate(QSerialPort::Baud38400);
      state=idle;
      tmr.stop();
      cmd_tmr.stop();
      if(!reply_valid)
        rx_data.clear();
      emit comm_result(rx_data);
      busy=false;
      break;
    default:
      tmr.stop();cmd_tmr.stop();
      qDebug() << "illegal state";
   }
}


bool BaseStation::sendCmd(QByteArray cmd,int timeout)
{
  if(busy && headers_enabled) return false;
  if(!port.isOpen()) return false;
  busy=true;
  reply_valid=false;
  curr_cmd=cmd;
  port.readAll(); //flush read buffer;
  rx_data.clear();
  if(!headers_enabled)
    {
      port.write(cmd);
      port.flush();
    }
  else
    {
      if((uint8_t)cmd.at(0)<=0x60)  //for CP commands, first enable transparent mode with BS cmd=0x83
        {
          state=open_transparent;
          port.write(tx_data,assemble_packet(QByteArray::fromRawData("\x83",1)));
          port.flush();
          tmr.start(200);
        }
      else
        {
          memset(card,0,17*64);
          state=read_bs;
          if((uint8_t)cmd.at(0)==0x85)
            disableHeaders();
          if((uint8_t)cmd.at(0)==0x84)
            {
              state=close_transparent;
              serial_handler(cmd_tmr_to);
              return true;
            }
          else
            {
              port.write(tx_data,assemble_packet(cmd));
              port.flush();
            }
          //no need to start tmr here, would be redunant of cmd_tmr
        }
    }
  cmd_tmr.start(timeout);
  return true;
}

bool BaseStation::openPort(const QString &name)
{
  port.setPortName(name);
  if(!port.open(QIODevice::ReadWrite)) return false;
  port.setBaudRate(QSerialPort::Baud38400);
  port.setDataBits(QSerialPort::Data8);
  port.setParity(QSerialPort::NoParity);
  port.setStopBits(QSerialPort::OneStop);
  port.setFlowControl(QSerialPort::NoFlowControl);
  connect(mapper,SIGNAL(mapped(int)),SLOT(serial_handler(int)));
  busy=false;
  enableHeaders();
  return true;
}

void BaseStation::closePort()
{
  port.close();
  disconnect(mapper,SIGNAL(mapped(int)),this,SLOT(serial_handler(int)));
}
