![GitKlient logo](https://github.com/pa-sowa/GitKlient/blob/develop/src/resources/icons/GitKlientLogo96.png "GitKlient")

# GitKlient: a Git GUI for KDE

GitKlient is a Git client originally forked from [GitQlient](https://github.com/francescmm/GitQlient) and adapted to fit the KDE desktop environment.

![GitKlient main screen](/docs/assets/GitQlient.png)

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

## Interactive UI guide (with code links)

There is a webpage for those developers that want to get some help understanding the different parts of the UI of the original GitQlient and how they are connected with the C++ code.

The web shows the current UI design with links to the code of the widgets when you click on the areas of the image. The code is shown in a frame near to the image, so a 1920px screen might be needed. Since I'm not a web developer and I don't intend to dedicate too much time to that, I'll update the guide only with major releases. If anybody wants to make it pritier and knows how to do it, please contact me to see if we can collaborate.

[Check the Interactive UI guide of GitQlient.](https://francescmm.github.io/gitqlient/)


## Translating GitKlient

GitKlient is using the translation system of Qt. That means that for every new language two files are needed: .ts and .qm. The first one is the text translation and the second one is a compiled file that GitKlient will load.

To add a new translation, please generate those files and add them to the resources.qrc.

For more information on [Qt translation system](https://doc.qt.io/qt-5/linguist-manager.html).

### Building GitKlient

In the [User Manual](https://francescmm.github.io/GitKlient/#appendix-b-build) you can find a whole section about building GitKlient and what dependencies you need.
  
## Licenses

Most of the icons on GitKlient are from Font Awesome. [The license states is GPL friendly](https://fontawesome.com/license/free). Those icons that are not from Font Awesome are custom made icons.
