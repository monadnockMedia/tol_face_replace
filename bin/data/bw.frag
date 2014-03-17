uniform sampler2DRect tex;
varying vec2 texCoordVarying;
void main()
{
	//Multiply color by texture
	vec4 color =  texture2DRect(tex, texCoordVarying);
	float gray = dot(color.rgb, vec3(0.299, 0.587, 0.314));
    gl_FragColor = vec4(gray, gray, gray, gl_Color.a);
}