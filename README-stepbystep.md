kodi-txupdate step-by-step guide
================================

## Installation

First of all we need to have the following packages insatlled on our target server:
'curl, libcurl-dev, libjsoncpp-dev, git, vim'

In case, the server runs Ubuntu, the install line:
```
sudo apt-get install build-essential curl libcurl4-gnutls-dev libjsoncpp0 libjsoncpp-dev git vim
```

On the server we use username `translator` for our job.

We need to git clone the kodi translations github repo first.
```
cd ~
mkdir transifex
cd transifex
git clone git@github.com:xbmc/translations.git
```

We compile the utility now:
```
cd translations/tool/kodi-txupdate
make
make install
git clean -f -d -x
exit
```
We have to re-log into the ssh session, to re-read $PATH so that we can use the newly created binary: ~/bin/kodi-txupdate

## Installation

```
cd ~/transifex/translation
```

After git cloning the utility, simply run make. The bin file called "kodi-txupdate" will be created.
make install copies this file to ~/bin/.
