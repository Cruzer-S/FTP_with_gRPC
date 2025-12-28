mkdir -p Resources

if [[ -f "Resources/image_backup.iso" ]]; then
	cp Resources/image_backup.iso Resources/image.iso
else
	wget https://download.fedoraproject.org/pub/fedora/linux/releases/43/Workstation/x86_64/iso/Fedora-Workstation-Live-43-1.6.x86_64.iso -O Resources/image.iso
	cp Resources/image.iso Resources/image_backup.iso
fi
