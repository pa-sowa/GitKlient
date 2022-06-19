#pragma once

#include <QDialog>
#include <QMap>

class GitBase;
class QTimer;
class FileEditor;
class QPushButton;
class QAbstractButton;
class QButtonGroup;

namespace Ui
{
class ConfigDialog;
}

class ConfigDialog : public QDialog
{
   Q_OBJECT

signals:
   void reloadView();
   void reloadDiffFont();
   void commitTitleMaxLenghtChanged();
   void panelsVisibilityChanged();

public:
   explicit ConfigDialog(const QSharedPointer<GitBase> &git, QWidget *parent = nullptr);
   ~ConfigDialog();

private:
   Ui::ConfigDialog *ui;
   QSharedPointer<GitBase> mGit;
   int mOriginalRepoOrder = 0;
   bool mShowResetMsg = false;
   FileEditor *mLocalGit = nullptr;
   FileEditor *mGlobalGit = nullptr;
   QButtonGroup *m_buttonGroup = nullptr;

   void clearCache();
   void calculateCacheSize();
   void enableWidgets();
   void saveFile();
   void showCredentialsDlg();

private slots:
   void onAccepted();
   void saveConfig();
   void onCredentialsOptionChanged(QAbstractButton *button);
   void onPullStrategyChanged(int index);
};
