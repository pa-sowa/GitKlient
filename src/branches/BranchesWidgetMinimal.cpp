#include "BranchesWidgetMinimal.h"

#include <GitBase.h>
#include <GitCache.h>
#include <GitStashes.h>
#include <GitSubmodules.h>

#include <QEvent>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

BranchesWidgetMinimal::BranchesWidgetMinimal(const QSharedPointer<GitCache> &cache, const QSharedPointer<GitBase> git,
                                             QWidget *parent)
   : QFrame(parent)
   , mGit(git)
   , mCache(cache)
   , mBack(new QPushButton())
   , mLocal(new QToolButton())
   , mLocalMenu(new QMenu(mLocal))
   , mRemote(new QToolButton())
   , mRemoteMenu(new QMenu(mRemote))
   , mTags(new QToolButton())
   , mTagsMenu(new QMenu(mTags))
   , mStashes(new QToolButton())
   , mStashesMenu(new QMenu(mStashes))
   , mSubmodules(new QToolButton())
   , mSubmodulesMenu(new QMenu(mSubmodules))
{
   mBack->setIcon(QIcon::fromTheme("sidebar-expand-right", QIcon(":/icons/back")));
   mBack->setToolTip(tr("Full view"));
   connect(mBack, &QPushButton::clicked, this, &BranchesWidgetMinimal::showFullBranchesView);

   const auto layout = new QVBoxLayout(this);
   layout->setContentsMargins(QMargins());
   layout->setSpacing(0);
   layout->addWidget(mBack);
   layout->addWidget(mLocal);
   layout->addWidget(mRemote);
   layout->addWidget(mTags);
   layout->addWidget(mStashes);
   layout->addWidget(mSubmodules);

   setupToolButton(mLocal, mLocalMenu, QIcon::fromTheme("vcs-branch", QIcon(":/icons/local")), tr("Local branches"));
   setupToolButton(mRemote, mRemoteMenu, QIcon::fromTheme("folder-cloud", QIcon(":/icons/server")),
                   tr("Remote branches"));
   setupToolButton(mTags, mTagsMenu, QIcon::fromTheme("tags", QIcon(":/icons/tags")), tr("Tags"));
   setupToolButton(mStashes, mStashesMenu, QIcon(":/icons/stashes"), tr("Stashes"));
   setupToolButton(mSubmodules, mSubmodulesMenu, QIcon(":/icons/submodules"), tr("Submodules"));
}

bool BranchesWidgetMinimal::eventFilter(QObject *obj, QEvent *event)
{

   if (const auto menu = qobject_cast<QMenu *>(obj); menu && event->type() == QEvent::Show)
   {
      auto localPos = menu->parentWidget()->pos();
      localPos.setX(localPos.x());
      auto pos = mapToGlobal(localPos);
      menu->show();
      pos.setX(pos.x() - menu->width());
      menu->move(pos);
      return true;
   }

   return false;
}

void BranchesWidgetMinimal::addActionToMenu(const QString &sha, const QString &name, QMenu *menu)
{
   const auto action = new QAction(name, menu);

   if (mGit->getCurrentBranch() == name)
   {
      auto font = action->font();
      font.setBold(true);
      action->setFont(font);
   }

   action->setData(sha);
   connect(action, &QAction::triggered, this, [this, sha] { emit commitSelected(sha); });

   menu->addAction(action);
}

void BranchesWidgetMinimal::setupToolButton(QToolButton *button, QMenu *menu, const QIcon &icon, const QString &tooltip)
{
   menu->installEventFilter(this);
   button->setMenu(menu);
   button->setIcon(icon);
   button->setPopupMode(QToolButton::InstantPopup);
   button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
   button->setText(QString::number(menu->actions().count()));
   button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
   button->setToolTip(tooltip);
   // Hide the menu drop down arrow
   button->setStyleSheet("QToolButton::menu-indicator { image: none; }");
}

void BranchesWidgetMinimal::configureLocalMenu(const QString &sha, const QString &branch)
{
   addActionToMenu(sha, branch, mLocalMenu);
   mLocal->setText(QString::number(mLocalMenu->actions().count()));
}

void BranchesWidgetMinimal::configureRemoteMenu(const QString &sha, const QString &branch)
{
   addActionToMenu(sha, branch, mRemoteMenu);
   mRemote->setText(QString::number(mRemoteMenu->actions().count()));
}

void BranchesWidgetMinimal::configureTagsMenu(const QString &sha, const QString &tag)
{
   addActionToMenu(sha, tag, mTagsMenu);
   mTags->setText(QString::number(mTagsMenu->actions().count()));
}

void BranchesWidgetMinimal::configureStashesMenu(const QString &stashId, const QString &name)
{
   const auto action = new QAction(name);
   action->setData(stashId);
   connect(action, &QAction::triggered, this, [this, stashId] { emit stashSelected(stashId); });

   mStashesMenu->addAction(action);
   mStashes->setText(QString::number(mStashesMenu->actions().count()));
}

void BranchesWidgetMinimal::configureSubmodulesMenu(const QString &name)
{
   const auto action = new QAction(name);
   action->setData(name);
   mSubmodulesMenu->addAction(action);
   mSubmodules->setText(QString::number(mSubmodulesMenu->actions().count()));
}

void BranchesWidgetMinimal::clearActions()
{
   mLocalMenu->clear();
   mRemoteMenu->clear();
   mTagsMenu->clear();
   mStashesMenu->clear();
   mSubmodulesMenu->clear();
}
