pragma Singleton
import QtQuick 2.15

QtObject {
    readonly property color background: "#0F172A"
    readonly property color surface: "#1B2336"
    readonly property color surfaceRaised: "#272F42"
    readonly property color surfaceHover: "#2E3850"
    readonly property color borderSubtle: "#273248"
    readonly property color border: "#475569"

    readonly property color textPrimary: "#F8FAFC"
    readonly property color textSecondary: "#94A3B8"
    readonly property color textMuted: "#64748B"

    readonly property color accent: "#22C55E"
    readonly property color accentForeground: "#0F172A"
    readonly property color warning: "#F59E0B"
    readonly property color danger: "#EF4444"

    readonly property color rollColor: "#60A5FA"
    readonly property color pitchColor: "#F472B6"
    readonly property color yawColor: "#FBBF24"
    readonly property color axisXColor: "#F87171"
    readonly property color axisYColor: "#34D399"
    readonly property color axisZColor: "#60A5FA"

    readonly property int spacingXs: 4
    readonly property int spacingSm: 8
    readonly property int spacingMd: 12
    readonly property int spacingLg: 16
    readonly property int spacingXl: 24

    readonly property int radiusSm: 6
    readonly property int radiusMd: 10
    readonly property int radiusLg: 14

    readonly property string fontFamily: "Fira Sans"
    readonly property string monoFontFamily: "Fira Mono"
    readonly property int fontSizeCaption: 12
    readonly property int fontSizeBody: 14
    readonly property int fontSizeSubtitle: 16
    readonly property int fontSizeTitle: 22
    readonly property int fontSizeReadout: 30

    readonly property int animationFast: 120
    readonly property int animationNormal: 200

    readonly property int navigationRailWidth: 96
    readonly property int controlHeight: 38
}
