#include "GitQlientStyles.h"

#include <Colors.h>
#include <GitQlientSettings.h>

#include <QFile>
#include <QPalette>

GitQlientStyles *GitQlientStyles::INSTANCE = nullptr;

GitQlientStyles *GitQlientStyles::getInstance()
{
   if (INSTANCE == nullptr)
      INSTANCE = new GitQlientStyles();

   return INSTANCE;
}

QString GitQlientStyles::getStyles()
{
   QString styles;
   /*
      QFile stylesFile(":/stylesheet");

      if (stylesFile.open(QIODevice::ReadOnly))
      {
         const auto colorSchema = GitQlientSettings().globalValue("colorSchema", "dark").toString();
         QFile colorsFile(QString(":/colors_%1").arg(colorSchema));
         QString colorsCss;

         if (colorsFile.open(QIODevice::ReadOnly))
         {
            colorsCss = QString::fromUtf8(colorsFile.readAll());
            colorsFile.close();
         }

         styles = stylesFile.readAll() + colorsCss;

         stylesFile.close();
      }
   */
   return styles;
}

QColor GitQlientStyles::getTextColor()
{
   return QPalette().color(QPalette::Text);
}

QColor GitQlientStyles::getGraphSelectionColor()
{
   return QPalette().color(QPalette::Highlight);
}

QColor GitQlientStyles::getGraphHoverColor()
{
   return QPalette().color(QPalette::Highlight).lighter(175);
}

QColor GitQlientStyles::getBackgroundColor()
{
   return QPalette().color(QPalette::Base);
}

QColor GitQlientStyles::getTabColor()
{
   const auto colorSchema = GitQlientSettings().globalValue("colorSchema", "dark").toString();

   return colorSchema == "dark" ? graphHoverColorDark : graphBackgroundColorBright;
}

QColor GitQlientStyles::getBlue()
{
   const auto colorSchema = GitQlientSettings().globalValue("colorSchema", "dark").toString();

   return colorSchema == "dark" ? graphBlueDark : graphBlueBright;
}

QColor GitQlientStyles::getRed()
{
   return graphRed;
}

QColor GitQlientStyles::getGreen()
{
   return graphGreen;
}

QColor GitQlientStyles::getGrey()
{
   return graphGrey;
}

QColor GitQlientStyles::getOrange()
{
   return graphOrange;
}

std::array<QColor, GitQlientStyles::kBranchColors> GitQlientStyles::getBranchColors()
{
   static std::array<QColor, kBranchColors> colors { { getTextColor(), graphRed, getBlue(), graphGreen, graphOrange,
                                                       graphGrey, graphPink, graphPastel } };

   return colors;
}

QColor GitQlientStyles::getBranchColorAt(int index)
{
   if (index < kBranchColors && index >= 0)
      return getBranchColors().at(static_cast<size_t>(index));

   return QColor();
}
