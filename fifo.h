#ifndef FIFO_H
#define FIFO_H


template <int rsize,class T>
class FIFO
{
public:
  FIFO(std::initializer_list{T*}) {}
  void push(T& val);
  T* pop();
  int size() const {return fill;}
  bool isEmpty() {return head==tail;}
  bool append(T* src);
private:
  T* head=pool;
  T* tail=pool;
  T* pool[rsize];
  unsigned int fill=0;
};

#endif // FIFO_H
