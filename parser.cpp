#include "parser.h"
extern "C"
{
#include "crc.h"
}
/*
void test_parser()
{
  QByteArray vec=QByteArray::fromRawData()={0x55,0x7,0,1,2,3,4,0x93,0x9a,
                         0x55,0x7,0,1,2,3,4,0x93,0x9a,0x55,6,
                         0x55,0x7,0,1,2,3,4,0x93,0x9a,0};
  Parser p;
  p.parse(vec);
  p.parse(vec);
}


RingBuffer<10> data;

void test_rb()
{
  int i;
  const char tv[]{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
  for(i=0;i<15;i++)
    data.push(i);
  for(i=0;i<15;i++)
    data.pop();
  data.append(tv,13);
  for(i=0;i<12;i++)
    data.pop();
  data.append(tv,7);
  for(i=0;i<6;i++)
    data.pop();
  data.append(tv,7);
  for(i=0;i<6;i++)
    data.pop();
  data.append(tv,7);
  for(i=0;i<6;i++)
    data.pop();
  data.append(tv,7);
  for(i=0;i<6;i++)
    data.pop();
  data.append(tv,2);
}
*/


void test_parser()
{
  QByteArray vec=QByteArray::fromRawData(
                "\x55\x7\x0\x1\x2\x3\x4\x93\x9a"
                "\x55\x7\x0\x1\x2\x3\x4\x93\x9a  "
                "\x55\x7\x0\x1\x2\x3\x4\x93\x9a   ",31);
  Parser p;
  p.parse(vec);
  p.parse(vec);
}


Parser::Parser()
{
  for (auto& x : pool){
    x=new QByteArray();
    x->reserve(MAX_PACKET_SIZE);
  }
}

Parser::~Parser()
{
  for(auto& x: pool) delete x;
}


void Parser::parse(QByteArray ar)
{
  bool pck_ready=false;
  buff.append(ar.constData(),ar.size());
  if(state>data) state=idle;
  while(!buff.isEmpty())
  {
    char c=buff.pop();
    switch(state)
      {
      case idle:
        if(c==0x55) {state=len;p=rx_data;}
        break;
      case len:
        rx_size=rx_cnt=c;
        *p++=c;
        if(rx_cnt<3 || rx_cnt>MAX_PACKET_SIZE)
          state=idle;
        else
          state=data;
        break;
      case data:
        *p++=c;
        if(!--rx_cnt)
        {
          state=idle;
          if(!crc16(rx_data,rx_size+1))
            pck_ready=true;
        }
      }
    if(pck_ready){
      push(rx_data,rx_size-1);pck_ready=false;
    }
  }
}


void Parser::push(const char* src,int size)
{
  if(fill==PKT_SIZE) return;
  pool[head]->clear();
  pool[head++]->append(QByteArray::fromRawData(src,size));
  ++fill;
  if(head==PKT_SIZE) head=0;
}

QByteArray Parser::pop()
{
  int temp;
  if(fill==0) return QByteArray();
  temp=tail;
  if(++tail==PKT_SIZE) tail=0;
  --fill;
  return *pool[temp];
}
