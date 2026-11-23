#!/bin/sh
set -e

GAME="${1:-Lad}"
TARGET="${2:-$GAME}"
VERSION="${3:-0.1}"
APP_NAME="${APP_NAME:-Muzremont90}"
DISPLAY_NAME="${DISPLAY_NAME:-Музремонт 90-х}"
BUNDLE_ID="${BUNDLE_ID:-local.lad.muzremont90}"
PRESET="clang-release-macos"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="$ROOT/Build/$PRESET"
DIST="$ROOT/dist"
APP="$DIST/$APP_NAME.app"
GAME_ASSETS="$ROOT/OpenGLEngine/Game/$GAME/Assets"

cd "$ROOT"
cmake --preset "$PRESET"
cmake --build --preset "$PRESET" --target "$TARGET" -j 8

rm -rf "$APP"
mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Resources/Assets"
cp "$BUILD/$TARGET" "$APP/Contents/MacOS/$APP_NAME"
for dir in Shaders Fonts Scenes Sounds; do
    if [ -d "$BUILD/Assets/$dir" ]; then
        mkdir -p "$(dirname "$APP/Contents/Resources/Assets/$dir")"
        cp -R "$BUILD/Assets/$dir" "$APP/Contents/Resources/Assets/$dir"
    fi
done
if [ -f "$BUILD/Assets/CREDITS.md" ]; then
    cp "$BUILD/Assets/CREDITS.md" "$APP/Contents/Resources/Assets/"
fi
if [ -d "$GAME_ASSETS" ]; then
    cp -R "$GAME_ASSETS/." "$APP/Contents/Resources/Assets/"
fi
echo "saves and settings live in ~/Library/Application Support/Muzremont90" > "$APP/Contents/Resources/user_data.txt"

find "$BUILD" -maxdepth 1 -name "*.dylib" -exec cp {} "$APP/Contents/MacOS/" \;
install_name_tool -add_rpath "@executable_path" "$APP/Contents/MacOS/$APP_NAME" 2>/dev/null || true

cat > "$APP/Contents/Info.plist" <<PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleName</key><string>$DISPLAY_NAME</string>
    <key>CFBundleDisplayName</key><string>$DISPLAY_NAME</string>
    <key>CFBundleIdentifier</key><string>$BUNDLE_ID</string>
    <key>CFBundleVersion</key><string>$VERSION</string>
    <key>CFBundleShortVersionString</key><string>$VERSION</string>
    <key>CFBundleExecutable</key><string>$APP_NAME</string>
    <key>CFBundlePackageType</key><string>APPL</string>
    <key>CFBundleDevelopmentRegion</key><string>ru</string>
    <key>LSMinimumSystemVersion</key><string>11.0</string>
    <key>LSApplicationCategoryType</key><string>public.app-category.simulation-games</string>
    <key>NSHighResolutionCapable</key><true/>
</dict>
</plist>
PLIST

if [ -n "$DEVELOPER_ID" ]; then
    codesign --force --deep --options runtime --timestamp --sign "$DEVELOPER_ID" "$APP"
else
    codesign --force --deep --sign - "$APP" || true
fi

cd "$DIST"
rm -f "$APP_NAME-$VERSION-macOS.zip"
ditto -c -k --keepParent "$APP_NAME.app" "$APP_NAME-$VERSION-macOS.zip"

STAGE="$DIST/dmg-$APP_NAME"
DMG="$DIST/$APP_NAME-$VERSION.dmg"
rm -rf "$STAGE" "$DMG"
mkdir -p "$STAGE"
cp -R "$APP" "$STAGE/"
ln -s /Applications "$STAGE/Applications"
hdiutil create -volname "$DISPLAY_NAME" -srcfolder "$STAGE" -ov -format UDZO "$DMG"
rm -rf "$STAGE"

if [ -n "$DEVELOPER_ID" ] && [ -n "$NOTARY_PROFILE" ]; then
    codesign --force --timestamp --sign "$DEVELOPER_ID" "$DMG"
    xcrun notarytool submit "$DMG" --keychain-profile "$NOTARY_PROFILE" --wait
    xcrun stapler staple "$DMG"
fi

echo "macOS app: $APP"
echo "macOS installer: $DMG"
