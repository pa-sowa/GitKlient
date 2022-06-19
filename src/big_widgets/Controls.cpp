#include "Controls.h"

#include <BranchDlg.h>
#include <GitBase.h>
#include <GitCache.h>
#include <GitConfig.h>
#include <GitQlientSettings.h>
#include <GitQlientStyles.h>
#include <GitRemote.h>
#include <GitStashes.h>
#include <QLogger.h>

#include <QApplication>
#include <QButtonGroup>
#include <QHBoxLayout>
#include <QMenu>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>

using namespace QLogger;

Controls::Controls(const QSharedPointer<GitCache> &cache, const QSharedPointer<GitBase> &git, QWidget *parent)
   : QFrame(parent)
   , mCache(cache)
   , mGit(git)
   , mMergeWarning(new QPushButton(tr("WARNING: There is a merge pending to be committed! Click here to solve it.")))
   , mBtnGroup(new QButtonGroup())
{
   setAttribute(Qt::WA_DeleteOnClose);

   mActionGroup = new QActionGroup(this);
   mHistoryAction = new QAction(this);
   mHistoryAction->setText(tr("History"));
   mHistoryAction->setCheckable(true);
   mHistoryAction->setIcon(QIcon::fromTheme("vcs-commit", QIcon(":/icons/git_orange")));
   mHistoryAction->setToolTip(tr("View History"));
   mActionGroup->addAction(mHistoryAction);

   mDiffAction = new QAction(this);
   mDiffAction->setText(tr("Diff"));
   mDiffAction->setCheckable(true);
   mDiffAction->setIcon(QIcon::fromTheme("vcs-diff", QIcon(":/icons/diff")));
   mDiffAction->setToolTip(tr("Diff"));
   mDiffAction->setEnabled(false);
   mActionGroup->addAction(mDiffAction);

   mBlameAction = new QAction(this);
   mBlameAction->setText(tr("Blame"));
   mBlameAction->setCheckable(true);
   mBlameAction->setIcon(QIcon::fromTheme("actor", QIcon(":/icons/blame")));
   mBlameAction->setToolTip(tr("Blame"));
   mActionGroup->addAction(mBlameAction);

   mTerminalAction = new QAction(this);
   mTerminalAction->setText(tr("Terminal"));
   mTerminalAction->setCheckable(true);
   mTerminalAction->setIcon(QIcon(":/icons/terminal"));
   mTerminalAction->setToolTip(tr("Terminal"));
   // mTerminalAction->setVisible(false);
   mActionGroup->addAction(mTerminalAction);

   const auto pullMenu = new QMenu(this);
   auto action = pullMenu->addAction(tr("Fetch all"));
   connect(action, &QAction::triggered, this, &Controls::fetchAll);

   action = pullMenu->addAction(tr("Prune"));
   connect(action, &QAction::triggered, this, &Controls::pruneBranches);
   pullMenu->addSeparator();

   mPullAction = new QAction(this);
   mPullAction->setText(tr("Pull"));
   mPullAction->setToolTip(tr("Pull"));
   mPullAction->setIcon(QIcon::fromTheme("vcs-pull", QIcon(":/icons/git_pull")));
   mPullAction->setMenu(pullMenu);

   mPushAction = new QAction(this);
   mPushAction->setText(tr("Push"));
   mPushAction->setIcon(QIcon::fromTheme("vcs-push", QIcon(":/icons/git_push")));
   mPushAction->setToolTip(tr("Push"));

   mRefreshAction = new QAction(this);
   mRefreshAction->setText(tr("Refresh"));
   mRefreshAction->setIcon(QIcon::fromTheme("view-refresh", QIcon(":/icons/refresh")));
   mRefreshAction->setToolTip(tr("Refresh"));

   mGitPlatformAction = createGitPlatformAction();
   if (mGitPlatformAction)
   {
      mActionGroup->addAction(mGitPlatformAction);
   }

   mConfigAction = new QAction(this);
   mConfigAction->setText(tr("Settings"));
   mConfigAction->setIcon(QIcon::fromTheme("configure", QIcon(":/icons/config")));
   mConfigAction->setToolTip(tr("Config"));
   mActionGroup->addAction(mConfigAction);

   GitQlientSettings settings(mGit->getGitDir());
   mBuildSystemAction = new QAction(this);
   mBuildSystemAction->setText("Jenkins");
   mBuildSystemAction->setVisible(settings.localValue("BuildSystemEnabled", false).toBool());
   mBuildSystemAction->setCheckable(true);
   mBuildSystemAction->setIcon(QIcon::fromTheme("run-build", QIcon(":/icons/build_system")));
   mBuildSystemAction->setToolTip("Jenkins");
   mActionGroup->addAction(mBuildSystemAction);
   configBuildSystemButton();

   auto toolBar = new QToolBar(this);
   toolBar->setToolButtonStyle(Qt::ToolButtonFollowStyle);
   toolBar->addAction(mHistoryAction);
   toolBar->addAction(mDiffAction);
   toolBar->addAction(mBlameAction);
   toolBar->addSeparator();
   toolBar->addAction(mPullAction);
   toolBar->addAction(mPushAction);
   toolBar->addAction(mRefreshAction);
   toolBar->addSeparator();
   if (mGitPlatformAction)
   {
      toolBar->addAction(mGitPlatformAction);
   }
   toolBar->addAction(mBuildSystemAction);
   toolBar->addAction(mConfigAction);

   const auto hLayout = new QHBoxLayout();
   hLayout->setContentsMargins(QMargins());
   hLayout->addStretch();
   hLayout->setSpacing(5);
   hLayout->addWidget(toolBar);
   hLayout->addStretch();

   mMergeWarning->setObjectName("WarningButton");
   mMergeWarning->setVisible(false);
   mBtnGroup->addButton(mMergeWarning, static_cast<int>(ControlsMainViews::Merge));

   const auto vLayout = new QVBoxLayout(this);
   vLayout->setContentsMargins(0, 5, 0, 0);
   vLayout->setSpacing(10);
   vLayout->addLayout(hLayout);
   vLayout->addWidget(mMergeWarning);

   connect(mHistoryAction, &QAction::triggered, this, &Controls::signalGoRepo);
   connect(mDiffAction, &QAction::triggered, this, &Controls::signalGoDiff);
   connect(mBlameAction, &QAction::triggered, this, &Controls::signalGoBlame);
   connect(mTerminalAction, &QAction::triggered, this, &Controls::goTerminal);
   connect(mPullAction, &QAction::triggered, this, &Controls::pullCurrentBranch);
   connect(mPushAction, &QAction::triggered, this, &Controls::pushCurrentBranch);
   connect(mRefreshAction, &QAction::triggered, this, &Controls::requestFullReload);
   connect(mMergeWarning, &QPushButton::clicked, this, &Controls::signalGoMerge);
   connect(mConfigAction, &QAction::triggered, this, &Controls::goConfig);
   connect(mBuildSystemAction, &QAction::triggered, this, &Controls::signalGoBuildSystem);

   enableButtons(false);
}

Controls::~Controls()
{
   delete mBtnGroup;
}

void Controls::toggleButton(ControlsMainViews view)
{
   QAction *action = nullptr;

   switch (view)
   {
      case ControlsMainViews::History:
         action = mHistoryAction;
         break;
      case ControlsMainViews::Diff:
         action = mDiffAction;
         break;
      case ControlsMainViews::Blame:
         action = mBlameAction;
         break;
      case ControlsMainViews::Terminal:
         action = mTerminalAction;
         break;
      case ControlsMainViews::Merge:
         break;
      case ControlsMainViews::GitServer:
         action = mGitPlatformAction;
         break;
      case ControlsMainViews::BuildSystem:
         action = mBuildSystemAction;
         break;
   }

   if (action)
   {
      action->setChecked(true);
   }
}

void Controls::enableButtons(bool enabled)
{
   mHistoryAction->setEnabled(enabled);
   mBlameAction->setEnabled(enabled);
   mPullAction->setEnabled(enabled);
   mPushAction->setEnabled(enabled);
   mRefreshAction->setEnabled(enabled);
   if (mGitPlatformAction)
   {
      mGitPlatformAction->setEnabled(enabled);
   }
   mConfigAction->setEnabled(enabled);

   if (enabled)
   {
      GitQlientSettings settings(mGit->getGitDir());

      mBuildSystemAction->setEnabled(settings.localValue("BuildSystemEnabled", false).toBool());
      mGitPlatformAction->setEnabled(settings.localValue("GitServerEnabled", false).toBool());
   }
   else
   {
      mBuildSystemAction->setEnabled(false);
      mGitPlatformAction->setEnabled(false);
   }
}

void Controls::pullCurrentBranch()
{
   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
   GitRemote gitRemote(mGit);
   const auto ret = gitRemote.pull();
   QApplication::restoreOverrideCursor();

   if (ret.success)
   {
      if (ret.output.contains("merge conflict", Qt::CaseInsensitive))
         emit signalPullConflict();
      else
         emit requestFullReload();
   }
   else
   {
      if (ret.output.contains("error: could not apply", Qt::CaseInsensitive)
          && ret.output.contains("causing a conflict", Qt::CaseInsensitive))
      {
         emit signalPullConflict();
      }
      else
      {
         QMessageBox msgBox(QMessageBox::Critical, tr("Error while pulling"),
                            QString(tr("There were problems during the pull operation. Please, see the detailed "
                                       "description for more information.")),
                            QMessageBox::Ok, this);
         msgBox.setDetailedText(ret.output);
         msgBox.setStyleSheet(GitQlientStyles::getStyles());
         msgBox.exec();
      }
   }
}

void Controls::fetchAll()
{
   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
   GitQlientSettings settings(mGit->getGitDir());
   GitRemote gitRemote(mGit);
   const auto ret = gitRemote.fetch(settings.localValue("PruneOnFetch").toBool());
   QApplication::restoreOverrideCursor();

   if (ret)
      emit requestFullReload();
}

void Controls::activateMergeWarning()
{
   mMergeWarning->setVisible(true);
}

void Controls::disableMergeWarning()
{
   mMergeWarning->setVisible(false);
}

void Controls::disableDiff()
{
   mDiffAction->setDisabled(true);
}

void Controls::enableDiff()
{
   mDiffAction->setEnabled(true);
}

ControlsMainViews Controls::getCurrentSelectedButton() const
{
   return mBlameAction->isChecked() ? ControlsMainViews::Blame : ControlsMainViews::History;
}

void Controls::showJenkinsButton(bool show)
{
   mBuildSystemAction->setVisible(show);
   // mPluginsSeparator->setVisible(show || mGitPlatformAction->isVisible());
}

void Controls::enableJenkins(bool enable)
{
   mBuildSystemAction->setEnabled(enable);
}

void Controls::showGitServerButton(bool show)
{
   mGitPlatformAction->setVisible(show);
   // mPluginsSeparator->setVisible(mBuildSystem->isVisible() || show);
}

void Controls::enableGitServer(bool enabled)
{
   mGitPlatformAction->setEnabled(enabled);
}

void Controls::enableTerminal()
{
   mTerminalAction->setVisible(true);
}

void Controls::pushCurrentBranch()
{
   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
   GitRemote gitRemote(mGit);
   const auto ret = gitRemote.push();
   QApplication::restoreOverrideCursor();

   if (ret.output.contains("has no upstream branch"))
   {
      const auto currentBranch = mGit->getCurrentBranch();
      BranchDlg dlg({ currentBranch, BranchDlgMode::PUSH_UPSTREAM, mCache, mGit });
      const auto dlgRet = dlg.exec();

      if (dlgRet == QDialog::Accepted)
         emit signalRefreshPRsCache();
   }
   else if (ret.success)
   {
      const auto currentBranch = mGit->getCurrentBranch();
      GitConfig gitConfig(mGit);
      const auto remote = gitConfig.getRemoteForBranch(currentBranch);

      if (remote.success)
      {
         const auto oldSha = mCache->getShaOfReference(QString("%1/%2").arg(remote.output, currentBranch),
                                                       References::Type::RemoteBranches);
         const auto sha = mCache->getShaOfReference(currentBranch, References::Type::LocalBranch);
         mCache->deleteReference(oldSha, References::Type::RemoteBranches,
                                 QString("%1/%2").arg(remote.output, currentBranch));
         mCache->insertReference(sha, References::Type::RemoteBranches,
                                 QString("%1/%2").arg(remote.output, currentBranch));
         emit mCache->signalCacheUpdated();
         emit signalRefreshPRsCache();
      }
   }
   else
   {
      QMessageBox msgBox(
          QMessageBox::Critical, tr("Error while pushing"),
          QString(tr("There were problems during the push operation. Please, see the detailed description "
                     "for more information.")),
          QMessageBox::Ok, this);
      msgBox.setDetailedText(ret.output);
      msgBox.setStyleSheet(GitQlientStyles::getStyles());
      msgBox.exec();
   }
}

void Controls::pruneBranches()
{
   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
   GitRemote gitRemote(mGit);
   const auto ret = gitRemote.prune();
   QApplication::restoreOverrideCursor();

   if (ret.success)
      emit requestReferencesReload();
}

QAction *Controls::createGitPlatformAction()
{

   GitConfig gitConfig(mGit);
   const auto remoteUrl = gitConfig.getServerHost();
   QIcon gitPlatformIcon;
   QString name;
   QString prName;
   auto add = false;

   if (remoteUrl.contains("github", Qt::CaseInsensitive))
   {
      add = true;

      gitPlatformIcon = QIcon(":/icons/github");
      name = "GitHub";
      prName = tr("Pull Request");
   }
   else if (remoteUrl.contains("gitlab", Qt::CaseInsensitive))
   {
      add = true;

      gitPlatformIcon = QIcon(":/icons/gitlab");
      name = "GitLab";
      prName = tr("Merge Request");
   }

   if (add)
   {
      QAction *action = new QAction(this);
      action->setText(name);
      action->setToolTip(name);
      action->setCheckable(true);
      action->setIcon(QIcon(gitPlatformIcon));
      connect(action, &QAction::triggered, this, &Controls::signalGoServer);
      return action;
   }
   else
   {
      return nullptr;
   }
}

void Controls::configBuildSystemButton()
{
   GitQlientSettings settings(mGit->getGitDir());
   const auto isConfigured = settings.localValue("BuildSystemEnabled", false).toBool();
   mBuildSystemAction->setEnabled(isConfigured);

   if (!isConfigured)
      emit signalGoRepo();
}
