#!/bin/sh
set -e

GAME="${1:-Lad}"
VERSION="${2:-0.1}"
APP_NAME="Muzremont90"
PRESET="clang-release-macos"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="$ROOT/Build/$PRESET"
DIST="$ROOT/dist"
APP="$DIST/$APP_NAME.app"

cd "$ROOT"
cmake --preset "$PRESET"
cmake --build --preset "$PRESET" --target "$GAME" -j 8

rm -rf "$APP"
mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Resources/Assets"
cp "$BUILD/$GAME" "$APP/Contents/MacOS/$APP_NAME"
for dir in Shaders Fonts Scenes; do
    if [ -d "$BUILD/Assets/$dir" ]; then
        cp -R "$BUILD/Assets/$dir" "$APP/Contents/Resources/Assets/$dir"
    fi
done
find "$BUILD" -maxdepth 1 -name "*.dylib" -exec cp {} "$APP/Contents/MacOS/" \;

cat > "$APP/Contents/Info.plist" <<PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleName</key><string>Музремонт 90-х</string>
    <key>CFBundleDisplayName</key><string>Музремонт 90-х</string>
    <key>CFBundleIdentifier</key><string>local.lad.muzremont90</string>
    <key>CFBundleVersion</key><string>$VERSION</string>
    <key>CFBundleShortVersionString</key><string>$VERSION</string>
    <key>CFBundleExecutable</key><string>$APP_NAME</string>
    <key>CFBundlePackageType</key><string>APPL</string>
    <key>LSMinimumSystemVersion</key><string>11.0</string>
    <key>NSHighResolutionCapable</key><true/>
</dict>
</plist>
PLIST

codesign --force --deep --sign - "$APP" || true
cd "$DIST"
rm -f "$APP_NAME-$VERSION-macOS.zip"
ditto -c -k --keepParent "$APP_NAME.app" "$APP_NAME-$VERSION-macOS.zip"
echo "macOS build: $DIST/$APP_NAME-$VERSION-macOS.zip"
