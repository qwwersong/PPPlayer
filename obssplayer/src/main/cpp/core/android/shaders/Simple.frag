const char* SimpleFragmentShader = STRINGIFY(
varying mediump vec2 TextureCoordOut;
varying lowp vec4 DestinationColor;
uniform sampler2D SamplerY;
uniform sampler2D SamplerU;
uniform sampler2D SamplerV;

const mediump mat3 yuv2rgb = mat3(
							1.0,	1.0,	1.0,
							0,		-0.391, 2.018,
							1.596,	-0.813, 0
                            );
const mediump vec3 lumCoeff = vec3(0.2125,0.7154,0.0721);
														
void main(void)
{
    mediump vec3 yuv = vec3(
                    1.1643 * (texture2D(SamplerY, TextureCoordOut).r - 0.0627),
                    (texture2D(SamplerU, TextureCoordOut).r - 0.502),
                    (texture2D(SamplerV, TextureCoordOut).r - 0.502)
                     );
					 

    mediump vec3 rgb = yuv2rgb*yuv;
    mediump vec3 intensity = vec3(dot(rgb,lumCoeff));
    mediump vec3 color = mix(intensity,rgb.rgb,1.1);
    color = clamp(color,0.0,255.0);

    gl_FragColor = vec4(color, 1.0);
}
);