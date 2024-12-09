# BLE Disco

This is an app to spin a disco ball controlled by bluetooth low energy.

## Setup

### Create virtualenv

virtualenv disco
source ./disco/bin/activate

pip3 install aiohttp bleak 

### Edit working directory in service

vim disco-app.service

### Install Service

Steps to Set Up the Service:
Create the Service File:

Save the file as ~/.config/systemd/user/disco-app.service.
Reload the Systemd Daemon:

bash
Copy code
systemctl --user daemon-reload
Enable the Service:

This ensures the service starts automatically at login:
bash
Copy code
systemctl --user enable --now disco-app.service


To ensure it’s running correctly:
bash
Copy code
systemctl --user status disco-app.service
