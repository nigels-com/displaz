# Flatpak builds of displaz

## Building the flatpak

1. Build
   `$ flatpak-builder --repo=repo --force-clean tmp com.fugro.roames.displaz.json`
2. Install (user scoped) 
   `$ flatpak install --user --reinstall ./repo com.fugro.roames.displaz`
3. Run
   `$ flatpak run com.fugro.roames.displaz`
4. (Optional) bash alias
   `$ alias displaz="flatpak run com.fugro.roames.displaz"`

## Bundles

1. Create a bundle from a repo
   `$ flatpak build-bundle repo displaz.flatpak com.fugro.roames.displaz`
2. Install a bundle
   `$ flatpak install --user displaz.flatpak`

## AppData file

* `$ appstream-util validate-relax com.fugro.roames.displaz.appdata.xml`