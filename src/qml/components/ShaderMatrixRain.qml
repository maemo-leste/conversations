import QtQuick 2.0
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.0
import QtGraphicalEffects 1.15

Item {
    id: root

    property real iTime: 0

    // texture sizes
    property var iChannel0Size: Qt.size(512,512)
    property var iChannel1Size: Qt.size(256,256)

    Timer {
        interval: 50
        running: true
        repeat: true
        onTriggered: root.iTime += 0.050
    }

    Image { id: iChannel0Image; source: "qrc:/ichannel0_small.png"; visible: false }
    Image { id: iChannel1Image; source: "qrc:/ichannel1.png"; visible: false }

    ShaderEffect {
        id: shader
        anchors.fill: parent

        property real iTime: root.iTime
        property variant iChannel0: ShaderEffectSource { sourceItem: iChannel0Image; wrapMode: ShaderEffectSource.Repeat }
        property variant iChannel1: ShaderEffectSource { sourceItem: iChannel1Image; wrapMode: ShaderEffectSource.Repeat }

        property var iResolution: Qt.point(width, height)
        property var iChannel0Resolution: Qt.point(root.iChannel0Size.width, root.iChannel0Size.height)
        property var iChannel1Resolution: Qt.point(root.iChannel1Size.width, root.iChannel1Size.height)

        property real speedFactor: 0.5
        property real trailLength: 10.0
        property real falloffPower: 0.5

        fragmentShader: "
            uniform lowp float qt_Opacity;
            uniform sampler2D iChannel0;
            uniform sampler2D iChannel1;
            uniform highp float iTime;
            uniform highp vec2 iResolution;
            uniform highp vec2 iChannel0Resolution;
            uniform highp vec2 iChannel1Resolution;
            uniform highp float trailLength;
            uniform highp float falloffPower;
            uniform highp float speedFactor;

            lowp float text(highp vec2 fragCoord) {
                highp vec2 uv = mod(fragCoord.xy, 16.0) * 0.0625;
                highp vec2 block = fragCoord * 0.0625 - uv;
                uv = uv * 0.8 + 0.1;

                highp vec2 rnd = texture2D(iChannel1, block / iChannel1Resolution + iTime * 0.002).xy * 16.0;
                uv += floor(rnd);
                uv *= 0.0625;
                uv.x = -uv.x;

                return texture2D(iChannel0, uv).r;
            }

            lowp vec3 rain(highp vec2 fragCoord)
            {
                fragCoord.x -= mod(fragCoord.x, 16.0);

                highp float offset = sin(fragCoord.x * 15.0);
                highp float speed  = cos(fragCoord.x * 3.0) * 0.3 + 0.7;

                highp float y = fract(fragCoord.y / iResolution.y + iTime * speed * speedFactor + offset);

                // Basic trail brightness
                highp float intensity = pow(1.0 - y, falloffPower) / (y * trailLength);

                // green tint
                return vec3(0.1, 1.0, 0.35) * intensity;
            }

            void main() {
                highp vec2 fragCoord = gl_FragCoord.xy;
                lowp float t = text(fragCoord);
                lowp vec3 r = rain(fragCoord);
                gl_FragColor = vec4(t * r, 1.0) * qt_Opacity;
            }
        "
    }
}