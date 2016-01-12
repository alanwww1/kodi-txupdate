kodi-txupdate.conf files help
=============================

These configuarzion files are telling the kodi-txupdate utility all the important information about what resources have to be synced, where the upstream files can be found, what Transifex projects are related to them, what lanaguage codes to use, how to git commit the changes, what commit texts to use etc.

These files are located at the kodi translations git repo, under kodi-translations folder for 3 projects: kodi-main, kodi-skins, kodi-addons.
You can always check how a sample fo the file look, [here](https://github.com/xbmc/translations/blob/master/kodi-translations/kodi-skins/kodi-txupdate.conf)

It is always the best to view and edit these config files with vim, using the bundled and auto copied syntax highlighting vim config files.

## Main commands in the config files
* **setvar** creates a new "external" variable. the name is up for you to choose. Best use for this is to pre-define variables at the beggining of the config files and later re-use them at several places.
* **set** sets an "internal" variable. The internal variables control the working of the utility. Check the list below, which one controls what.
* **pset** permanently sets an internal variable re-doing the assignment at each create of new resource.
* **tset** temporary sets an internal variable only until a new resource is created. After the creation, it is not valid anymore for the next resource.
* **clear** clears an internal variable. Sets it to an empty state.
* **create resource** creates the resource with the previously given internal variables and the arguments given after this command (see later)

To use a variable called "VAR" just refer to it as $VAR in the assignment.

## Internal variables

First of all, the name of these variables are usually conatain the location of the file or resource it refers to.
These locations are defined as the following:
* **UPS** - upstream git location
* **LOC** - local clone of the upstream git repositories
* **TRX** - Transifex source location
* **UPD** - Transifex target location
* **MRG** - merged language files at the local kodi translations repo clone
* **UPSSRC** - like UPS, just in the special case of the en_GB language addon which is at a different place than the rest of the language files
* **LOCSRC** - like LOC, just in the special case of the en_GB language addon which is at a different place than the rest of the language files

**General internal variables**

* **ChgLogFormat**
* **GitCommitText**
* **GitCommitTextSRC**
* MRGLFilesDir
* UPSLocalPath
* UPDLFilesDir
* SupportEmailAddr
* SRCLCode
* BaseLForm
* LTeamLFormat
* LDatabaseURL
* MinComplPercent
* CacheExpire
* GitPushInterval
* ForceComm
* ForceGitDloadToCache
* SkipGitReset
* SkipGitPush
* ForceGitPush
* Rebrand
* ForceTXUpd
* IsLangAddon
* HasOnlyAddonXML


