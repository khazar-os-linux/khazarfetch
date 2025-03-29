# Maintainer: <Azad Zeynalov>
pkgname=khazarfetch
pkgver=2.5
pkgrel=2
pkgdesc="Minimal fetch tool"
arch=('x86_64')
url="https://github.com/khazar-os-linux/khazarfetch"
license=('GPL-v3.0')
source=()
depends=(
  'inetutils'
  'usbutils'
  'xorg-xrandr'
)

pkgver() {
    cd "$srcdir"
    git describe --tags --abbrev=0 2>/dev/null || git rev-parse --short HEAD
}

prepare() {
    make
}

package() {
    make DESTDIR="$pkgdir" install
}
