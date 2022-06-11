#include "ConfigDialog.h"
#include "ui_ConfigDialog.h"

#include <FileEditor.h>
#include <GitBase.h>
#include <GitQlientSettings.h>
#include <QLogger.h>

#include <CredentialsDlg.h>
#include <GitConfig.h>
#include <GitCredentials.h>
#include <QButtonGroup>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QStandardPaths>
#include <QTimer>

using namespace QLogger;

namespace
{
qint64 dirSize(QString dirPath)
{
   qint64 size = 0;
   QDir dir(dirPath);

   auto entryList = dir.entryList(QDir::Files | QDir::System | QDir::Hidden);

   for (const auto &filePath : qAsConst(entryList))
   {
      QFileInfo fi(dir, filePath);
      size += fi.size();
   }

   entryList = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::System | QDir::Hidden);

   for (const auto &childDirPath : qAsConst(entryList))
      size += dirSize(dirPath + QDir::separator() + childDirPath);

   return size;
}

QString humanReadableSize(qint64 number)
{
   int i = 0;
   const QVector<QString> units = { "B", "kiB", "MiB", "GiB", "TiB", "PiB", "EiB", "ZiB", "YiB" };
   while (number > 1024)
   {
      number /= 1024;
      i++;
   }
   return QString::number(static_cast<double>(number), 'f', 2) + " " + units[i];
}

}

ConfigDialog::ConfigDialog(const QSharedPointer<GitBase> &git, QWidget *parent)
   : QDialog(parent)
   , ui(new Ui::ConfigDialog)
   , mGit(git)
   , m_buttonGroup(new QButtonGroup())
{
   ui->setupUi(this);

   const auto localGitLayout = new QVBoxLayout(ui->localGit);
   localGitLayout->setContentsMargins(QMargins());

   mLocalGit = new FileEditor(false, this);
   mLocalGit->editFile(mGit->getGitDir().append("/config"));
   localGitLayout->addWidget(mLocalGit);

   const auto globalGitLayout = new QVBoxLayout(ui->globalGit);
   globalGitLayout->setContentsMargins(QMargins());

   mGlobalGit = new FileEditor(false, this);
   mGlobalGit->editFile(
       QString("%1/%2").arg(QStandardPaths::writableLocation(QStandardPaths::HomeLocation), ".gitconfig"));
   globalGitLayout->addWidget(mGlobalGit);

   GitQlientSettings settings(mGit->getGitDir());

   enableWidgets();

   // GitQlient configuration
   ui->chEnableLogs->setChecked(settings.globalValue("logsEnabled", false).toBool());
   ui->cbLogLevel->setCurrentIndex(settings.globalValue("logsLevel", static_cast<int>(LogLevel::Warning)).toInt());
   ui->spCommitTitleLength->setValue(settings.globalValue("commitTitleMaxLength", 50).toInt());
   ui->sbEditorFontSize->setValue(settings.globalValue("FileDiffView/FontSize", 8).toInt());
   ui->cbEditorFontFamily->setCurrentText(settings.globalValue("FileDiffView/FontFamily").toString());

   const auto originalStyles = settings.globalValue("colorSchema", "dark").toString();
   ui->cbStyle->setCurrentText(originalStyles);

   // Repository configuration
   mOriginalRepoOrder = settings.localValue("GraphSortingOrder", 0).toInt();
   ui->cbLogOrder->setCurrentIndex(mOriginalRepoOrder);
   ui->autoFetch->setValue(settings.localValue("AutoFetch", 5).toInt());
   ui->pruneOnFetch->setChecked(settings.localValue("PruneOnFetch", true).toBool());
   ui->clangFormat->setChecked(settings.localValue("ClangFormatOnCommit", false).toBool());
   ui->updateOnPull->setChecked(settings.localValue("UpdateOnPull", false).toBool());
   ui->sbMaxCommits->setValue(settings.localValue("MaxCommits", 0).toInt());

   ui->tabWidget->setCurrentIndex(0);
   connect(ui->pbClearCache, &ButtonLink::clicked, this, &ConfigDialog::clearCache);

   ui->cbStash->setChecked(settings.localValue("StashesHeader", true).toBool());
   ui->cbSubmodule->setChecked(settings.localValue("SubmodulesHeader", true).toBool());
   ui->cbSubtree->setChecked(settings.localValue("SubtreeHeader", true).toBool());
   ui->cbDeleteFolder->setChecked(settings.localValue("DeleteRemoteFolder", false).toBool());

   // Build System configuration
   const auto isConfigured = settings.localValue("BuildSystemEnabled", false).toBool();
   ui->chBoxBuildSystem->setChecked(isConfigured);
   connect(ui->chBoxBuildSystem, &QCheckBox::stateChanged, this, &ConfigDialog::toggleBsAccesInfo);

   ui->leBsUser->setVisible(isConfigured);
   ui->leBsUserLabel->setVisible(isConfigured);
   ui->leBsToken->setVisible(isConfigured);
   ui->leBsTokenLabel->setVisible(isConfigured);
   ui->leBsUrl->setVisible(isConfigured);
   ui->leBsUrlLabel->setVisible(isConfigured);

   if (isConfigured)
   {
      const auto url = settings.localValue("BuildSystemUrl", "").toString();
      const auto user = settings.localValue("BuildSystemUser", "").toString();
      const auto token = settings.localValue("BuildSystemToken", "").toString();

      ui->leBsUrl->setText(url);
      ui->leBsUser->setText(user);
      ui->leBsToken->setText(token);
   }

   QScopedPointer<GitConfig> gitConfig(new GitConfig(mGit));

   const auto url = gitConfig->getServerUrl();
   ui->credentialsFrames->setVisible(url.startsWith("https"));

   const auto mergeStrategyFF = gitConfig->getGitValue("pull.ff").output;
   const auto mergeStrategyRebase = gitConfig->getGitValue("pull.rebase").output;

   if (mergeStrategyFF.isEmpty())
   {
      if (mergeStrategyRebase.isEmpty() || mergeStrategyRebase.toLower().contains("false"))
         ui->cbPullStrategy->setCurrentIndex(0);
      else if (mergeStrategyRebase.toLower().contains("true"))
         ui->cbPullStrategy->setCurrentIndex(1);
   }
   else if (mergeStrategyFF.toLower().contains("true"))
      ui->cbPullStrategy->setCurrentIndex(2);

   connect(ui->cbPullStrategy, SIGNAL(currentIndexChanged(int)), this, SLOT(onPullStrategyChanged(int)));

   connect(m_buttonGroup, SIGNAL(buttonClicked(QAbstractButton *)), this,
           SLOT(onCredentialsOptionChanged(QAbstractButton *)));
   connect(ui->pbAddCredentials, &QPushButton::clicked, this, &ConfigDialog::showCredentialsDlg);

   connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &ConfigDialog::onAccepted);
   connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &ConfigDialog::reject);

   calculateCacheSize();
}

ConfigDialog::~ConfigDialog()
{
   delete ui;
}

void ConfigDialog::onCredentialsOptionChanged(QAbstractButton *button)
{
   ui->sbTimeout->setEnabled(button == ui->rbCache);
}

void ConfigDialog::onPullStrategyChanged(int index)
{
   QScopedPointer<GitConfig> gitConfig(new GitConfig(mGit));

   switch (index)
   {
      case 0:
         gitConfig->unset("pull.ff");
         gitConfig->setLocalData("pull.rebase", "false");
         break;
      case 1:
         gitConfig->unset("pull.ff");
         gitConfig->setLocalData("pull.rebase", "true");
         break;
      case 2:
         gitConfig->unset("pull.rebase");
         gitConfig->setLocalData("pull.ff", "only");
         break;
   }
}

void ConfigDialog::clearCache()
{
   const auto path = QString("%1").arg(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
   QProcess p;
   p.setWorkingDirectory(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
   p.start("rm", { "-rf", path });

   if (p.waitForFinished())
      calculateCacheSize();
}

void ConfigDialog::calculateCacheSize()
{
   auto size = 0;
   const auto dirPath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
   QDir dir(dirPath);
   QDir::Filters dirFilters = QDir::Dirs | QDir::NoDotAndDotDot | QDir::System | QDir::Hidden | QDir::Files;
   const auto &list = dir.entryInfoList(dirFilters);

   for (const QFileInfo &file : list)
   {
      size += file.size();
      size += dirSize(dirPath + "/" + file.fileName());
   }

   ui->lCacheSize->setText(humanReadableSize(size));
}

void ConfigDialog::toggleBsAccesInfo()
{
   const auto visible = ui->chBoxBuildSystem->isChecked();
   ui->leBsUser->setVisible(visible);
   ui->leBsUserLabel->setVisible(visible);
   ui->leBsToken->setVisible(visible);
   ui->leBsTokenLabel->setVisible(visible);
   ui->leBsUrl->setVisible(visible);
   ui->leBsUrlLabel->setVisible(visible);
}

void ConfigDialog::saveConfig()
{
   GitQlientSettings settings(mGit->getGitDir());

   settings.setGlobalValue("logsEnabled", ui->chEnableLogs->isChecked());
   settings.setGlobalValue("logsLevel", ui->cbLogLevel->currentIndex());
   settings.setGlobalValue("commitTitleMaxLength", ui->spCommitTitleLength->value());
   settings.setGlobalValue("FileDiffView/FontSize", ui->sbEditorFontSize->value());
   settings.setGlobalValue("FileDiffView/FontFamily", ui->cbEditorFontFamily->currentText());
   settings.setGlobalValue("colorSchema", ui->cbStyle->currentText());
   settings.setGlobalValue("gitLocation", ui->leGitPath->text());

   mLocalGit->changeFontSize();
   mGlobalGit->changeFontSize();

   emit reloadDiffFont();
   emit commitTitleMaxLenghtChanged();

   if (mShowResetMsg)
   {
      QMessageBox::information(this, tr("Reset needed!"),
                               tr("You need to restart GitQlient to see the changes in the styles applid."));
   }

   const auto logger = QLoggerManager::getInstance();
   logger->overwriteLogLevel(static_cast<LogLevel>(ui->cbLogLevel->currentIndex()));

   if (ui->chEnableLogs->isChecked())
      logger->resume();
   else
      logger->pause();

   if (mOriginalRepoOrder != ui->cbLogOrder->currentIndex())
   {
      settings.setLocalValue("GraphSortingOrder", ui->cbLogOrder->currentIndex());
      emit reloadView();
   }

   settings.setLocalValue("AutoFetch", ui->autoFetch->value());
   settings.setLocalValue("PruneOnFetch", ui->pruneOnFetch->isChecked());
   settings.setLocalValue("ClangFormatOnCommit", ui->clangFormat->isChecked());
   settings.setLocalValue("UpdateOnPull", ui->updateOnPull->isChecked());
   settings.setLocalValue("MaxCommits", ui->sbMaxCommits->value());

   settings.setLocalValue("StashesHeader", ui->cbStash->isChecked());
   settings.setLocalValue("SubmodulesHeader", ui->cbSubmodule->isChecked());
   settings.setLocalValue("SubtreeHeader", ui->cbSubtree->isChecked());

   settings.setLocalValue("DeleteRemoteFolder", ui->cbDeleteFolder->isChecked());

   emit panelsVisibilityChanged();

   /* BUILD SYSTEM CONFIG */

   const auto showBs = ui->chBoxBuildSystem->isChecked();
   const auto bsUser = ui->leBsUser->text();
   const auto bsToken = ui->leBsToken->text();
   const auto bsUrl = ui->leBsUrl->text();

   if (showBs && !bsUser.isEmpty() && !bsToken.isEmpty() && !bsUrl.isEmpty())
   {
      settings.setLocalValue("BuildSystemEnabled", showBs);
      settings.setLocalValue("BuildSystemUrl", bsUrl);
      settings.setLocalValue("BuildSystemUser", bsUser);
      settings.setLocalValue("BuildSystemToken", bsToken);
      emit buildSystemConfigured(showBs);
   }
   else
   {
      settings.setLocalValue("BuildSystemEnabled", false);
      emit buildSystemConfigured(false);
   }
}

void ConfigDialog::enableWidgets()
{
   ui->tabWidget->setEnabled(true);
}

void ConfigDialog::saveFile()
{
   const auto id = ui->tabWidget->currentIndex();

   if (id == 0)
      mLocalGit->saveFile();
   else
      mGlobalGit->saveFile();
}

void ConfigDialog::showCredentialsDlg()
{
   // Store credentials if allowed and the user checked the box
   if (ui->credentialsFrames->isVisible() && ui->chbCredentials->isChecked())
   {
      if (ui->rbCache->isChecked())
         GitCredentials::configureCache(ui->sbTimeout->value(), mGit);
      else
      {
         CredentialsDlg dlg(mGit, this);
         dlg.exec();
      }
   }
}

void ConfigDialog::onAccepted()
{
   saveConfig();
   accept();
}
