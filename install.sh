#!/bin/bash

# copy binary
cp ./build/yanta ~/local/bin/yanta

# register icon
# xdg-icon-resource install --context mimetypes --size 48 yanta-file-type.png x-application-yanta

# copy mime-type configuration
cp yanta-mime.xml ~/.local/share/mime/packages/application-x-yanta.xml

# copy desktop entry
cp yanta.desktop ~/.local/share/applications/yanta.desktop

# let the system process the changes
update-mime-database ~/.local/share/mime
update-desktop-database ~/.local/share/applications
