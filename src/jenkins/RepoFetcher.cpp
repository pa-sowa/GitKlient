#include "RepoFetcher.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include <QLogger.h>

using namespace QLogger;

namespace Jenkins
{

RepoFetcher::RepoFetcher(const IFetcher::Config &config, const QString &url, QObject *parent)
   : IFetcher(config, parent)
   , mUrl(url)
{
}

RepoFetcher::~RepoFetcher()
{
   QLog_Debug("Jenkins", "Destroying repo fetcher object.");
}

void RepoFetcher::triggerFetch()
{
   get(mUrl);
}

void RepoFetcher::processData(const QJsonDocument &json)
{
   const auto jsonObject = json.object();

   if (!jsonObject.contains(QStringLiteral("views")))
   {
      QLog_Info("Jenkins", "Views are absent.");
      return;
   }

   const auto views = jsonObject[QStringLiteral("views")].toArray();
   QVector<JenkinsViewInfo> viewsInfo;
   viewsInfo.reserve(views.count());

   for (const auto &view : views)
   {
      auto appendView = false;
      const auto viewObject = view.toObject();
      const auto jobs = viewObject[QStringLiteral("jobs")].toArray();

      for (const auto &job : jobs)
      {
         QJsonObject jobObject = job.toObject();
         QString url;

         if (jobObject.contains(QStringLiteral("url")))
            url = jobObject[QStringLiteral("url")].toString();

         if (jobObject[QStringLiteral("_class")].toString().contains("WorkflowMultiBranchProject"))
         {
            JenkinsViewInfo info;
            info.url = url;

            if (jobObject.contains(QStringLiteral("name")))
               info.name = jobObject[QStringLiteral("name")].toString();

            info.name.replace("%2F", "/");
            info.name.replace("%20", " ");

            viewsInfo.append(info);
         }
         else if (jobObject[QStringLiteral("_class")].toString().contains("WorkflowJob"))
            appendView = true;
      }

      if (appendView)
      {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
         const auto flag = Qt::SkipEmptyParts;
#else
         const auto flag = QString::SkipEmptyParts;
#endif

         const auto url = viewObject[QStringLiteral("url")].toString();
         JenkinsViewInfo info;
         info.name = url.split("/", flag).constLast();
         info.url = url + QString::fromUtf8("api/json");
         info.isCustomUrl = true;
         viewsInfo.prepend(info);
      }
   }

   emit signalViewsReceived(viewsInfo);
}

}
