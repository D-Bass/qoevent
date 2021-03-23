#include <QByteArray>
#include <cstring>

template <int rsize>
class RingBuffer
{
public:
  void push(const char val);
  char pop();
  int size() const {return fill;}
  bool isEmpty() {return head==tail;}
  bool append(const char* src,int size);
private:
  char* head=data;
  char* tail=data;
  char data[rsize];
  unsigned int fill=0;
};



template <int rsize>
void RingBuffer<rsize>::push(const char val)
{
  if(fill==rsize)return;
  *head++=val;
  if(head>=data+rsize) head=data;
  ++fill;
}

template <int rsize>
char RingBuffer<rsize>::pop()
{
  if(!fill)return 0;

  char* temp=tail;
  if(++tail>=data+rsize) tail=data;
  --fill;
  return *temp;
}

template <int rsize>
bool RingBuffer<rsize>::append(const char* src, int size)
{
  if (rsize< fill+size ) return false;
  int temp=head-data+size;
  if(temp <= rsize)
  {
    memcpy(head,src,size);
    head+=size;
    if(head==data+rsize) head=data;
    fill+=size;
  }
  else
  {
    temp=data+rsize-head;
    memcpy(head,src,temp);
    fill+=size;
    size-=temp;
    memcpy(data,src+temp,size);
    head=data+size;
  }

  return true;

}

