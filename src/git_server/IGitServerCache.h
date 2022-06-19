#pragma once

/****************************************************************************************
 ** GitQlient is an application to manage and operate one or several Git repositories. With
 ** GitQlient you will be able to add commits, branches and manage all the options Git provides.
 ** Copyright (C) 2021  Francesc Martinez
 **
 ** LinkedIn: www.linkedin.com/in/cescmm/
 ** Web: www.francescmm.com
 **
 ** This program is free software; you can redistribute it and/or
 ** modify it under the terms of the GNU Lesser General Public
 ** License as published by the Free Software Foundation; either
 ** version 2 of the License, or (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 ** Lesser General Public License for more details.
 **
 ** You should have received a copy of the GNU Lesser General Public
 ** License along with this library; if not, write to the Free Software
 ** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 ***************************************************************************************/

#include <ConfigData.h>

namespace GitServerPlugin
{
enum class Platform;
class IRestApi;
struct Issue;
struct Label;
struct Milestone;
struct PullRequest;
};

class IGitServerCache
{
public:
   virtual ~IGitServerCache() = default;

   virtual bool init(GitServerPlugin::ConfigData data) = 0;
   virtual QString getUserName() const = 0;
   virtual QVector<GitServerPlugin::PullRequest> getPullRequests() const = 0;
   virtual GitServerPlugin::PullRequest getPullRequest(int number) const = 0;
   virtual GitServerPlugin::PullRequest getPullRequest(const QString &sha) const = 0;
   virtual QVector<GitServerPlugin::Issue> getIssues() const = 0;
   virtual GitServerPlugin::Issue getIssue(int number) const = 0;
   virtual QVector<GitServerPlugin::Label> getLabels() const = 0;
   virtual QVector<GitServerPlugin::Milestone> getMilestones() const = 0;

   virtual GitServerPlugin::Platform getPlatform() const = 0;
   virtual GitServerPlugin::IRestApi *getApi() const = 0;
};
