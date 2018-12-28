# Flatpak builds of displaz

## HOWTO

1. Build the flatpak
   `$ flatpak-builder --force-clean tmp com.fugro.roames.displaz.json`
2. Build the flatpak to to a repository
   `$ flatpak-builder --repo=repo --force-clean tmp com.fugro.roames.displaz.json`
3. Install from repository
   `$ flatpak --user install repo com.fugro.roames.displaz`
4. Run
   `$ flatpak run com.fugro.roames.displaz`
