#!/bin/sh
set -e

GAME="${1:-Lad}"
TARGET="${2:-$GAME}"
VERSION="${3:-0.1}"
APP_NAME="${APP_NAME:-Muzremont90}"
DISPLAY_NAME="${DISPLAY_NAME:-Музремонт 90-х}"
PRESET="clang-release-linux"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="$ROOT/Build/$PRESET"
DIST="$ROOT/dist"
OUT="$DIST/$APP_NAME-linux"
GAME_DIR="$ROOT/OpenGLEngine/Game/$GAME"

cd "$ROOT"
cmake --preset "$PRESET"
cmake --build --preset "$PRESET" --target "$TARGET" -j 8

rm -rf "$OUT"
mkdir -p "$OUT/Assets"
cp "$BUILD/$TARGET" "$OUT/$APP_NAME"
chmod +x "$OUT/$APP_NAME"
for dir in Shaders Fonts Scenes Sounds; do
    if [ -d "$BUILD/Assets/$dir" ]; then
        mkdir -p "$(dirname "$OUT/Assets/$dir")"
        cp -R "$BUILD/Assets/$dir" "$OUT/Assets/$dir"
    fi
done
if [ -f "$BUILD/Assets/CREDITS.md" ]; then
    cp "$BUILD/Assets/CREDITS.md" "$OUT/Assets/"
fi
if [ -d "$GAME_DIR/Assets" ]; then
    cp -R "$GAME_DIR/Assets/." "$OUT/Assets/"
fi
echo "saves and settings live in ~/.local/share/Muzremont90" > "$OUT/user_data.txt"
find "$BUILD" -maxdepth 1 -name "*.so*" -exec cp {} "$OUT/" \;

cd "$DIST"
rm -f "$APP_NAME-$VERSION-linux.tar.gz"
tar -czf "$APP_NAME-$VERSION-linux.tar.gz" "$APP_NAME-linux"
echo "Linux archive: $DIST/$APP_NAME-$VERSION-linux.tar.gz"

APPIMAGETOOL="$(command -v appimagetool || true)"
if [ -z "$APPIMAGETOOL" ]; then
    echo "appimagetool not found: only the archive was made"
    exit 0
fi
APPDIR="$DIST/$APP_NAME.AppDir"
rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr"
cp -R "$OUT" "$APPDIR/usr/game"
cat > "$APPDIR/AppRun" <<RUN
#!/bin/sh
HERE="\$(dirname "\$(readlink -f "\$0")")"
cd "\$HERE/usr/game"
exec "\$HERE/usr/game/$APP_NAME" "\$@"
RUN
chmod +x "$APPDIR/AppRun"
cat > "$APPDIR/$APP_NAME.desktop" <<DESKTOP
[Desktop Entry]
Type=Application
Name=$DISPLAY_NAME
Exec=$APP_NAME
Icon=$APP_NAME
Categories=Game;Simulation;
DESKTOP
if [ -f "$GAME_DIR/Installer/icon.png" ]; then
    cp "$GAME_DIR/Installer/icon.png" "$APPDIR/$APP_NAME.png"
fi
ARCH=x86_64 "$APPIMAGETOOL" "$APPDIR" "$DIST/$APP_NAME-$VERSION-x86_64.AppImage"
rm -rf "$APPDIR"
echo "Linux AppImage: $DIST/$APP_NAME-$VERSION-x86_64.AppImage"
