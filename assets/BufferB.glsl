uniform float g_camPitch;
uniform float g_camYaw;
uniform float g_camRoll;
uniform float g_forceCamAngles;
uniform int g_forceCamPos;
uniform vec3 g_overridePos;
uniform vec3 g_overrideFwd;
uniform vec3 g_overrideRight;
uniform vec3 g_overrideUp;


#ifndef iSpin
#define iSpin 0.997114514   
#endif
#define CONST_M (0.5 * iBhSize)

const int KEY_W = 87;
const int KEY_A = 65;
const int KEY_S = 83;
const int KEY_D = 68;
const int KEY_Q = 81;
const int KEY_E = 69;
const int KEY_R = 82;
const int KEY_F = 70;


// const float g_moveSpeed = 2.5; 
const float MOUSE_SENSITIVITY = 0.003;
const float ROLL_SPEED = 2.0;


bool isKeyPressed(int key) {
    return texelFetch(iChannel3, ivec2(key, 0), 0).x > 0.5;
}


mat3 rotAxis(vec3 axis, float angle) {
    float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0 - c;
    return mat3(
        oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,
        oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,
        oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c
    );
}





vec3 ColorFetch(vec2 coord)
{
 	return texture(iChannel0, coord).rgb;   
}

vec3 Grab1(vec2 coord, const float octave, const vec2 offset)
{
 	float scale = exp2(octave);
    coord += offset;
    coord *= scale;
   	if (coord.x < 0.0 || coord.x > 1.0 || coord.y < 0.0 || coord.y > 1.0) return vec3(0.0);   
    return ColorFetch(coord);
}

vec3 Grab4(vec2 coord, const float octave, const vec2 offset)
{
 	float scale = exp2(octave);
    coord += offset;
    coord *= scale;
   	if (coord.x < 0.0 || coord.x > 1.0 || coord.y < 0.0 || coord.y > 1.0) return vec3(0.0);   
    
    vec3 color = vec3(0.0);
    float weights = 0.0;
    const int oversampling = 4;
    for (int i = 0; i < oversampling; i++) {    	    
        for (int j = 0; j < oversampling; j++) {
			vec2 off = (vec2(i, j) / iResolution.xy + vec2(-float(oversampling)*0.5) / iResolution.xy) * scale / float(oversampling);
            color += ColorFetch(coord + off);
            weights += 1.0;
        }
    }
    return color / weights;
}

vec3 Grab8(vec2 coord, const float octave, const vec2 offset)
{
 	float scale = exp2(octave);
    coord += offset;
    coord *= scale;
   	if (coord.x < 0.0 || coord.x > 1.0 || coord.y < 0.0 || coord.y > 1.0) return vec3(0.0);   
    
    vec3 color = vec3(0.0);
    float weights = 0.0;
    const int oversampling = 8;
    for (int i = 0; i < oversampling; i++) {    	    
        for (int j = 0; j < oversampling; j++) {
			vec2 off = (vec2(i, j) / iResolution.xy + vec2(-float(oversampling)*0.5) / iResolution.xy) * scale / float(oversampling);
            color += ColorFetch(coord + off);
            weights += 1.0;
        }
    }
    return color / weights;
}

vec3 Grab16(vec2 coord, const float octave, const vec2 offset)
{
 	float scale = exp2(octave);
    coord += offset;
    coord *= scale;
   	if (coord.x < 0.0 || coord.x > 1.0 || coord.y < 0.0 || coord.y > 1.0) return vec3(0.0);   
    
    vec3 color = vec3(0.0);
    float weights = 0.0;
    const int oversampling = 16;
    for (int i = 0; i < oversampling; i++) {    	    
        for (int j = 0; j < oversampling; j++) {
			vec2 off = (vec2(i, j) / iResolution.xy + vec2(-float(oversampling)*0.5) / iResolution.xy) * scale / float(oversampling);
            color += ColorFetch(coord + off);
            weights += 1.0;
        }
    }
    return color / weights;
}

vec2 CalcOffset(float octave)
{
    vec2 offset = vec2(0.0);
    vec2 padding = vec2(10.0) / iResolution.xy;
    offset.x = -min(1.0, floor(octave / 3.0)) * (0.25 + padding.x);
    offset.y = -(1.0 - (1.0 / exp2(octave))) - padding.y * octave;
	offset.y += min(1.0, floor(octave / 3.0)) * 0.35;
 	return offset;   
}





#define OFFSET_UP    1  
#define OFFSET_RIGHT 2  
#define OFFSET_POS   3  
#define OFFSET_FWD   4  
#define OFFSET_MOUSE 5  
#define OFFSET_TIME  6  

void UpdateCameraState(out vec4 fragColor, in vec2 fragCoord)
{
    int pxIndex = int(iResolution.x) - int(fragCoord.x);
    int width = int(iResolution.x);
    vec3  up      = texelFetch(iChannel1, ivec2(width - OFFSET_UP, 0), 0).xyz;
    vec3  right   = texelFetch(iChannel1, ivec2(width - OFFSET_RIGHT, 0), 0).xyz;
    vec3  pos     = texelFetch(iChannel1, ivec2(width - OFFSET_POS, 0), 0).xyz;
    vec3  fwd     = texelFetch(iChannel1, ivec2(width - OFFSET_FWD, 0), 0).xyz;
    vec4  lastMouse = texelFetch(iChannel1, ivec2(width - OFFSET_MOUSE, 0), 0);
    vec4  timeData = texelFetch(iChannel1, ivec2(width - OFFSET_TIME, 0), 0);
    float gTime   = timeData.x;
    float uniSign = timeData.y; 
    vec3 oldPos = pos; 
    if (iFrame <= 5 || length(fwd) < 0.1) {
        pos = vec3(-2.0, -3.6, 22.0); 
        fwd = vec3(0.0, 0.15, -1.0);
        fwd = normalize(fwd);
        right = normalize(cross(fwd, vec3(-0.5, 1.0, 0.0)));
        up    = normalize(cross(right, fwd));
        gTime = 0.0;
        lastMouse = iMouse;
        uniSign = 1.0; 
    }

    
    if (iMouse.z > 0.0) {
        vec2 mouseDelta = iMouse.xy - lastMouse.xy;
        
        
        if (lastMouse.z < 0.0) mouseDelta = vec2(0.0);
        
        float yaw = -mouseDelta.x * MOUSE_SENSITIVITY;
        float pitch = mouseDelta.y * MOUSE_SENSITIVITY;
        
        
        fwd = rotAxis(up, yaw) * fwd;
        right = rotAxis(up, yaw) * right;
        
        
        fwd = rotAxis(right, pitch) * fwd;
        
        
        up = normalize(cross(right, fwd));
        right = normalize(cross(fwd, up));
    }
    
    
    float roll = 0.0;
    if (isKeyPressed(KEY_Q)) roll -= ROLL_SPEED * iTimeDelta;
    if (isKeyPressed(KEY_E)) roll += ROLL_SPEED * iTimeDelta;
    
    if (roll != 0.0) {
        right = rotAxis(fwd, roll) * right;
        up = normalize(cross(right, fwd));
    }

    
    vec3 moveDir = vec3(0.0);
    if (isKeyPressed(KEY_W)) moveDir += fwd;
    if (isKeyPressed(KEY_S)) moveDir -= fwd;
    if (isKeyPressed(KEY_A)) moveDir -= right;
    if (isKeyPressed(KEY_D)) moveDir += right;
    if (isKeyPressed(265)) moveDir += fwd;
    if (isKeyPressed(264)) moveDir -= fwd;
    if (isKeyPressed(263)) moveDir -= right;
    if (isKeyPressed(262)) moveDir += right;

    if (isKeyPressed(KEY_R)) moveDir += up; 
    if (isKeyPressed(KEY_F)) moveDir -= up; 

    pos += moveDir * g_moveSpeed * iTimeDelta * (length(pos) > 3.0 ? 1.0 : (length(pos) > 0.5 ? 0.1 + 0.9 * (length(pos) - 0.5) / 2.5 : 0.1));
    
    
    if (g_forceCamAngles > 0.5) {
        float p = radians(g_camPitch);
        float y = radians(g_camYaw);
        float r = radians(g_camRoll);
        fwd = vec3(0.0, 0.0, -1.0);
        right = vec3(1.0, 0.0, 0.0);
        up = vec3(0.0, 1.0, 0.0);
        fwd = rotAxis(right, p) * fwd;
        up = rotAxis(right, p) * up;
        fwd = rotAxis(vec3(0.0, 1.0, 0.0), y) * fwd;
        right = rotAxis(vec3(0.0, 1.0, 0.0), y) * right;
        up = rotAxis(vec3(0.0, 1.0, 0.0), y) * up;
        right = rotAxis(fwd, r) * right;
        up = normalize(cross(right, fwd));
    }
    
    if (g_forceCamPos == 1) {
        pos = g_overridePos;
        fwd = normalize(g_overrideFwd);
        right = normalize(g_overrideRight);
        up = normalize(g_overrideUp);
    }

    float spinRadius = abs(iSpin * CONST_M);
    if (oldPos.y * pos.y < 0.0) 
    {
        float t = oldPos.y / (oldPos.y - pos.y);
        vec3 crossPoint = mix(oldPos, pos, t);
        
        if (length(crossPoint.xz) < spinRadius) {
            uniSign *= -1.0;
        }
    }
    
    gTime += iTimeDelta;

    
    fragColor = vec4(0.0);
    
    if (pxIndex == OFFSET_UP)    fragColor = vec4(up, 1.0);     
    if (pxIndex == OFFSET_RIGHT) fragColor = vec4(right, 1.0);  
    if (pxIndex == OFFSET_POS)   fragColor = vec4(pos, 1.0);    
    if (pxIndex == OFFSET_FWD)   fragColor = vec4(fwd, 1.0);    
    if (pxIndex == OFFSET_MOUSE) fragColor = iMouse;            
    if (pxIndex == OFFSET_TIME)  fragColor = vec4(gTime, 0.0, 0.0, 1.0); 
    if (pxIndex == OFFSET_TIME)  fragColor = vec4(gTime, uniSign, 0.0, 1.0); 
}





void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    
    
    bool isDataPixel = (fragCoord.y < 1.0 && fragCoord.x > (iResolution.x - 8.5));

    if (isDataPixel) {
        UpdateCameraState(fragColor, fragCoord);
    } else {
        
        vec2 uv = fragCoord.xy / iResolution.xy;
        vec3 color = vec3(0.0);
        
        color += Grab1(uv, 1.0, vec2(0.0,  0.0)   );
        color += Grab4(uv, 2.0, vec2(CalcOffset(1.0))   );
        color += Grab8(uv, 3.0, vec2(CalcOffset(2.0))   );
        color += Grab16(uv, 4.0, vec2(CalcOffset(3.0))   );
        color += Grab16(uv, 5.0, vec2(CalcOffset(4.0))   );
        color += Grab16(uv, 6.0, vec2(CalcOffset(5.0))   );
        color += Grab16(uv, 7.0, vec2(CalcOffset(6.0))   );
        color += Grab16(uv, 8.0, vec2(CalcOffset(7.0))   );

        fragColor = vec4(color, 1.0);
    }
}
