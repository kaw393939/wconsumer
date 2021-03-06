#!/bin/sh

set -e

mysql -e 'create database drupal;'

pear channel-discover pear.drush.org
pear install drush/drush
phpenv rehash

cd ..
drush dl drupal --drupal-project-rename=drupal
cd drupal

drush site-install --db-url=mysql://root:@127.0.0.1/drupal --yes
chmod +w ./sites/default/settings.php && echo "\$base_url = 'http://drupal.loc';" >> ./sites/default/settings.php

mv ../wconsumer ./sites/all/modules/
drush en --yes wconsumer
drush en --yes wconsumer_ui

cd sites/all/modules/wconsumer/