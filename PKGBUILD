# Maintainer: <Azad Zeynalov>
pkgname=khazarfetch
pkgver=0.1.5
pkgrel=1
pkgdesc="Minimal fetch tool for Khazar OS"
arch=('x86_64')
url="https://github.com/khazar-os-linux/khazarfetch"
license=('GPL-v3.0')
source=("git+${url}.git")
sha256sums=('SKIP')
depends=(
)
optdepends=(
  'xorg-xrandr: X11 resolution & refresh rate detection'
  'wlr-randr: Wayland resolution detection'
  'kscreen-doctor: KDE Wayland resolution detection'
  'hyprctl: Hyprland resolution & refresh rate detection'
  'inetutils: hostname fallback'
  'nerd-fonts: icon glyphs support'
)
makedepends=(
  'gcc'
  'make'
)

pkgver() {
  cd "$srcdir/$pkgname"
  git describe --tags --abbrev=0 2>/dev/null || git rev-parse --short HEAD
}

build() {
  cd "$srcdir/$pkgname"
  make
}

package() {
  cd "$srcdir/$pkgname"
  make DESTDIR="$pkgdir" install
}
