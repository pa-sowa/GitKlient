#include <GitBase.h>
#include <GitSubtree.h>

#include <QLogger.h>

using namespace QLogger;

GitSubtree::GitSubtree(const QSharedPointer<GitBase> &gitBase)
   : mGitBase(gitBase)
{
}

GitExecResult GitSubtree::add(const QString &url, const QString &ref, const QString &name, bool squash)
{
   QLog_Debug("UI", "Adding a subtree");

   auto cmd = QString("git subtree add --prefix=%1 %2 %3").arg(name, url, ref);
   QLog_Trace("Git", QString("Adding a subtree: {%1}").arg(cmd));

   if (squash)
      cmd.append(" --squash");

   auto ret = mGitBase->run(cmd);

   if (ret.output.contains("Cannot"))
      ret.success = false;

   return ret;
}

GitExecResult GitSubtree::pull(const QString &url, const QString &ref, const QString &prefix) const
{
   QLog_Debug("UI", "Pulling a subtree");

   const auto cmd = QString("git subtree pull --prefix=%1 %2 %3").arg(prefix, url, ref);

   QLog_Trace("Git", QString("Pulling a subtree: {%1}").arg(cmd));

   auto ret = mGitBase->run(cmd);

   if (ret.output.contains("Cannot"))
      ret.success = false;

   return ret;
}

GitExecResult GitSubtree::push(const QString &url, const QString &ref, const QString &prefix) const
{
   QLog_Debug("UI", "Pushing changes to a subtree");

   const auto cmd = QString("git subtree push --prefix=%1 %2 %3").arg(prefix, url, ref);

   QLog_Trace("Git", QString("Pushing changes to a subtree: {%1}").arg(cmd));

   auto ret = mGitBase->run(cmd);

   if (ret.output.contains("Cannot"))
      ret.success = false;

   return ret;
}

GitExecResult GitSubtree::merge(const QString &sha) const
{
   QLog_Debug("UI", "Merging changes from the remote of a subtree");

   const auto cmd = QString("git subtree merge %1").arg(sha);

   QLog_Trace("Git", QString("Merging changes from the remote of a subtree: {%1}").arg(cmd));

   auto ret = mGitBase->run(cmd);

   if (ret.output.contains("Cannot"))
      ret.success = false;

   return ret;
}

GitExecResult GitSubtree::list() const
{
   QLog_Debug("UI", "Listing all subtrees");

   const auto cmd = QString("git log --pretty=format:%b --grep=git-subtree-dir");

   QLog_Trace("Git", QString("Listing all subtrees: {%1}").arg(cmd));

   return mGitBase->run(cmd);
}
