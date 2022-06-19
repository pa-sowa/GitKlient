#include "FileDiffWidget.h"

#include <CommitInfo.h>
#include <DiffHelper.h>
#include <FileDiffView.h>
#include <FileEditor.h>
#include <GitBase.h>
#include <GitCache.h>
#include <GitHistory.h>
#include <GitLocal.h>
#include <GitPatches.h>
#include <GitQlientSettings.h>
#include <LineNumberArea.h>

#include <QDateTime>
#include <QDir>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollBar>
#include <QStackedWidget>
#include <QTemporaryFile>

FileDiffWidget::FileDiffWidget(const QSharedPointer<GitBase> &git, QSharedPointer<GitCache> cache, QWidget *parent)
   : IDiffWidget(git, cache, parent)
   , mGoPrevious(new QPushButton())
   , mGoNext(new QPushButton())
   , mFullView(new QPushButton())
   , mSplitView(new QPushButton())
   , mNewFile(new FileDiffView())
   , mSearchOld(new QLineEdit())
   , mOldFile(new FileDiffView())
{
   mNewFile->addNumberArea(new LineNumberArea(mNewFile));
   mOldFile->addNumberArea(new LineNumberArea(mOldFile));

   mNewFile->setObjectName("newFile");
   mOldFile->setObjectName("oldFile");

   GitQlientSettings settings(mGit->getGitDir());
   QFont font = settings.globalFont("FileDiffView", mNewFile->font());
   mNewFile->setFont(font);
   mOldFile->setFont(font);

   const auto optionsLayout = new QHBoxLayout();
   optionsLayout->setContentsMargins(5, 5, 0, 0);
   optionsLayout->setSpacing(5);
   optionsLayout->addWidget(mGoPrevious);
   optionsLayout->addWidget(mGoNext);
   optionsLayout->addWidget(mFullView);
   optionsLayout->addWidget(mSplitView);
   optionsLayout->addStretch();

   const auto searchNew = new QLineEdit();
   searchNew->setObjectName("SearchInput");
   searchNew->setPlaceholderText(tr("Press Enter to search a text... "));
   connect(searchNew, &QLineEdit::editingFinished, this,
           [this, searchNew]() { DiffHelper::findString(searchNew->text(), mNewFile, this); });

   const auto newFileLayout = new QVBoxLayout();
   newFileLayout->setContentsMargins(QMargins());
   newFileLayout->setSpacing(5);
   newFileLayout->addWidget(searchNew);
   newFileLayout->addWidget(mNewFile);

   mSearchOld->setPlaceholderText(tr("Press Enter to search a text... "));
   mSearchOld->setObjectName("SearchInput");
   connect(mSearchOld, &QLineEdit::editingFinished, this,
           [this]() { DiffHelper::findString(mSearchOld->text(), mOldFile, this); });

   const auto oldFileLayout = new QVBoxLayout();
   oldFileLayout->setContentsMargins(QMargins());
   oldFileLayout->setSpacing(5);
   oldFileLayout->addWidget(mSearchOld);
   oldFileLayout->addWidget(mOldFile);

   const auto diffLayout = new QHBoxLayout();
   diffLayout->setContentsMargins(10, 0, 10, 0);
   diffLayout->addLayout(newFileLayout);
   diffLayout->addLayout(oldFileLayout);

   const auto diffFrame = new QFrame();
   diffFrame->setLayout(diffLayout);

   const auto vLayout = new QVBoxLayout(this);
   vLayout->setContentsMargins(QMargins());
   vLayout->setSpacing(5);
   vLayout->addLayout(optionsLayout);
   vLayout->addWidget(diffFrame);

   mFileVsFile = settings.localValue(GitQlientSettings::SplitFileDiffView, false).toBool();

   mGoPrevious->setIcon(QIcon(":/icons/arrow_up"));
   mGoPrevious->setToolTip(tr("Previous change"));
   connect(mGoPrevious, &QPushButton::clicked, this, &FileDiffWidget::moveChunkUp);

   mGoNext->setToolTip(tr("Next change"));
   mGoNext->setIcon(QIcon(":/icons/arrow_down"));
   connect(mGoNext, &QPushButton::clicked, this, &FileDiffWidget::moveChunkDown);

   mFullView->setIcon(QIcon(":/icons/text-file"));
   mFullView->setCheckable(true);
   mFullView->setToolTip(tr("Full file view"));
   connect(mFullView, &QPushButton::toggled, this, &FileDiffWidget::setFullViewEnabled);

   mSplitView->setIcon(QIcon(":/icons/split_view"));
   mSplitView->setCheckable(true);
   mSplitView->setToolTip(tr("Split file view"));
   connect(mSplitView, &QPushButton::toggled, this, &FileDiffWidget::setSplitViewEnabled);

   if (!mFileVsFile)
   {
      mOldFile->setHidden(true);
      mSearchOld->setHidden(true);
   }

   connect(mNewFile, &FileDiffView::signalScrollChanged, mOldFile, &FileDiffView::moveScrollBarToPos);
   connect(mOldFile, &FileDiffView::signalScrollChanged, mNewFile, &FileDiffView::moveScrollBarToPos);

   setAttribute(Qt::WA_DeleteOnClose);
}

void FileDiffWidget::clear()
{
   mNewFile->clear();
}

bool FileDiffWidget::reload()
{
   if (mCurrentSha == ZERO_SHA)
      return configure(mCurrentSha, mPreviousSha, mCurrentFile, mIsCached);

   return false;
}

void FileDiffWidget::updateFontSize()
{
   GitQlientSettings settings;
   QFont font = settings.globalFont("FileDiffView", mNewFile->font());
   auto cursor = mNewFile->textCursor();

   mNewFile->selectAll();
   mNewFile->setFont(font);
   mNewFile->setTextCursor(cursor);

   cursor = mOldFile->textCursor();
   mOldFile->selectAll();
   mOldFile->setFont(font);
   mOldFile->setTextCursor(cursor);
}

bool FileDiffWidget::configure(const QString &currentSha, const QString &previousSha, const QString &file,
                               bool isCached)
{
   auto destFile = file;

   if (destFile.contains("-->"))
      destFile = destFile.split("--> ").last().split("(").first().trimmed();

   QString text;
   QScopedPointer<GitHistory> git(new GitHistory(mGit));

   if (const auto ret
       = git->getFullFileDiff(currentSha == ZERO_SHA ? QString() : currentSha, previousSha, destFile, isCached);
       ret.success)
   {
      text = ret.output;

      if (text.isEmpty())
      {
         if (const auto ret = git->getUntrackedFileDiff(destFile); ret.success)
            text = ret.output;
      }

      if (text.startsWith("* "))
         return false;
   }

   mIsCached = isCached;
   mCurrentFile = file;
   mCurrentSha = currentSha;
   mPreviousSha = previousSha;

   auto pos = 0;
   for (auto i = 0; i < 5; ++i)
      pos = text.indexOf("\n", pos + 1);

   text = text.mid(pos + 1);

   if (!text.isEmpty())
   {
      if (mFileVsFile)
      {
         QPair<QStringList, QVector<ChunkDiffInfo::ChunkInfo>> oldData;
         QPair<QStringList, QVector<ChunkDiffInfo::ChunkInfo>> newData;

         mChunks = DiffHelper::processDiff(text, newData, oldData);

         mOldFile->blockSignals(true);
         mOldFile->loadDiff(oldData.first.join('\n'), oldData.second);
         mOldFile->blockSignals(false);

         mNewFile->blockSignals(true);
         mNewFile->loadDiff(newData.first.join('\n'), newData.second);
         mNewFile->blockSignals(false);
      }
      else
      {
         const auto data = DiffHelper::processDiff(text);

         mNewFile->blockSignals(true);
         mNewFile->loadDiff(text, data);
         mNewFile->blockSignals(false);
      }

      mFullView->setChecked(!mFileVsFile);
      mSplitView->setChecked(mFileVsFile);

      return true;
   }

   return false;
}

void FileDiffWidget::setSplitViewEnabled(bool enable)
{
   mFileVsFile = enable;

   mOldFile->setVisible(mFileVsFile);
   mSearchOld->setVisible(mFileVsFile);

   GitQlientSettings settings(mGit->getGitDir());
   settings.setLocalValue(GitQlientSettings::SplitFileDiffView, mFileVsFile);

   configure(mCurrentSha, mPreviousSha, mCurrentFile, mIsCached);

   mFullView->blockSignals(true);
   mFullView->setChecked(!mFileVsFile);
   mFullView->blockSignals(false);

   mGoNext->setEnabled(true);
   mGoPrevious->setEnabled(true);
}

void FileDiffWidget::setFullViewEnabled(bool enable)
{
   mFileVsFile = !enable;

   mOldFile->setVisible(mFileVsFile);
   mSearchOld->setVisible(mFileVsFile);

   GitQlientSettings settings(mGit->getGitDir());
   settings.setLocalValue(GitQlientSettings::SplitFileDiffView, mFileVsFile);

   configure(mCurrentSha, mPreviousSha, mCurrentFile, mIsCached);

   mSplitView->blockSignals(true);
   mSplitView->setChecked(mFileVsFile);
   mSplitView->blockSignals(false);

   mGoNext->setDisabled(true);
   mGoPrevious->setDisabled(true);
}

void FileDiffWidget::moveChunkUp()
{
   for (auto i = mChunks.chunks.count() - 1; i >= 0; --i)
   {
      auto &chunk = mChunks.chunks.at(i);

      if (auto [chunkNewStart, chunkOldStart] = std::make_tuple(chunk.newFile.startLine, chunk.oldFile.startLine);
          chunkNewStart < mCurrentChunkLine || chunkOldStart < mCurrentChunkLine)
      {
         if (chunkNewStart < mCurrentChunkLine)
            mCurrentChunkLine = chunkNewStart;
         else if (chunkOldStart < mCurrentChunkLine)
            mCurrentChunkLine = chunkOldStart;

         mNewFile->moveScrollBarToPos(mCurrentChunkLine - 1);
         mOldFile->moveScrollBarToPos(mCurrentChunkLine - 1);

         break;
      }
   }
}

void FileDiffWidget::moveChunkDown()
{
   const auto endIter = mChunks.chunks.constEnd();
   auto iter = mChunks.chunks.constBegin();

   for (; iter != endIter; ++iter)
   {
      if (iter->newFile.startLine > mCurrentChunkLine)
      {
         mCurrentChunkLine = iter->newFile.startLine;
         break;
      }
      else if (iter->oldFile.startLine > mCurrentChunkLine)
      {
         mCurrentChunkLine = iter->oldFile.startLine;
         break;
      }
   }

   if (iter != endIter)
   {
      mNewFile->moveScrollBarToPos(mCurrentChunkLine - 1);
      mOldFile->moveScrollBarToPos(mCurrentChunkLine - 1);
   }
}

void FileDiffWidget::enterEditionMode(bool enter)
{
   if (enter)
   {
      mSplitView->blockSignals(true);
      mSplitView->setChecked(!enter);
      mSplitView->blockSignals(false);

      mFullView->blockSignals(true);
      mFullView->setChecked(!enter);
      mFullView->blockSignals(false);
   }
   else if (mFileVsFile)
      setSplitViewEnabled(true);
   else
      setFullViewEnabled(true);
}

void FileDiffWidget::stageFile()
{
   QScopedPointer<GitLocal> git(new GitLocal(mGit));
   const auto ret = git->stageFile(mCurrentFile);

   if (ret.success)
   {
      emit fileStaged(mCurrentFile);
      emit exitRequested();
   }
}

void FileDiffWidget::revertFile()
{
   const auto ret = QMessageBox::warning(
       this, tr("Revert all changes"),
       tr("Please, take into account that this will revert all the changes you have performed so far."),
       QMessageBox::Ok, QMessageBox::Cancel);

   if (ret == QMessageBox::Ok)
   {
      QScopedPointer<GitLocal> git(new GitLocal(mGit));
      const auto ret = git->checkoutFile(mCurrentFile);

      if (ret)
      {
         emit fileReverted(mCurrentFile);
         emit exitRequested();
      }
   }
}

void FileDiffWidget::stageChunk(const QString &id)
{
   const auto iter = std::find_if(mChunks.chunks.cbegin(), mChunks.chunks.cend(),
                                  [id](const ChunkDiffInfo &info) { return id == info.id; });

   if (iter != mChunks.chunks.cend())
   {
      auto startingLine = 0;

      if (iter->newFile.startLine != -1
          && (iter->oldFile.startLine == -1 || iter->newFile.startLine < iter->oldFile.startLine))
         startingLine = iter->newFile.startLine;
      else
         startingLine = iter->oldFile.startLine;

      const auto buffer = 3;
      QString text;
      QString postLine = " \n";
      auto fileCount = startingLine - 1 - buffer;

      if (fileCount < 0)
         fileCount = 0;

      for (; fileCount < startingLine - 1 && fileCount < mChunks.oldFileDiff.count(); ++fileCount)
         text.append(QString(" %1\n").arg(mChunks.oldFileDiff.at(fileCount)));

      if (fileCount < mChunks.oldFileDiff.count())
         postLine = QString(" %1\n").arg(mChunks.oldFileDiff.at(fileCount));

      auto totalLinesOldFile = 0;

      if (iter->oldFile.startLine != -1)
      {
         const auto realStart = iter->oldFile.startLine - 1;
         totalLinesOldFile = iter->oldFile.startLine == iter->oldFile.endLine ? 1 : iter->oldFile.endLine - realStart;

         auto i = realStart;
         for (; i < iter->oldFile.endLine && i < mChunks.oldFileDiff.count(); ++i)
            text.append(QString("-%1\n").arg(mChunks.oldFileDiff.at(i)));

         postLine = QString(" %1\n").arg(mChunks.oldFileDiff.at(i));
      }

      auto totalLinesNewFile = 0;

      if (iter->newFile.startLine != -1)
      {
         const auto realStart = iter->newFile.startLine - 1;
         totalLinesNewFile = iter->newFile.startLine == iter->newFile.endLine ? 1 : iter->newFile.endLine - realStart;

         for (auto i = realStart; i < iter->newFile.endLine && i < mChunks.newFileDiff.count(); ++i)
            text.append(QString("+%1\n").arg(mChunks.newFileDiff.at(i)));
      }

      text.append(postLine);

      const auto filePath = QString(mCurrentFile).remove(mGit->getWorkingDir());
      const auto patch
          = QString("--- a%1\n"
                    "+++ b%1\n"
                    "@@ -%2,%3 +%2,%4 @@\n"
                    "%5")
                .arg(filePath, QString::number(startingLine - buffer), QString::number(buffer + totalLinesOldFile + 1),
                     QString::number(buffer + totalLinesNewFile + 1), text);

      QTemporaryFile f;

      if (f.open())
      {
         f.write(patch.toUtf8());
         f.close();

         QScopedPointer<GitPatches> git(new GitPatches(mGit));

         if (const auto ret = git->stagePatch(f.fileName()); ret.success)
            QMessageBox::information(this, tr("Changes staged!"), tr("The chunk has been successfully staged."));
         else
         {
#ifdef DEBUG
            QFile patch("aux.patch");

            if (patch.open(QIODevice::WriteOnly) && f.open())
            {
               patch.write(f.readAll());
               patch.close();
               f.close();
            }
#endif
            QMessageBox::information(this, tr("Stage failed"),
                                     tr("The chunk couldn't be applied:\n%1").arg(ret.output));
         }
      }
   }
}
