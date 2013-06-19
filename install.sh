#!/bin/bash

# copy binary
cp ./build/yanta ~/local/bin/splines

# register icon
# xdg-icon-resource install --context mimetypes --size 48 splines-file-type.png x-application-splines

# copy mime-type configuration
cp splines-mime.xml ~/.local/share/mime/packages/application-x-splines.xml

# copy desktop entry
cp splines.desktop ~/.local/share/applications/splines.desktop

# let the system process the changes
update-mime-database ~/.local/share/mime
update-desktop-database ~/.local/share/applications
