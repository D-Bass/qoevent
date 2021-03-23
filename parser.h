#include <QByteArray>
#include <QVector>
#include "ringbuffer.h"

const uint8_t MAX_PACKET_SIZE=126;
const int PKT_SIZE=64;

void test_parser();

class Parser {
public:
  Parser();
  ~Parser();
  void parse(const QByteArray ar);
  bool isEmpty() const {return !fill;}
  QByteArray pop();
  void push(const char* src,int size);
  void clear();

private:
  enum STATES{idle,len,data};
  uint32_t state=idle;
  uint8_t rx_cnt;
  uint8_t rx_size;
  char* p;
  char rx_data[MAX_PACKET_SIZE+2];
  RingBuffer<8192> buff;

  QByteArray* pool[PKT_SIZE];
  int fill=0;
  int head=0,tail=0;

};



