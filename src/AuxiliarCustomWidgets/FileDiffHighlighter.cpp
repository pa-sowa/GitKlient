#include "FileDiffHighlighter.h"

#include <GitQlientStyles.h>
#include <QPalette>
#include <QTextDocument>

FileDiffHighlighter::FileDiffHighlighter(QTextDocument *document)
   : QSyntaxHighlighter(document)
{
}

void FileDiffHighlighter::highlightBlock(const QString &text)
{
   setCurrentBlockState(previousBlockState() + 1);

   QTextBlockFormat blockFormat;
   QTextCharFormat charFormat;
   const auto currentLine = currentBlock().blockNumber() + 1;

   if (!mFileDiffInfo.isEmpty())
   {
      for (const auto &diff : qAsConst(mFileDiffInfo))
      {
         if (diff.startLine <= currentLine && currentLine <= diff.endLine)
         {
            if (diff.addition)
            {
               charFormat.setForeground(GitQlientStyles::getGreen());
            }
            else
               charFormat.setForeground(GitQlientStyles::getRed());
         }
      }
   }
   else if (!text.isEmpty())
   {
      switch (text.at(0).toLatin1())
      {
         case '@': {
            QPalette palette;
            charFormat.setForeground(palette.color(QPalette::WindowText));
            blockFormat.setBackground(palette.color(QPalette::Window));
            break;
         }
         case '+':
            charFormat.setForeground(GitQlientStyles::getGreen());
            break;
         case '-':
            charFormat.setForeground(GitQlientStyles::getRed());
            break;
         default:
            break;
      }
   }

   if (blockFormat.isValid())
   {
      QTextCursor(currentBlock()).setBlockFormat(blockFormat);
   }

   if (charFormat.isValid())
   {
      setFormat(0, currentBlock().length(), charFormat);
   }
}
