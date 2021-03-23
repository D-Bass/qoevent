#include <QObject>
#include <QFile>
#include <QXmlStreamReader>

using iof_ret = std::tuple<int,int>;

class IOFReader
{

public:
  IOFReader(QFile* file);
  iof_ret readFile();
private:
  using course=struct {
    QString name;
    QString family;
    QString sequence;
  };
  typedef struct {
    QString cls;
    QString crs;
  } classcourse_t;

  using xtt=QXmlStreamReader::TokenType;
  float map_scale=0;
  static QXmlStreamAttributes last_attr;
  QXmlStreamReader xml;
  QString path;

};
