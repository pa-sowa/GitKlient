#include "Controls.h"

#include <BranchDlg.h>
#include <GitBase.h>
#include <GitCache.h>
#include <GitConfig.h>
#include <GitQlientSettings.h>
#include <GitQlientStyles.h>
#include <GitQlientUpdater.h>
#include <GitRemote.h>
#include <GitStashes.h>
#include <GitTags.h>
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
   , mGit(git)
   , mGitTags(new GitTags(mGit, cache))
   , mHistory(new QToolButton())
   , mDiff(new QToolButton())
   , mBlame(new QToolButton())
   , mPullBtn(new QToolButton())
   , mPullOptions(new QToolButton())
   , mPushBtn(new QToolButton())
   , mRefreshBtn(new QToolButton())
   , mConfigBtn(new QToolButton())
   , mGitPlatform(new QToolButton())
   , mBuildSystem(new QToolButton())
   , mVersionCheck(new QToolButton())
   , mMergeWarning(new QPushButton(tr("WARNING: There is a merge pending to be committed! Click here to solve it.")))
   , mUpdater(new GitQlientUpdater(this))
   , mBtnGroup(new QButtonGroup())
{
   setAttribute(Qt::WA_DeleteOnClose);

   connect(mUpdater, &GitQlientUpdater::newVersionAvailable, this, [this]() { mVersionCheck->setVisible(true); });

   mHistory->setText(tr("History"));
   mHistory->setCheckable(true);
   mHistory->setIcon(QIcon::fromTheme("vcs-commit", QIcon(":/icons/git_orange")));
   mHistory->setIconSize(QSize(22, 22));
   mHistory->setToolTip(tr("View"));
   mHistory->setToolButtonStyle(Qt::ToolButtonFollowStyle);
   mBtnGroup->addButton(mHistory, static_cast<int>(ControlsMainViews::History));

   mDiff->setText(tr("Diff"));
   mDiff->setCheckable(true);
   mDiff->setIcon(QIcon::fromTheme("vcs-diff", QIcon(":/icons/diff")));
   mDiff->setIconSize(QSize(22, 22));
   mDiff->setToolTip(tr("Diff"));
   mDiff->setToolButtonStyle(Qt::ToolButtonFollowStyle);
   mDiff->setEnabled(false);
   mBtnGroup->addButton(mDiff, static_cast<int>(ControlsMainViews::Diff));

   mBlame->setText(tr("Blame"));
   mBlame->setCheckable(true);
   mBlame->setIcon(QIcon::fromTheme("actor", QIcon(":/icons/blame")));
   mBlame->setIconSize(QSize(22, 22));
   mBlame->setToolTip(tr("Blame"));
   mBlame->setToolButtonStyle(Qt::ToolButtonFollowStyle);
   mBtnGroup->addButton(mBlame, static_cast<int>(ControlsMainViews::Blame));

   const auto menu = new QMenu(mPullOptions);
   menu->installEventFilter(this);

   auto action = menu->addAction(tr("Fetch all"));
   connect(action, &QAction::triggered, this, &Controls::fetchAll);

   action = menu->addAction(tr("Prune"));
   connect(action, &QAction::triggered, this, &Controls::pruneBranches);
   menu->addSeparator();

   mPullBtn->setIconSize(QSize(22, 22));
   mPullBtn->setText(tr("Pull"));
   mPullBtn->setToolTip(tr("Pull"));
   mPullBtn->setToolButtonStyle(Qt::ToolButtonFollowStyle);
   mPullBtn->setPopupMode(QToolButton::InstantPopup);
   mPullBtn->setIcon(QIcon::fromTheme("vcs-pull", QIcon(":/icons/git_pull")));
   mPullBtn->setObjectName("ToolButtonAboveMenu");
   mPullBtn->setMenu(menu);

   mPushBtn->setText(tr("Push"));
   mPushBtn->setIcon(QIcon::fromTheme("vcs-push", QIcon(":/icons/git_push")));
   mPushBtn->setIconSize(QSize(22, 22));
   mPushBtn->setToolTip(tr("Push"));
   mPushBtn->setToolButtonStyle(Qt::ToolButtonFollowStyle);

   mRefreshBtn->setText(tr("Refresh"));
   mRefreshBtn->setIcon(QIcon::fromTheme("view-refresh", QIcon(":/icons/refresh")));
   mRefreshBtn->setIconSize(QSize(22, 22));
   mRefreshBtn->setToolTip(tr("Refresh"));
   mRefreshBtn->setToolButtonStyle(Qt::ToolButtonFollowStyle);

   mConfigBtn->setText(tr("Settings"));
   mConfigBtn->setCheckable(true);
   mConfigBtn->setIcon(QIcon::fromTheme("configure", QIcon(":/icons/config")));
   mConfigBtn->setIconSize(QSize(22, 22));
   mConfigBtn->setToolTip(tr("Config"));
   mConfigBtn->setToolButtonStyle(Qt::ToolButtonFollowStyle);
   mBtnGroup->addButton(mConfigBtn, static_cast<int>(ControlsMainViews::Config));

   const auto separator = new QFrame();
   separator->setObjectName("orangeSeparator");
   separator->setFixedHeight(20);

   const auto separator2 = new QFrame();
   separator2->setObjectName("orangeSeparator");
   separator2->setFixedHeight(20);

   const auto hLayout = new QHBoxLayout();
   hLayout->setContentsMargins(QMargins());
   hLayout->addStretch();
   hLayout->setSpacing(5);
   hLayout->addWidget(mHistory);
   hLayout->addWidget(mDiff);
   hLayout->addWidget(mBlame);
   hLayout->addWidget(separator);
   hLayout->addWidget(mPushBtn);
   hLayout->addWidget(separator2);

   createGitPlatformButton(hLayout);

   GitQlientSettings settings(mGit->getGitDir());
   mBuildSystem->setVisible(settings.localValue("BuildSystemEnabled", false).toBool());
   mBuildSystem->setCheckable(true);
   mBuildSystem->setIcon(QIcon::fromTheme("run-build", QIcon(":/icons/build_system")));
   mBuildSystem->setIconSize(QSize(22, 22));
   mBuildSystem->setToolTip("Jenkins");
   mBuildSystem->setToolButtonStyle(Qt::ToolButtonFollowStyle);
   mBuildSystem->setPopupMode(QToolButton::InstantPopup);
   mBtnGroup->addButton(mBuildSystem, static_cast<int>(ControlsMainViews::BuildSystem));

   connect(mBuildSystem, &QToolButton::clicked, this, &Controls::signalGoBuildSystem);

   hLayout->addWidget(mBuildSystem);

   configBuildSystemButton();

   const auto separator3 = new QFrame();
   separator3->setObjectName("orangeSeparator");
   separator3->setFixedHeight(20);
   hLayout->addWidget(separator3);

   mVersionCheck->setIcon(QIcon::fromTheme("system-upgrade", QIcon(":/icons/get_gitqlient")));
   mVersionCheck->setIconSize(QSize(22, 22));
   mVersionCheck->setText(tr("New version"));
   mVersionCheck->setObjectName("longToolButton");
   mVersionCheck->setToolButtonStyle(Qt::ToolButtonIconOnly);
   mVersionCheck->setVisible(false);

   mUpdater->checkNewGitQlientVersion();

   hLayout->addWidget(mRefreshBtn);
   hLayout->addWidget(mConfigBtn);
   hLayout->addWidget(mVersionCheck);
   hLayout->addStretch();

   mMergeWarning->setObjectName("WarningButton");
   mMergeWarning->setVisible(false);
   mBtnGroup->addButton(mMergeWarning, static_cast<int>(ControlsMainViews::Merge));

   const auto vLayout = new QVBoxLayout(this);
   vLayout->setContentsMargins(0, 5, 0, 0);
   vLayout->setSpacing(10);
   vLayout->addLayout(hLayout);
   vLayout->addWidget(mMergeWarning);

   connect(mHistory, &QToolButton::clicked, this, &Controls::signalGoRepo);
   connect(mDiff, &QToolButton::clicked, this, &Controls::signalGoDiff);
   connect(mBlame, &QToolButton::clicked, this, &Controls::signalGoBlame);
   connect(mPullBtn, &QToolButton::clicked, this, &Controls::pullCurrentBranch);
   connect(mPushBtn, &QToolButton::clicked, this, &Controls::pushCurrentBranch);
   connect(mRefreshBtn, &QToolButton::clicked, this, [this]() { emit requestReload(true); });
   connect(mMergeWarning, &QPushButton::clicked, this, &Controls::signalGoMerge);
   connect(mVersionCheck, &QToolButton::clicked, mUpdater, &GitQlientUpdater::showInfoMessage);
   connect(mConfigBtn, &QToolButton::clicked, this, &Controls::goConfig);

   enableButtons(false);
}

Controls::~Controls()
{
   delete mBtnGroup;
}

void Controls::toggleButton(ControlsMainViews view)
{
   mBtnGroup->button(static_cast<int>(view))->setChecked(true);
}

void Controls::enableButtons(bool enabled)
{
   mHistory->setEnabled(enabled);
   mBlame->setEnabled(enabled);
   mPullBtn->setEnabled(enabled);
   mPullOptions->setEnabled(enabled);
   mPushBtn->setEnabled(enabled);
   mRefreshBtn->setEnabled(enabled);
   mGitPlatform->setEnabled(enabled);
   mConfigBtn->setEnabled(enabled);

   if (enabled)
   {
      GitQlientSettings settings(mGit->getGitDir());
      const auto isConfigured = settings.localValue("BuildSystemEanbled", false).toBool();

      mBuildSystem->setEnabled(isConfigured);
   }
   else
      mBuildSystem->setEnabled(false);
}

void Controls::pullCurrentBranch()
{
   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
   QScopedPointer<GitRemote> git(new GitRemote(mGit));
   const auto ret = git->pull();
   QApplication::restoreOverrideCursor();

   const auto msg = ret.output.toString();

   if (ret.success)
   {
      if (msg.contains("merge conflict", Qt::CaseInsensitive))
         emit signalPullConflict();
      else
         emit requestReload(true);
   }
   else
   {
      if (msg.contains("error: could not apply", Qt::CaseInsensitive)
          && msg.contains("causing a conflict", Qt::CaseInsensitive))
      {
         emit signalPullConflict();
      }
      else
      {
         QMessageBox msgBox(QMessageBox::Critical, tr("Error while pulling"),
                            QString(tr("There were problems during the pull operation. Please, see the detailed "
                                       "description for more information.")),
                            QMessageBox::Ok, this);
         msgBox.setDetailedText(msg);
         msgBox.setStyleSheet(GitQlientStyles::getStyles());
         msgBox.exec();
      }
   }
}

void Controls::fetchAll()
{
   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
   QScopedPointer<GitRemote> git(new GitRemote(mGit));
   const auto ret = git->fetch();
   QApplication::restoreOverrideCursor();

   if (ret)
   {
      mGitTags->getRemoteTags();
      emit requestReload(true);
   }
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
   mDiff->setDisabled(true);
}

void Controls::enableDiff()
{
   mDiff->setEnabled(true);
}

ControlsMainViews Controls::getCurrentSelectedButton() const
{
   return mBlame->isChecked() ? ControlsMainViews::Blame : ControlsMainViews::History;
}

void Controls::pushCurrentBranch()
{
   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
   QScopedPointer<GitRemote> git(new GitRemote(mGit));
   const auto ret = git->push();
   QApplication::restoreOverrideCursor();

   if (ret.output.toString().contains("has no upstream branch"))
   {
      const auto currentBranch = mGit->getCurrentBranch();
      BranchDlg dlg({ currentBranch, BranchDlgMode::PUSH_UPSTREAM, mGit });
      const auto dlgRet = dlg.exec();

      if (dlgRet == QDialog::Accepted)
      {
         emit signalRefreshPRsCache();
         emit requestReload(true);
      }
   }
   else if (ret.success)
   {
      emit signalRefreshPRsCache();
      emit requestReload(true);
   }
   else
   {
      QMessageBox msgBox(
          QMessageBox::Critical, tr("Error while pushing"),
          QString(tr("There were problems during the push operation. Please, see the detailed description "
                     "for more information.")),
          QMessageBox::Ok, this);
      msgBox.setDetailedText(ret.output.toString());
      msgBox.setStyleSheet(GitQlientStyles::getStyles());
      msgBox.exec();
   }
}

void Controls::stashCurrentWork()
{
   QScopedPointer<GitStashes> git(new GitStashes(mGit));
   const auto ret = git->stash();

   if (ret.success)
      emit requestReload(false);
}

void Controls::popStashedWork()
{
   QScopedPointer<GitStashes> git(new GitStashes(mGit));
   const auto ret = git->pop();

   if (ret.success)
      emit requestReload(false);
   else
   {
      QMessageBox msgBox(QMessageBox::Critical, tr("Error while popping a stash"),
                         tr("There were problems during the stash pop operation. Please, see the detailed "
                            "description for more information."),
                         QMessageBox::Ok, this);
      msgBox.setDetailedText(ret.output.toString());
      msgBox.setStyleSheet(GitQlientStyles::getStyles());
      msgBox.exec();
   }
}

void Controls::pruneBranches()
{
   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
   QScopedPointer<GitRemote> git(new GitRemote(mGit));
   const auto ret = git->prune();
   QApplication::restoreOverrideCursor();

   if (ret.success)
      emit requestReload(true);
}

void Controls::createGitPlatformButton(QHBoxLayout *layout)
{
   QScopedPointer<GitConfig> gitConfig(new GitConfig(mGit));
   const auto remoteUrl = gitConfig->getServerUrl();
   QString iconPath;
   QString name;
   QString prName;
   auto add = false;

   if (remoteUrl.contains("github", Qt::CaseInsensitive))
   {
      add = true;

      iconPath = ":/icons/github";
      name = "GitHub";
      prName = tr("Pull Request");
   }
   else if (remoteUrl.contains("gitlab", Qt::CaseInsensitive))
   {
      add = true;

      iconPath = ":/icons/gitlab";
      name = "GitLab";
      prName = tr("Merge Request");
   }

   if (add)
   {
      mGitPlatform->setText(name);
      mGitPlatform->setToolTip(name);
      mGitPlatform->setCheckable(true);
      mGitPlatform->setIcon(QIcon(iconPath));
      mGitPlatform->setIconSize(QSize(22, 22));
      mGitPlatform->setToolButtonStyle(Qt::ToolButtonFollowStyle);
      mGitPlatform->setPopupMode(QToolButton::InstantPopup);
      mBtnGroup->addButton(mGitPlatform, static_cast<int>(ControlsMainViews::GitServer));

      layout->addWidget(mGitPlatform);

      connect(mGitPlatform, &QToolButton::clicked, this, &Controls::signalGoServer);
   }
   else
      mGitPlatform->setVisible(false);
}

void Controls::configBuildSystemButton()
{
   GitQlientSettings settings(mGit->getGitDir());
   const auto isConfigured = settings.localValue("BuildSystemEanbled", false).toBool();
   mBuildSystem->setEnabled(isConfigured);

   if (!isConfigured)
      emit signalGoRepo();
}

bool Controls::eventFilter(QObject *obj, QEvent *event)
{
   if (const auto menu = qobject_cast<QMenu *>(obj); menu && event->type() == QEvent::Show)
   {
      auto localPos = menu->parentWidget()->pos();
      localPos.setX(localPos.x());
      auto pos = mapToGlobal(localPos);
      menu->show();
      pos.setY(pos.y() + menu->parentWidget()->height());
      menu->move(pos);
      return true;
   }

   return false;
}
