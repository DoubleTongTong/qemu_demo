# qemu_demo
qemu-system-x86_64 -m 2048 -hda ubuntu-server.img -boot c -net user,hostfwd=tcp::2222-:22 -net nic -device my-pci-device

