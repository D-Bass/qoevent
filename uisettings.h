#include <QObject>

struct UISettings : public QObject
{
  Q_OBJECT
public:
  UISettings(QObject* parent=0);
  ~UISettings();
  QString printer_name;
  QString printer_port_name;
  QString current_dir;
  qint64 current_event;
  qint64 current_race;

  bool events_dirty;
  bool races_dirty;
  bool serial_ready=0;
  bool account_teams=0;
  bool auto_printout=0;

  void update_current_dir(const QString& filename)
  {current_dir=filename.left(filename.lastIndexOf('/'));}

  void set_current_event(qint64 event) {current_event=event;
                                        events_dirty=true;
                                       }

  void set_current_race(qint64 race) {current_race=race;
                                      races_dirty=true;
                                     }


  void events_change_notify()
  {
    if(events_dirty)
      {events_dirty=0;emit events_changed();}
  }

  void races_change_notify()
  {
    if(races_dirty)
      {races_dirty=0;emit race_changed();}
  }

  void classes_change_notify() {emit classes_changed();}
  void courses_change_notify() {emit courses_changed();}
  void clscrs_change_notify() {emit clscrs_changed();}
  void teams_change_notify() {emit teams_changed();}
  void cp_change_notify() {emit cp_changed();}
  void protocol_change_notify() {emit protocol_changed();}
  void finish_list_change_notify() {emit finish_list_changed();}
  void serial_change_notify() {emit serial_changed();}
  void statusbar_message_notify(const QString msg,const int to) { emit statusbar_message(msg,to);}
signals:
  void serial_changed();
  void race_changed();
  void events_changed();
  void classes_changed();
  void courses_changed();
  void clscrs_changed();
  void teams_changed();
  void finish_list_changed();
  void cp_changed();
  void protocol_changed();
  void statusbar_message(const QString msg,const int timeout);
public slots:
  void account_teams_changed(bool state) {account_teams=state;}

};

extern UISettings ui_settings;
