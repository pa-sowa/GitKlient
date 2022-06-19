#pragma once

#include <QSharedPointer>

#include <RevisionFiles.h>
#include <WipRevisionInfo.h>

#include <optional>

class GitBase;

class GitWip
{
public:
   enum class FileStatus
   {
      BothModified,
      DeletedByThem,
      DeletedByUs
   };

   explicit GitWip(const QSharedPointer<GitBase> &git);

   QVector<QString> getUntrackedFiles() const;
   std::optional<QPair<QString, RevisionFiles>> getWipInfo() const;
   std::optional<FileStatus> getFileStatus(const QString &filePath) const;

private:
   QSharedPointer<GitBase> mGit;

   RevisionFiles fakeWorkDirRevFile(const QString &diffIndex, const QString &diffIndexCache) const;
};
