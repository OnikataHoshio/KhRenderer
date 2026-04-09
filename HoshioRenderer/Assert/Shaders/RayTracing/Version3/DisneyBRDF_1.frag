#version 460 core
out vec4 FragColor;

in vec3 CanvasPos; 

struct Primitive{
    vec4 P1, P2, P3;
    vec4 N1, N2, N3;
    ivec2 PrimitiveType;
    ivec2 MaterialSlot;
};

struct EncodedBRDFMaterial{
	vec4 Emissive;  
	vec4 BaseColor;
    vec4 Param1;
    vec4 Param2;
    vec4 Param3;
};

struct BRDFMaterial{
	vec3 Emissive; 
	vec3 BaseColor;
	float Subsurface;
	float Metallic;
	float Specular;
	float SpecularTint;
	float Roughness;
	float Anisotropic;
	float Sheen;
	float SheenTint;
	float Clearcoat;
	float ClearcoatGloss;
};

struct LBVHNode{
    ivec4 Param1;
    ivec4 Param2;
    vec4 AABB_MinPos;
    vec4 AABB_MaxPos;
};

layout(std430, binding = 0) buffer PrimitiveSSBO { Primitive Primitives[]; };
layout(std430, binding = 1) buffer Morton3DBuffer { uvec2 SortedMorton3D[]; };
layout(std430, binding = 2) buffer LBVHNodeBuffer { LBVHNode LBVHNodes[]; };
layout(std430, binding = 3) buffer AuxiliaryBuffer { ivec4 Root; };
layout(std430, binding = 4) buffer EncodedBRDFMaterialSSBO{ EncodedBRDFMaterial Materials[]; };

layout(std140, binding = 5) uniform CameraBlock {
    vec4 AspectAndFovy; // x: Aspect, y: Fovy
    vec4 Position;
    vec4 Right;
    vec4 Up;
    vec4 Front;
} UCameraParam;


uniform sampler2D uLastFrame;
uniform sampler2D uSkybox;
uniform int uLBVHNodeCount;
uniform uint uFrameCounter;
uniform uvec2 uResolution; 

uniform int uEnableSobol;
uniform int uEnableDiffuseIS;
uniform int uEnableSpecularIS;
uniform int uEnableClearcoatIS;


const vec3 SkyColor = vec3(0.05, 0.05, 0.05);

struct Ray
{
	vec3 Start;
	vec3 Direction;
};

struct HitResult {
	bool bIsHit;
    // bool BIsInside;
	float Distance;
	vec3 HitPoint;
	vec3 GeoNormal;
    vec3 ShadeNormal;
    int MaterialSlot;
};

#define INF             114514.0
#define EPS             1e-8
#define PI              3.14159265358979323846

uint rngState;

const uint V[8*32] = {
	2147483648, 1073741824, 536870912, 268435456, 134217728, 67108864, 33554432, 
	16777216, 8388608, 4194304, 2097152, 1048576, 524288, 262144, 131072, 65536, 
	32768, 16384, 8192, 4096, 2048, 1024, 512, 256, 128, 64, 32, 16, 8, 4, 2, 1,
	2147483648, 3221225472, 2684354560, 4026531840, 2281701376, 3422552064, 
	2852126720, 4278190080, 2155872256, 3233808384, 2694840320, 4042260480, 
	2290614272, 3435921408, 2863267840, 4294901760, 2147516416, 3221274624, 
	2684395520, 4026593280, 2281736192, 3422604288, 2852170240, 4278255360, 
	2155905152, 3233857728, 2694881440, 4042322160, 2290649224, 3435973836, 
	2863311530, 4294967295,
	2147483648, 3221225472, 1610612736, 2415919104, 3892314112, 1543503872, 
	2382364672, 3305111552, 1753219072, 2629828608, 3999268864, 1435500544, 
	2154299392, 3231449088, 1626210304, 2421489664, 3900735488, 1556135936, 
	2388680704, 3314585600, 1751705600, 2627492864, 4008611328, 1431684352, 
	2147543168, 3221249216, 1610649184, 2415969680, 3892340840, 1543543964, 
	2382425838, 3305133397,
	2147483648, 3221225472, 536870912, 1342177280, 4160749568, 1946157056, 
	2717908992, 2466250752, 3632267264, 624951296, 1507852288, 3872391168, 
	2013790208, 3020685312, 2181169152, 3271884800, 546275328, 1363623936, 
	4226424832, 1977167872, 2693105664, 2437829632, 3689389568, 635137280, 
	1484783744, 3846176960, 2044723232, 3067084880, 2148008184, 3222012020, 
	537002146, 1342505107,
	2147483648, 1073741824, 536870912, 2952790016, 4160749568, 3690987520, 
	2046820352, 2634022912, 1518338048, 801112064, 2707423232, 4038066176, 
	3666345984, 1875116032, 2170683392, 1085997056, 579305472, 3016343552, 
	4217741312, 3719483392, 2013407232, 2617981952, 1510979072, 755882752, 
	2726789248, 4090085440, 3680870432, 1840435376, 2147625208, 1074478300, 
	537900666, 2953698205,
	2147483648, 1073741824, 1610612736, 805306368, 2818572288, 335544320, 
	2113929216, 3472883712, 2290089984, 3829399552, 3059744768, 1127219200, 
	3089629184, 4199809024, 3567124480, 1891565568, 394297344, 3988799488, 
	920674304, 4193267712, 2950604800, 3977188352, 3250028032, 129093376, 
	2231568512, 2963678272, 4281226848, 432124720, 803643432, 1633613396, 
	2672665246, 3170194367,
	2147483648, 3221225472, 2684354560, 3489660928, 1476395008, 2483027968, 
	1040187392, 3808428032, 3196059648, 599785472, 505413632, 4077912064, 
	1182269440, 1736704000, 2017853440, 2221342720, 3329785856, 2810494976, 
	3628507136, 1416089600, 2658719744, 864310272, 3863387648, 3076993792, 
	553150080, 272922560, 4167467040, 1148698640, 1719673080, 2009075780, 
	2149644390, 3222291575,
	2147483648, 1073741824, 2684354560, 1342177280, 2281701376, 1946157056, 
	436207616, 2566914048, 2625634304, 3208642560, 2720006144, 2098200576, 
	111673344, 2354315264, 3464626176, 4027383808, 2886631424, 3770826752, 
	1691164672, 3357462528, 1993345024, 3752330240, 873073152, 2870150400, 
	1700563072, 87021376, 1097028000, 1222351248, 1560027592, 2977959924, 
	23268898, 437609937
};

uint GrayCode(uint i) {
	return i ^ (i>>1);
}

float Sobol(uint d, uint i) {
	uint result = 0;
	uint offset = d * 32;
	for(uint j = 0; i > 0; i >>= 1, j++) 
	{
		if((i & 1) == 1)
			result ^= V[j+offset];
	}

	return float(result) * (1.0f/float(0xFFFFFFFFU));
}

vec2 SobolVec2(uint i, uint b) {
	float u = Sobol(b*2, GrayCode(i));
	float v = Sobol(b*2+1, GrayCode(i));
	return vec2(u, v);
}


BRDFMaterial DecodeBRDFMaterial(int MaterialSlotID)
{
    BRDFMaterial Material;
    EncodedBRDFMaterial EncodedMaterial = Materials[MaterialSlotID];

    Material.BaseColor = EncodedMaterial.BaseColor.xyz;
    Material.Emissive = EncodedMaterial.Emissive.xyz;
    Material.Subsurface = EncodedMaterial.Param1.x;
    Material.Metallic = EncodedMaterial.Param1.y;
    Material.Specular = EncodedMaterial.Param1.z;
    Material.SpecularTint = EncodedMaterial.Param1.w;
    Material.Roughness = EncodedMaterial.Param2.x;
    Material.Anisotropic = EncodedMaterial.Param2.y;
    Material.Sheen = EncodedMaterial.Param2.z;
    Material.SheenTint = EncodedMaterial.Param2.w;
    Material.Clearcoat = EncodedMaterial.Param3.x;
    Material.ClearcoatGloss = EncodedMaterial.Param3.y;

    return Material;
}

uvec2 GetFragCoord()
{
    vec2 uv = (CanvasPos.xy + 1.0)/2.0;
    return uvec2(uv * uResolution);
}

void InitRNG()
{
    uvec2 fc = GetFragCoord();
    rngState = uint(fc.x) * 1973u + uint(fc.y) * 9277u + uint(uFrameCounter) * 26699u + 1u;
}

uint wang_hash(uint seed) {
    seed = uint(seed ^ uint(61)) ^ uint(seed >> uint(16));
    seed *= uint(9);
    seed = seed ^ (seed >> 4);
    seed *= uint(0x27d4eb2d);
    seed = seed ^ (seed >> 15);
    return seed;
}

float rand()
{
    rngState = wang_hash(rngState);
    return float(rngState) / 4294967296.0;
}

float UintTo01(uint x)
{
    return float(x) / 4294967296.0;
}

vec2 PixelRotation(uvec2 pix, uint bounce)
{
    uint s0 = wang_hash(pix.x * 1973u ^ pix.y * 9277u ^ bounce * 26699u ^ 0x68bc21ebu);
    uint s1 = wang_hash(s0 ^ 0x02e5be93u);
    return vec2(UintTo01(s0), UintTo01(s1));
}

vec2 CranleyPattersonRotation(vec2 sobol2, uvec2 pix, uint bounce)
{
    vec2 p = sobol2 + PixelRotation(pix, bounce);

    if(p.x > 1)
        p.x -= 1;
    if(p.x < 0)
        p.x += 1;

    if(p.y > 1)
        p.y -= 1;
    if(p.y < 0)
        p.y += 1;

    return p;
}

vec3 SampleHemisphere(float x, float y) {
    float z = x;
    float r = max(0, sqrt(1.0 - z*z));
    float phi = 2.0 * PI * y;
    return vec3(r * cos(phi), r * sin(phi), z);
}

vec3 ToNormalHemisphere(vec3 v, vec3 N) {
    N = normalize(N);
    vec3 helper = vec3(1, 0, 0);
    if(abs(N.x)>0.999) helper = vec3(0, 0, 1);
    vec3 tangent = normalize(cross(N, helper));
    vec3 bitangent = normalize(cross(N, tangent));
    return normalize(v.x * tangent + v.y * bitangent + v.z * N);
}

vec2 SampleSphericalMap(vec3 v) {
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv /= vec2(2.0 * PI, PI);
    uv += 0.5;
    return uv;
}

vec3 SampleSkybox(vec3 v) {
    vec2 uv = SampleSphericalMap(normalize(v));
    vec3 color = texture2D(uSkybox, uv).rgb;
    return color;
}


vec3 GetRayDirection(vec2 uv)
{
    float scale = tan(radians(UCameraParam.AspectAndFovy.y) * 0.5);

    vec2 pixelSize = 2.0 / uResolution;

    uv += pixelSize * (rand() - 0.5);

    vec3 DirWorldSpace = (uv.x * UCameraParam.AspectAndFovy.x * scale) * UCameraParam.Right.xyz +
        (uv.y * scale) * UCameraParam.Up.xyz +
        UCameraParam.Front.xyz;

    return normalize(DirWorldSpace);
}

HitResult HitTriangle(int Primitive_index, Ray ray)
{
    HitResult hit_result;
    hit_result.bIsHit = false;
    hit_result.Distance = INF;
    hit_result.MaterialSlot = -1;

    Primitive Primitive = Primitives[Primitive_index];

    vec3 p1 = Primitive.P1.xyz;
    vec3 p2 = Primitive.P2.xyz;
    vec3 p3 = Primitive.P3.xyz;

    vec3 edge1 = p2 - p1;
    vec3 edge2 = p3 - p1;

    vec3 Ng = normalize(cross(edge1, edge2));

    vec3 pvec = cross(ray.Direction, edge2);
    float det = dot(edge1, pvec);

    if (abs(det) < EPS) return hit_result;

    float invDet = 1.0 / det;

    vec3 tvec = ray.Start - p1;
    float u = dot(tvec, pvec) * invDet;
    if (u < 0.0 || u > 1.0) return hit_result;

    vec3 qvec = cross(tvec, edge1);
    float v = dot(ray.Direction, qvec) * invDet;
    if (v < 0.0 || u + v > 1.0) return hit_result;

    float t = dot(edge2, qvec) * invDet;
    if (t < EPS) return hit_result;

    hit_result.bIsHit = true;
    hit_result.Distance = t;
    hit_result.HitPoint = ray.Start + t * ray.Direction;
    hit_result.MaterialSlot = Primitive.MaterialSlot.x;

    float w1 = 1.0 - u - v;
    vec3 Ns = normalize(w1 * Primitive.N1.xyz + u * Primitive.N2.xyz + v * Primitive.N3.xyz);

    if (dot(Ng, ray.Direction) > 0.0)
        Ng = -Ng;

    if (dot(Ns, Ng) < 0.0)
        Ns = -Ns;

    hit_result.GeoNormal = Ng;
    hit_result.ShadeNormal = Ns;

    return hit_result;
}

HitResult Hit(Ray ray, int l, int r) 
{
    HitResult hit_result;
    hit_result.bIsHit = false;    
    hit_result.Distance = INF;
    hit_result.MaterialSlot = -1; 

	for (int i = l; i <= r; i++)
	{
		HitResult temp = HitTriangle(int(SortedMorton3D[i].y), ray);
		if (temp.bIsHit && temp.Distance < hit_result.Distance)
			hit_result = temp;
	}
    return hit_result;
}

float HitAABB(int node_index, Ray ray)
{
    vec3 AABB_MinPos = vec3(LBVHNodes[node_index].AABB_MinPos);
    vec3 AABB_MaxPos = vec3(LBVHNodes[node_index].AABB_MaxPos);

    vec3 invDir = 1.0 / ray.Direction;
    
    vec3 t0s = (AABB_MinPos - ray.Start) * invDir;
    vec3 t1s = (AABB_MaxPos - ray.Start) * invDir;
    
    vec3 tmin = min(t0s, t1s);
    vec3 tmax = max(t0s, t1s);
    
    float t_start = max(tmin.x, max(tmin.y, tmin.z));
    float t_end = min(tmax.x, min(tmax.y, tmax.z));
    
    if (t_start <= t_end && t_end > 0.0)
    {
        return t_start > 0.0 ? t_start : t_end; 
    }

    return -1.0;
}

HitResult HitBVH(Ray ray)
{
    HitResult hit_result;
    hit_result.bIsHit = false;    
    hit_result.Distance = INF;
    hit_result.MaterialSlot = -1; 

    int stack[256];
    int top = 0;

    stack[top++] = Root.x;

    while(top > 0)
    {
        int cur_node_idx = stack[--top];
        
        if(cur_node_idx < 0 || cur_node_idx >= uLBVHNodeCount) 
            continue;

        int left = LBVHNodes[cur_node_idx].Param1.x;
        int right = LBVHNodes[cur_node_idx].Param1.y;
        int isLeaf = LBVHNodes[cur_node_idx].Param1.z;
        int front = LBVHNodes[cur_node_idx].Param2.x;
        int back = LBVHNodes[cur_node_idx].Param2.y;

        if(isLeaf == 1)
        {
            HitResult temp = Hit(ray, front , back); 
            if (temp.bIsHit && temp.Distance < hit_result.Distance)
			    hit_result = temp;
            continue;
        }
        
        float t_left = -INF;
        float t_right = -INF;

        if(left >= 0 && left < uLBVHNodeCount)
            t_left = HitAABB(left, ray);
        if(right >= 0 && right < uLBVHNodeCount)
            t_right = HitAABB(right, ray);

        if(t_left > 0 && t_right > 0)
        {
            if(t_left > t_right)
            {
                stack[top++] = left;
                stack[top++] = right;
            }else{
                stack[top++] = right;
                stack[top++] = left;
            }
        }
        else if (t_left > 0.0) {
            stack[top++] = left;
        } else if (t_right > 0.0) { 
            stack[top++] = right;
        }
    }
    
    return hit_result;
}

float sqr(float x) { return x*x; }

float SchlickFresnel(float u)
{
    float m = clamp(1-u, 0, 1);
    float m2 = m*m;
    return m2*m2*m; // pow(m,5)
}

float GTR1(float NdotH, float a)
{
    if (a >= 1) return 1/PI;
    float a2 = a*a;
    float t = 1 + (a2-1)*NdotH*NdotH;
    return (a2-1) / (PI*log(a2)*t);
}

float GTR2(float NdotH, float a)
{
    float a2 = a*a;
    float t = 1 + (a2-1)*NdotH*NdotH;
    return a2 / (PI * t*t);
}

float GTR2_aniso(float NdotH, float HdotX, float HdotY, float ax, float ay)
{
    return 1 / (PI * ax*ay * sqr( sqr(HdotX/ax) + sqr(HdotY/ay) + NdotH*NdotH ));
}

float smithG_GGX(float NdotV, float alphaG)
{
    float a = alphaG*alphaG;
    float b = NdotV*NdotV;
    return 1 / (NdotV + sqrt(a + b - a*b));
}

float smithG_GGX_aniso(float NdotV, float VdotX, float VdotY, float ax, float ay)
{
    return 1 / (NdotV + sqrt( sqr(VdotX*ax) + sqr(VdotY*ay) + sqr(NdotV) ));
}

vec3 mon2lin(vec3 x)
{
    return pow(x, vec3(2.2));
}

vec3 BRDF( vec3 L, vec3 V, vec3 N,  int MaterialSlotID)
{
    BRDFMaterial Mat = DecodeBRDFMaterial(MaterialSlotID);

    float NdotL = dot(N,L);
    float NdotV = dot(N,V);
    if (NdotL < 0 || NdotV < 0) return vec3(0);

    vec3 H = normalize(L+V);
    float NdotH = dot(N,H);
    float LdotH = dot(L,H);
    
    vec3 Cdlin = Mat.BaseColor;
    float Cdlum = .3*Cdlin[0] + .6*Cdlin[1]  + .1*Cdlin[2]; // luminance approx.
    
    vec3 Ctint = Cdlum > 0 ? Cdlin/Cdlum : vec3(1); // normalize lum. to isolate hue+sat
    vec3 Cspec0 = mix(Mat.Specular*.08*mix(vec3(1), Ctint, Mat.SpecularTint), Cdlin, Mat.Metallic);
    vec3 Csheen = mix(vec3(1), Ctint, Mat.SheenTint);

    // Diffuse fresnel - go from 1 at normal incidence to .5 at grazing
    // and mix in diffuse retro-reflection based on roughness
    float FL = SchlickFresnel(NdotL), FV = SchlickFresnel(NdotV);
    float Fd90 = 0.5 + 2 * LdotH*LdotH * Mat.Roughness;
    float Fd = mix(1.0, Fd90, FL) * mix(1.0, Fd90, FV);

    // Based on Hanrahan-Krueger brdf approximation of isotropic bssrdf
    // 1.25 scale is used to (roughly) preserve albedo
    // Fss90 used to "flatten" retroreflection based on roughness
    float Fss90 = LdotH*LdotH*Mat.Roughness;
    float Fss = mix(1.0, Fss90, FL) * mix(1.0, Fss90, FV);
    float ss = 1.25 * (Fss * (1 / (NdotL + NdotV) - .5) + .5);

    vec3 diffuse = (1.0/PI) * mix(Fd, ss, Mat.Subsurface) * Cdlin;

    float alpha = Mat.Roughness * Mat.Roughness;
    float Ds =  GTR2(NdotH, alpha);
    float FH = SchlickFresnel(LdotH);
    vec3 Fs = mix(Cspec0, vec3(1), FH);
    float Gs = smithG_GGX(NdotL, Mat.Roughness) * smithG_GGX(NdotV, Mat.Roughness);

    vec3 specular = Gs * Fs * Ds;

    // sheen
    vec3 Fsheen = FH * Mat.Sheen * Csheen;
    diffuse += Fsheen;

    // clearcoat (ior = 1.5 -> F0 = 0.04)
    float Dr = GTR1(NdotH, mix(.1,.001,Mat.ClearcoatGloss));
    float Fr = mix(.04, 1.0, FH);
    float Gr = smithG_GGX(NdotL, .25) * smithG_GGX(NdotV, .25);

    return diffuse  * (1.0 - Mat.Metallic) + specular + .25 * Mat.Clearcoat * Gr * Fr * Ds;
}

vec3 PathTracing(Ray ray, int maxBounce)
{
    vec3 finalColor = vec3(0.0);
    vec3 throughput = vec3(1.0);
    Ray currRay = ray;

    uvec2 pix = uvec2(gl_FragCoord.xy);

    for(int bounce = 0; bounce < maxBounce; bounce++)
    {
        HitResult hit_result = HitBVH(currRay);

        if(!hit_result.bIsHit)
        {
            finalColor += throughput * SkyColor;
            break;
        }

        EncodedBRDFMaterial material = Materials[hit_result.MaterialSlot];
        finalColor += throughput * material.Emissive.xyz;

        vec3 Ng = normalize(hit_result.GeoNormal);
        vec3 Ns = normalize(hit_result.ShadeNormal);

        vec3 V = -currRay.Direction;

        vec3 wi;

        if (uEnableSobol == 1){
            vec2 Sobol2 = SobolVec2(uFrameCounter + 1u, bounce);
            vec2 p = CranleyPattersonRotation(Sobol2, pix, bounce);

            wi = ToNormalHemisphere(SampleHemisphere(p.x, p.y), Ng);
        }else{
            wi = ToNormalHemisphere(SampleHemisphere(rand(), rand()), Ng);
        }

        float PDF = 1.0 / (2.0 * PI);
        float cosTheta = max(dot(wi, Ns), 0.0);

        throughput *= BRDF(wi, V, Ns, hit_result.MaterialSlot) * cosTheta / PDF;

        currRay.Start = hit_result.HitPoint + Ng * 1e-4;
        currRay.Direction = wi;

        if(bounce >= 3)
        {
            float p = clamp(max(throughput.r, max(throughput.g, throughput.b)), 0.05, 0.95);
            if(rand() > p)
                break;
            throughput /= p;
        }
    }

    return finalColor;
}

void main()
{
    InitRNG();

    Ray ray;
    ray.Start = UCameraParam.Position.xyz;
    ray.Direction = GetRayDirection(CanvasPos.xy);

    vec4 Color = vec4(PathTracing(ray, 4), 1.0);
    ivec2 pix = ivec2(gl_FragCoord.xy);

    vec4 LastFrameColor = texelFetch(uLastFrame, pix , 0);

    if(uFrameCounter < 4096)
        FragColor = mix(LastFrameColor, Color, 1.0/float(uFrameCounter+1));
    else
        FragColor = LastFrameColor;
}



