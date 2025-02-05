#include <QApplication>
#include <QFontDatabase>
#include <QIcon>
#include <QTimer>

#include <GitQlient.h>
#include <QLogger.h>

using namespace QLogger;

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
   QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
   QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

   QApplication app(argc, argv);

   QApplication::setApplicationName("GitKlient");
   QApplication::setApplicationVersion(VER);
   QApplication::setWindowIcon(QIcon(":/icons/GitKlientLogoSVG"));

   QStringList repos;
   if (GitQlient::parseArguments(app.arguments(), &repos))
   {
      GitQlient mainWin;
      mainWin.setRepositories(repos);
      mainWin.show();

      QTimer::singleShot(0, &mainWin, &GitQlient::restorePinnedRepos);

      return app.exec();
   }

   return 0;
}
