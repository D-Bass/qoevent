#include <QXmlStreamReader>




struct cp_point
{
  QString name;
  double lat=0;
  double lon=0;
  double x=0;
  double y=0;
  QString desc;
};



class GPXReader : public QObject
{
  Q_OBJECT

public:
  GPXReader(QObject* parent=0);
public slots:
  void readFile();
};


extern GPXReader gpxreader;
