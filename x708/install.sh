#!/usr/bin/bash -x

if [[ "${UID}" -ne 0 ]]
then
  echo "This script must be run as root or with sudo."
  exit 1
fi

cp -f x708-pwr.service /lib/systemd/system/
cp -f xPWR.sh /usr/local/bin/
cp -f xSoft.sh /usr/local/bin/
systemctl daemon-reload
systemctl enable --now x708-pwr.service
