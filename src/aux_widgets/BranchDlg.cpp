#include "BranchDlg.h"
#include "ui_BranchDlg.h"

#include <GitBase.h>
#include <GitBranches.h>
#include <GitCache.h>
#include <GitConfig.h>
#include <GitQlientStyles.h>
#include <GitStashes.h>

#include <QFile>
#include <QMessageBox>

#include <utility>

BranchDlg::BranchDlg(BranchDlgConfig config, QWidget *parent)
   : QDialog(parent)
   , ui(new Ui::BranchDlg)
   , mConfig(std::move(config))
{
   setStyleSheet(GitQlientStyles::getStyles());

   ui->setupUi(this);
   ui->leOldName->setText(mConfig.mCurrentBranchName);

   ui->chbCopyRemote->setHidden(true);

   switch (mConfig.mDialogMode)
   {
      case BranchDlgMode::CREATE:
         setWindowTitle(tr("Create branch"));
         break;
      case BranchDlgMode::RENAME:
         ui->pbAccept->setText(tr("Rename"));
         setWindowTitle(tr("Rename branch"));
         break;
      case BranchDlgMode::CREATE_CHECKOUT:
         setWindowTitle(tr("Create and checkout branch"));
         ui->currentBranchFrame->setHidden(true);
         break;
      case BranchDlgMode::CREATE_FROM_COMMIT:
         setWindowTitle(tr("Create branch at commit"));
         ui->currentBranchFrame->setHidden(true);
         break;
      case BranchDlgMode::CREATE_CHECKOUT_FROM_COMMIT:
         setWindowTitle(tr("Create and checkout branch"));
         ui->currentBranchFrame->setHidden(true);
         break;
      case BranchDlgMode::STASH_BRANCH:
         setWindowTitle(tr("Stash branch"));
         break;
      case BranchDlgMode::PUSH_UPSTREAM:
         connect(ui->chbCopyRemote, &QCheckBox::stateChanged, this, &BranchDlg::copyBranchName);
         ui->chbCopyRemote->setVisible(true);
         ui->chbCopyRemote->setChecked(true);
         setWindowTitle(tr("Push upstream branch"));
         ui->pbAccept->setText(tr("Push"));
         break;
      default:
         break;
   }

   connect(ui->leNewName, &QLineEdit::editingFinished, this, &BranchDlg::checkNewBranchName);
   connect(ui->leNewName, &QLineEdit::returnPressed, this, &BranchDlg::accept);
   connect(ui->pbAccept, &QPushButton::clicked, this, &BranchDlg::accept);
   connect(ui->pbCancel, &QPushButton::clicked, this, &BranchDlg::reject);
}

BranchDlg::~BranchDlg()
{
   delete ui;
}

void BranchDlg::checkNewBranchName()
{
   if (ui->leNewName->text() == ui->leOldName->text() && mConfig.mDialogMode != BranchDlgMode::PUSH_UPSTREAM)
      ui->leNewName->setStyleSheet("border: 1px solid red;");
}

void BranchDlg::accept()
{
   if (ui->leNewName->text() == ui->leOldName->text() && mConfig.mDialogMode != BranchDlgMode::PUSH_UPSTREAM)
      ui->leNewName->setStyleSheet("border: 1px solid red;");
   else
   {
      QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

      GitBranches branches(mConfig.mGit);
      GitExecResult ret;

      if (mConfig.mDialogMode == BranchDlgMode::CREATE)
      {
         ret = branches.createBranchFromAnotherBranch(ui->leOldName->text(), ui->leNewName->text());

         if (ret.success)
         {
            auto type = References::Type::LocalBranch;
            auto sha = mConfig.mCache->getShaOfReference(ui->leOldName->text(), type);

            if (sha.isEmpty())
            {
               type = References::Type::RemoteBranches;
               sha = mConfig.mCache->getShaOfReference(ui->leOldName->text(), type);
            }

            if (!sha.isEmpty())
            {
               mConfig.mCache->insertReference(sha, type, ui->leNewName->text());
               emit mConfig.mCache->signalCacheUpdated();
            }
         }
      }
      else if (mConfig.mDialogMode == BranchDlgMode::CREATE_CHECKOUT)
      {
         ret = branches.checkoutNewLocalBranchFromAnotherBranch(ui->leOldName->text(), ui->leNewName->text());

         if (ret.success)
         {
            mConfig.mCache->insertReference(mConfig.mGit->getLastCommit().output.trimmed(),
                                            References::Type::LocalBranch, ui->leNewName->text());
            emit mConfig.mCache->signalCacheUpdated();
         }
      }
      else if (mConfig.mDialogMode == BranchDlgMode::RENAME)
      {
         ret = branches.renameBranch(ui->leOldName->text(), ui->leNewName->text());

         if (ret.success)
         {
            const auto type = References::Type::LocalBranch;
            const auto sha = mConfig.mCache->getShaOfReference(ui->leOldName->text(), type);

            mConfig.mCache->deleteReference(sha, type, ui->leOldName->text());
            mConfig.mCache->insertReference(sha, type, ui->leNewName->text());
            emit mConfig.mCache->signalCacheUpdated();
         }
      }
      else if (mConfig.mDialogMode == BranchDlgMode::CREATE_FROM_COMMIT)
      {
         ret = branches.createBranchAtCommit(ui->leOldName->text(), ui->leNewName->text());

         if (ret.success)
         {
            mConfig.mCache->insertReference(ui->leOldName->text(), References::Type::LocalBranch,
                                            ui->leNewName->text());
            emit mConfig.mCache->signalCacheUpdated();
         }
      }
      else if (mConfig.mDialogMode == BranchDlgMode::CREATE_CHECKOUT_FROM_COMMIT)
      {
         ret = branches.checkoutBranchFromCommit(ui->leOldName->text(), ui->leNewName->text());

         if (ret.success)
         {
            mConfig.mCache->insertReference(ui->leOldName->text(), References::Type::LocalBranch,
                                            ui->leNewName->text());
            emit mConfig.mCache->signalCacheUpdated();
         }
      }
      else if (mConfig.mDialogMode == BranchDlgMode::STASH_BRANCH)
      {
         GitStashes stashes(mConfig.mGit);
         ret = stashes.stashBranch(ui->leOldName->text(), ui->leNewName->text());
      }
      else if (mConfig.mDialogMode == BranchDlgMode::PUSH_UPSTREAM)
      {
         ret = branches.pushUpstream(ui->leNewName->text());

         if (ret.success)
         {
            GitConfig config(mConfig.mGit);
            const auto remote = config.getRemoteForBranch(ui->leNewName->text());

            if (remote.success)
            {
               const auto sha = mConfig.mCache->getShaOfReference(ui->leOldName->text(), References::Type::LocalBranch);
               mConfig.mCache->insertReference(sha, References::Type::RemoteBranches,
                                               QString("%1/%2").arg(remote.output, ui->leNewName->text()));
               emit mConfig.mCache->signalCacheUpdated();
            }
         }
      }

      QApplication::restoreOverrideCursor();

      if (!ret.success)
      {
         QDialog::reject();

         QMessageBox msgBox(
             QMessageBox::Critical, tr("Error on branch action!"),
             QString(tr("There were problems during the branch operation. Please, see the detailed description "
                        "for more information.")),
             QMessageBox::Ok, this);
         msgBox.setDetailedText(ret.output);
         msgBox.setStyleSheet(GitQlientStyles::getStyles());
         msgBox.exec();
      }
      else
         QDialog::accept();
   }
}

void BranchDlg::copyBranchName()
{
   const auto checked = ui->chbCopyRemote->isChecked();
   ui->leNewName->setVisible(!checked);
   ui->leNewName->setText(checked ? ui->leOldName->text() : "");
}
