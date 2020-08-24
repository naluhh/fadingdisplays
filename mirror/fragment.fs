
#define NUM_OCTAVES 5

const vec3 color = vec3(0, 0.745, 0.9);


// Get random value
float random(in vec2 st)
{
    return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

// Get noise
float noise(in vec2 st)
{
    // Splited integer and float values.
    vec2 i = floor(st);
    vec2 f = fract(st);
    
    float a = random(i + vec2(0.0, 0.0));
    float b = random(i + vec2(1.0, 0.0));
    float c = random(i + vec2(0.0, 1.0));
    float d = random(i + vec2(1.0, 1.0));
    
    // -2.0f^3 + 3.0f^2
    vec2 u = f * f * (3.0 - 2.0 * f);
    
    return mix(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}

// fractional brown motion
//
// Reduce amplitude multiplied by 0.5, and frequency multiplied by 2.
float fbm(in vec2 st)
{
    float v = 0.0;
    float a = 0.5;
    
    for (int i = 0; i < NUM_OCTAVES; i++)
    {
        v += a * noise(st);
        st = st * 2.0;
        a *= 0.5;
    }
    
    return v;
}


void main2(void)
{
    // Calculate normalized UV values.
    vec2 st = gl_FragCoord.xy / resolution.xy;
        
    vec2 q = vec2(0.0);
    q.x = fbm(st + vec2(0.0));
    q.y = fbm(st + vec2(1.0));
    
    // These numbers(such as 1.7, 9.2, etc.) are not special meaning.
    vec2 r = vec2(0.0);
    r.x = fbm(st + (1.0 * q) + vec2(1.7, 9.2) + (0.15 * time));
    r.y = fbm(st + (1.0 * q) + vec2(8.3, 2.8) + (0.12 * time));
    
    // Calculate 'r' is that getting domain warping.
    float f = fbm(st + r);
    
    // f^3 + 0.6f^2 + 0.5f
    float coef = (f * f * f + (0.6 * f * f) + (0.5 * f));
    
    gl_FragColor = vec4(coef * color, 1.0);
}

//    gl_FragColor = frag_color * texture2D(texture0, tex_coord0);

void main(void) {
    vec2 uv = -1.0 + 2.0*gl_FragCoord.xy / resolution.xy;
    uv.x *=  resolution.x / resolution.y;
    vec3 color = vec3(0.0);
    for( int i=0; i<128; i++ )
    {
      float pha =      sin(float(i)*546.13+1.0)*0.5 + 0.5;
      float siz = pow( sin(float(i)*651.74+5.0)*0.5 + 0.5, 4.0 );
      float pox =      sin(float(i)*321.55+4.1) * resolution.x / resolution.y;
      float rad = 0.1+0.5*siz+sin(pha+siz)/4.0;
      vec2  pos = vec2( pox+sin(time/15.+pha+siz), -1.0-rad + (2.0+2.0*rad)*mod(pha+0.3*(time/7.)*(0.2+0.8*siz),1.0));
      float dis = length( uv - pos );
      vec3  col = mix( vec3(0.194*sin(time/6.0)+0.3,0.2,0.3*pha), vec3(1.1*sin(time/9.0)+0.3,0.2*pha,0.4), 0.5+0.5*sin(float(i)));
      float f = length(uv-pos)/rad;
      f = sqrt(clamp(1.0+(sin((time)*siz)*0.5)*f,0.0,1.0));
      color += col.zyx *(1.0-smoothstep( rad*0.15, rad, dis ));
    }
    color *= sqrt(1.5-0.5*length(uv));
    gl_FragColor = vec4(color, 0.5);
}
