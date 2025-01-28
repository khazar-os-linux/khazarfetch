# Maintainer: <Azad Zeynalov>
pkgname=khazarfetch
pkgver=2.0
pkgrel=2
pkgdesc="Minimal fetch tool"
arch=('x86_64')
url="https://github.com/khazar-os-linux/khazarfetch"
license=('MIT')
source=("${pkgname}.cpp")
sha256sums=('SKIP')

build() {
    cd "$srcdir"
    g++ -o "$pkgname" "$pkgname.cpp"
}

package() {
    install -Dm755 "$srcdir/$pkgname" "$pkgdir/usr/bin/$pkgname"
    chmod 755 "$pkgdir/usr/bin/$pkgname"
}

