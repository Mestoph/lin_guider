#!/bin/bash

if [ $USER == "root" ]; then
	FLIST=`ls /home/*/.config/GM_software/devconf*.conf 2>/dev/null`
else
	FLIST=`ls ~/.config/GM_software/devconf*.conf 2>/dev/null`
fi

if [ ! $FLIST == "" ]; then
	echo "Previous configuration of Lin_guider has been detected."
	echo "Lin_guider 3.0.0 configuration files have been changed and older configuratios may lead to erratic behaviour."
	echo "It is highy recommended to remove the old config files (they will be remaned as *.back)"
	echo
	read -p "Remove old configurations? [y/N] " resp
	if [[ $resp =~ ^([yY][eE][sS]|[yY])$ ]]
	then
		for FILE in $FLIST; do
			echo "File $FILE moved to $FILE.back"
			mv $FILE $FILE.back
		done
	else
		echo "The ols congiuration is kept!"
	fi
fi
