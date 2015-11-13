xbmc-txupdate
=============

This utility is suitable for keeping XBMC upstream language files and the language files hosted on transifex.com in sync.

What it does:
* Downloads the fresh files from upstream http URLs specified in the xbmc-txupdate.xml file and also downloads the fresh translations from transifex.net and makes a merge of the files. 
* With this merge process it creates fresh files containing all changes upstream and on transifex. 
* These updated files than can be commited to upstream repositories for end user usage.
* During the merge process it also creates update files which only contain the new upstream version if the English language files and also contain the new English strings translations introduced for different languagees on the upstream repository. These update PO files than can automatically uploaded to transifex with this utility.

Important to note that in case we both have a translation at the upstream repository and we have a translation at transifex.net for the same English string, the utility prefers the one at transifex.net. This means that new translations modifications can only be pulled from ustream into the merged files in case they are for completely newly introduced English strings which do not have translation existing at transifex yet.

## Install
Requirements:
* OS: Linux
* Packages: curl, libcurl, libjsoncpp

Ubuntu prerequisites installation:
```
sudo apt-get install build-essential curl libcurl4-gnutls-dev libjsoncpp0 libjsoncpp-dev git
```
After git cloning the utility, simply run make. The bin file called "xbmc-txupdate" will be created.

## Usage


  **xbmc-txpudate PROJECTDIR**


  * **PROJECTDIR:** the directory which contains the xbmc-txupdate.xml settings file and the .passwords file. This will be the directory where your merged and transifex update files get generated.

Please note that the utility needs at least a projet setup conf file in the PROJECTDIR to work. Additionally it needs to have a passwords xml file which holds credentials for Transifex.com and for the http places you are using as upstream repositories.

## Setting files
In you PROJECTDIR folder you need to have the following files:


**II. .passwords.xml**

This file stores http password to access protected upstream or Transifex language files. For Transifex you must specify your user credentials here.
The format of the file looks like this:

```xml
<?xml version="1.0" ?>
<websites>
    <website prefix="">
        <login></login>
        <password></password>
    </website>
</websites>
```

Where:
   * Attribute prefix: The URL prefix of the website you want to use a password with. For example for Transifex, you have to use : "https://www.transifex.com/api/2/project/"

## Tips and tricks
* Cache files for http download and upload actions are stored in the .httpcache directory. If the format of a downloaded file is corrupt upstream, you can find the file here and correct it to continue. If you want to be sure to use the latest upstream files, you can simply delete this directory.
* Don't run in upload mode, before doing a succesful download and merge. The status is stored in a file called ".dload_merge_status" in the .httpcache directory. If there is an OK in it, download and merge has run successfully before.
* The xbmc-txupdate.xml file is monitored if it has been changed before upload. The old state of the file is stored in the ".last_xbmc-txupdate.xml" file in .httpcache. If you want to force-run upload mode, you can copy the changed xml file as this name to cheat the utility. Only do this, if you know what you are doing!

For any questions, please write to: alanwww1@xbmc.org

2012 Attila Jakosa, Team XBMC
