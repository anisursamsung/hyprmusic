# HyprMusic

A dedicated Hyprland frontend for the Media Player Daemon (MPD) on Linux, built with C++ and the Hyprtoolkit API.


## Features
We have Tabs for Player View, Current Queue, Database, Playlists, YT-DLP and Settings.
Bottom contains all the controls along with tab navigation bar.
Songs can easily be moved around from Databse to Que to Playlist to Que etc.
YT-DLP offers to download to database or direct add to que and stream.
Visualizers are there. On click switches to next visualizations.
IconPack is embeded into the binary. No need of installation of any icon pack. We made it deliberate.

## File Browser Integration:
The make.sh after installing, automatially updates mime-lists and .desktop file has mime type set. So, once installed, it will automatically be offered as "Open With" option in any File Manager. And we can easily play it from File Browser.

## No duplicate state
Only single state of the player is allowed.

## Theming 
We want consistency and less headache while setting theme, so, like other hypr-ecosystem apps e.g. hyprlauncher, our hyprmusic app follows the Hyprtoolkit theme definition. It is defined in the root/system already but user can customise it in ~/.config/hypr/hyprtoolkit.conf. Have a look at "https://wiki.hypr.land/Hypr-Ecosystem/hyprtoolkit/" in the Configuration section. We have utilised almost all of the definitions there. So, if you define background color there, same background color will be used in hyprmusic. Only thing we did not use is the IconPack becasue we wanted signature look and gurantee the icons to render theme specific colors. Moreover, we wanted our own icon set hence we created a font pack(actually it is icon pack) and embeded inside the app. 
TL-DR: It inherits ~/.config/hypr/hyprtoolkit.conf definitions of color/font/etc. Except icon pack. If not defined, it uses system predefined hyprtoolkit.conf.

**Interested in the look of the screenshots attached?**
It was obtained by setting the background color to 50% transparent color in hyprtoolkit.conf and blur definition is in hyprland.lua. You are on arch hyprland, you are pro. You know what I am talking about.

## Important
It is MPD front-end hence it will do nothing is MPD is not installed.

## Installation
Clone and run the make.sh script. It will compile, put the .desktop file into /home/user/.local/share/applications so that it appears in your launcher/rofi/etc. The binary is copied to /home/user/.local/bin. So, ensure /home/user/.local/bin is in your env path in the bashrc. If you want system install, you can specify the path to put the bin e.g. "sudo make.sh /usr/bin", sudo is needed because we are going into root directory. For .desktop file, you can copy to system, but not recommended. Only binary placing into system is enough. Any launcher scan /home/user/.local/share/applications anyways.



## Screenshots
![screenshot1](assets/Screenshot1.png)
![screenshot2](assets/Screenshot2.png)
![screenshot3](assets/Screenshot3.png)
![screenshot4](assets/Screenshot4.png)
![screenshot5](assets/Screenshot5.png)
![screenshot6](assets/Screenshot6.png)
![screenshot7](assets/Screenshot7.png)





