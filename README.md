![GitKlient logo](https://github.com/pa-sowa/GitKlient/blob/master/src/resources/icons/GitKlientLogo96.png "GitKlient")

# GitKlient: a Git GUI for KDE

GitKlient is a Git client originally forked from [GitQlient](https://github.com/francescmm/GitQlient) and adapted to fit the KDE desktop environment.

![GitKlient main screen](/docs/assets/GitKlient.png)

## Main features

Some of the major feature you can find are:

1. Easy access to remote actions like: push, pull, submodules management and branches
2. Branches management
3. Tags and stashes management
4. Submodules handling
5. Allow to open several repositories in the same window
6. Better visualization of the commits and the work in progress
7. Better visualization of the repository view
8. GitHub/GitLab integration
9. Embedded text editor with syntax highlight for C++

For all the features take a look to the [Release Notes in the Wiki](https://github.com/pa-sowa/GitKlient/wiki).

## User Manual

Please, if you have any doubts about how to use it or you just want to know all you can do with GitKlient, take a look at [the user manual of the original GitQlient](https://francescmm.github.io/GitKlient).

## Translating GitKlient

GitKlient is using the translation system of Qt. That means that for every new language two files are needed: .ts and .qm. The first one is the text translation and the second one is a compiled file that GitKlient will load.

To add a new translation, please generate those files and add them to the resources.qrc.

For more information on [Qt translation system](https://doc.qt.io/qt-5/linguist-manager.html).

### Building GitKlient

In the [User Manual](https://francescmm.github.io/GitKlient/#appendix-b-build) you can find a whole section about building GitKlient and what dependencies you need.
  
## Licenses

Most of the icons on GitKlient are from Font Awesome. [The license states is GPL friendly](https://fontawesome.com/license/free). Those icons that are not from Font Awesome are custom made icons.
