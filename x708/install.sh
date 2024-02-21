#!/usr/bin/bash -x

if [[ "${UID}" -ne 0 ]]
then
  echo "This script must be run as root or with sudo."
  exit 1
fi

cp x708-pwr.service /lib/systemd/system/
cp x708-pwr.sh /usr/local/bin/
cp x708-softsd.sh /usr/local/bin/
systemctl daemon-reload
systemctl enable --now x708-pwr.service
