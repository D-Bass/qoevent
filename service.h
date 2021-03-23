#ifndef SERVICE_H
#define SERVICE_H


#include <QDialog>
#include <QBoxLayout>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>
#include <QFile>
#include <QComboBox>
#include <QSerialPortInfo>


class ServiceDialog : public QDialog
{
  Q_OBJECT
public:
  ServiceDialog(QWidget* parent=0);
private:
  QPushButton* bFirmwareCP;
  QPushButton* bFirmwareBS;
  QPushButton* bChecksumsBS;
  QPushButton* bRebootCP;
  QPushButton* bLogDebug;
  QPushButton* bLogSplits;
  QPushButton* bTextLog;
  QPushButton* bDecodeCard;
  QComboBox* cbPorts;
  QProgressBar* progress;
  QLabel* label;
  QVBoxLayout* lay;

  QFile file;
#pragma pack(1)
  struct Page
  {
    const uint8_t header[3]{0xaa,0xbb,0xcc};
    uint8_t cmd;
    char data[4096];
  } page;
#pragma pack()
  uint32_t page_size;
  enum States{idle,req_bldr_checksums,ack_bldr_checksums,
              req_app_checksums,ack_app_checksums,
              write_page,ack_page,exit_bldr,waiting_log,finished};
  int state=0;
  enum FW_Types {fw_bs,fw_cp};
  int firmware_type;
  int progress_cnt;
  void abort_connection();
  QString checksum_format(const QByteArray arr, const int size);
  QVector<QPushButton**> buttons{&bFirmwareBS,&bFirmwareCP,&bChecksumsBS,&bLogDebug,
                                 &bLogSplits,&bRebootCP,&bTextLog};
  void setCtrlEnabled(const bool val);
private slots:
  void getDebugLog();
  void getSplitsLog();
  void getBSChecksums();
  void updateCP();
  void updateBS();
  void rebootCP();
  void giveTextLog();
  void firmware_handler(QByteArray ack);
  bool openFile(const QString& title,const QString& type);
};


#endif // SERVICE_H
