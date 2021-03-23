#include <QApplication>
#include <mainwindow.h>
#include <QTextCodec>
#include <QTranslator>
#include <QLocale>

int main(int argc, char *argv[])
{
  QApplication a(argc, argv);
  QTranslator translator;
  if (translator.load(":/trans_ru.qm"))
    a.installTranslator(&translator);
  MainWindow main_window;
  return a.exec();
}
