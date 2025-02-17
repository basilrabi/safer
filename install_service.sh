#!/bin/bash

path='\$HOME/bin'

if ! grep -qF "$path" ~/.bashrc; then
    echo 'export PATH=$HOME/bin:$PATH' >> ~/.bashrc
fi

if ! grep -qF "x708off" ~/.bashrc; then
    echo "alias x708off='sudo /usr/local/bin/xSoft.sh 0 13'" >> ~/.bashrc
fi

mkdir -p $HOME/.config/systemd/user
sed "s/\$HOME/$(echo $HOME | sed 's/\//\\\//g')/g" python/bat.service > $HOME/.config/systemd/user/bat.service
