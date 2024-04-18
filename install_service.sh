#!/bin/bash

sed "s/\$HOME/$(echo $HOME | sed 's/\//\\\//g')/g" python/bat.service > $HOME/.config/systemd/user/bat.service
