#include <QSerialPort>
#include <QSerialPortInfo>
#include <QIODevice>
#include <QTimer>
#include <QSignalMapper>
#include "parser.h"

const char EI2C=200;          /* I2C bus eror */
const char ETIMEFMT=201;    /* received time structure is corrupted  */
const char EMFWRITE=202;    /* write to mifare failed */
const char EMFREAD=203;     /* read from mifare failed */
const char EMFVERIFY=204;   /* verify failed */
const char EMFAUTH=205;   /* authentication failed */
const char EMFHOSTILE=206;  /* unexpected uid in the field */
const char EMFPRESENT=207;
const char MODE_CP=0x55;
const char MODE_START=0xaa;

class PortProbe: public QObject
{
  Q_OBJECT
  enum States {try_next_port,waiting_ack,finish};
public:
  PortProbe(QObject* parent=0) : QObject(parent) {}
  void findStation();
private slots:
  void handler(QByteArray recv);
private:
  void abort(const bool success);
  int state;
  QList<QSerialPortInfo>::const_iterator portinfo;
  QList<QSerialPortInfo> portlist;
  QSerialPort port;
};


class BaseStation : public QObject
{
  Q_OBJECT
public:
  BaseStation(QObject* parent=0);
  bool sendCmd(QByteArray cmd, int timeout);
  void enableHeaders(){headers_enabled=true;busy=false;cmd_tmr.stop();}
  void disableHeaders(){headers_enabled=false;}
  bool isPortAvailable() {return port.isOpen();}
  bool openPort(const QString& name);
  void setPortBaudrate(QSerialPort::BaudRate br) {port.setBaudRate(br);}
  void closePort();
public slots:
  void init() {pp.findStation();}
signals:
  void comm_result(QByteArray data);
private:
  PortProbe pp;
  bool headers_enabled=true;
  bool busy=false;
  bool reply_valid;
  int state=idle;
  enum{idle,write_bs,open_transparent,reopen_transparent,write_cp,write_cp_firmware,
       read_cp,close_transparent,read_bs,read_bs_bldr,finish};
  Parser p;
  QSerialPort port;
  QTimer cmd_tmr; //timeout of the whole command
  QTimer tmr; //internal timeout
  QByteArray curr_cmd;
  QByteArray rx_data;
  const char synch[5]{'S','Y','N','C','H'};
  char tx_data[256];
  char card[64][17];
  qint64 assemble_packet(const QByteArray src);
  enum SerialCallers{tmr_to,cmd_tmr_to,rxready};
  QSignalMapper* mapper;
private slots:

  void serial_handler(int event);
};

extern BaseStation* bs;
