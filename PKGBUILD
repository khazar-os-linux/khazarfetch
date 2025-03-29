# Maintainer: <Azad Zeynalov>
pkgname=khazarfetch
pkgver=2.5
pkgrel=2
pkgdesc="Minimal fetch tool"
arch=('x86_64')
url="https://github.com/khazar-os-linux/khazarfetch"
license=('GPL-v3.0')
source=("${pkgname}.cpp")
sha256sums=('SKIP')
depends=(
  'inetutils'
  'usbutils'
  'xorg-xrandr'
)

pkgver() {
  cd "$srcdir/$pkgname"
  local count hash
  count=$(git rev-list --count HEAD)
  hash=$(git rev-parse --short HEAD)
  echo "r${count}.${hash}"
}

build() {
    cd "$srcdir"
    g++ -o "$pkgname" "$pkgname.cpp"
}

package() {
    install -Dm755 "$srcdir/$pkgname" "$pkgdir/usr/bin/$pkgname"
    chmod 755 "$pkgdir/usr/bin/$pkgname"
}

