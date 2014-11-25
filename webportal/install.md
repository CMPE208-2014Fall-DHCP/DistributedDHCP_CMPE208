# Web Portal Install instructions.

Update package tool first. sudo apt-get update

## Install Node and NPM
sudo apt-get install node

sudo apt-get install npm


## Go to web portal directory
cd webportal/

## Install dependencies
sudo npm install


## For test, mv the db
sudo cp db /var/db/dhcpd.lease

sudo chmod 666 dhcpd.lease

## Run
for Debian/Ubuntu:
sudo nodejs bin/www

for BSD
sudo node bin/www

## View
Go to http://<your_host>:3000


