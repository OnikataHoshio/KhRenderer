#version 460 core
out vec4 FragColor;

in vec3 CanvasPos; 

struct Primitive{
    vec4 P1, P2, P3;
    vec4 N1, N2, N3;
    ivec2 PrimitiveType;
    ivec2 MaterialSlot;
};


struct BSSRDFMaterial{
    vec4 Emissive;
    vec4 BaseColor;
    vec4 Radius;
    vec2 Eta;
    vec2 Scale;
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
layout(std430, binding = 4) buffer BSSRDFMaterialSSBO{ BSSRDFMaterial Materials[]; };

layout(std140, binding = 5) uniform CameraBlock {
    vec4 AspectAndFovy; // x: Aspect, y: Fovy
    vec4 Position;
    vec4 Right;
    vec4 Up;
    vec4 Front;
} UCameraParam;

layout(std430, binding = 6) buffer InvertCDFSSBO {float InvertCDF[]; };

uniform sampler2D uLastFrame;
uniform sampler2D uSkybox;
uniform sampler2D uHDRCache;
uniform int uLBVHNodeCount;
uniform uint uFrameCounter;
uniform uvec2 uResolution; 

uniform int uInvertCDFResolution;
//uniform float uRmax;

uniform int uEnableSkybox;

const vec3 SkyColor = vec3(0.05, 0.05, 0.05);

struct Ray
{
	vec3 Start;
	vec3 Direction;
};

struct HitResult {
	bool bIsHit;
    bool bIsInside;
	float Distance;
	vec3 HitPoint;
	vec3 GeoNormal;
    vec3 ShadeNormal;
    int MaterialSlot;
};

#define INF             1e30
#define EPS             1e-8
#define PI              3.14159265358979323846

uint rngState;

const uint V[16*32] = {
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
	23268898, 437609937,
    2147483648, 1073741824, 2684354560, 1342177280, 671088640, 3556769792,
    1778384896, 1895825408, 947912704, 1480589312, 3927965696, 823132160,
    2561146880, 139722752, 3257532416, 3844407296, 4071784448, 2034778112,
    4205060096, 3178434560, 413665280, 1213465600, 1646922240, 3039102208,
    3669131904, 2907196736, 2426676896, 3430094608, 539495304, 269746564,
    2282357922, 2218067473,
    2147483648, 1073741824, 3758096384, 2952790016, 2550136832, 2483027968,
    2315255808, 1526726656, 864026624, 3653238784, 1914699776, 1058013184,
    3250061312, 2800484352, 1401290752, 703922176, 171606016, 455786496,
    3549618176, 1778348032, 3929540608, 2871788544, 1269173760, 4259646208,
    1610779008, 4026976576, 2016733344, 605713840, 305826616, 3475687836,
    3113412898, 2197780721,
    2147483648, 1073741824, 2684354560, 268435456, 134217728, 1811939328,
    2650800128, 587202560, 1468006400, 2915041280, 2141192192, 2446327808,
    1233649664, 3470000128, 2282356736, 739180544, 1041072128, 857194496,
    1605394432, 3254300672, 3784148992, 3000484864, 504392192, 1663611136,
    4152723584, 3183723200, 2008703968, 4260868912, 3615493624, 3988785180,
    3751805978, 2177894957,
    2147483648, 1073741824, 536870912, 805306368, 1476395008, 2885681152,
    2516582400, 721420288, 3565158400, 155189248, 3802136576, 1380974592,
    1311244288, 3340500992, 1654521856, 308740096, 1846771712, 4147232768,
    983080960, 3192164352, 4164651008, 3693986816, 3993412096, 3072561920,
    447221120, 2388397760, 2688420704, 1882653104, 2017167560, 2620246612,
    3456542538, 2267256725,
    2147483648, 3221225472, 2684354560, 1342177280, 4160749568, 2348810240,
    3791650816, 855638016, 260046848, 557842432, 2510290944, 1584398336,
    3624402944, 472121344, 3122003968, 4013359104, 361136128, 2658123776,
    2015059968, 1278513152, 1108248576, 1661717504, 4155337216, 2910033152,
    2004879232, 1832912064, 3617588256, 1030751792, 797446008, 2976123604,
    3451258746, 2185692887,
    2147483648, 3221225472, 1610612736, 2415919104, 939524096, 3288334336,
    1107296256, 2734686208, 4051697664, 2856321024, 4242538496, 2232418304,
    3758620672, 1342963712, 1476788224, 1409875968, 2047049728, 1728856064,
    3011780608, 155856896, 225384448, 794469376, 484953600, 3574878464,
    3087007872, 67109056, 570425440, 855638160, 3380609080, 1849688260,
    3202351170, 638582947,
    2147483648, 1073741824, 536870912, 4026531840, 2818572288, 1409286144,
    2583691264, 2634022912, 511705088, 1556086784, 2099249152, 2366636032,
    612892672, 1908670464, 3953262592, 1977548800, 1805811712, 902905856,
    1269014528, 3318927360, 3819005952, 2447084544, 2041508352, 215957760,
    1730838656, 1343569984, 436314144, 3708670192, 1048832168, 2898971732,
    3576517274, 3642593693,
    2147483648, 3221225472, 536870912, 3489660928, 3623878656, 3288334336,
    1174405120, 2231369728, 2776629248, 1992294400, 2912944128, 1789919232,
    765984768, 2864447488, 229244928, 2058420224, 3584524288, 3200073728,
    2476990464, 1001721856, 908703744, 1299348480, 2609078784, 667211520,
    3056187520, 2373090496, 3145949728, 4156872656, 1848227928, 1232239620,
    4253246054, 1925502805
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

void InitRNG()
{
    uvec2 fc = uvec2(gl_FragCoord.xy);
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
    return fract(sobol2 + PixelRotation(pix, bounce));
}

vec3 SampleHemisphere(float xi_1, float xi_2) {
    float z = xi_1;
    float r = max(0, sqrt(1.0 - z*z));
    float phi = 2.0 * PI * xi_2;
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
    vec3 color = texture(uSkybox, uv).rgb;
    return color;
}

vec3 SampleCosineHemisphere(float xi_1, float xi_2, vec3 N) {
    float r = sqrt(xi_1);
    float theta = xi_2 * 2.0 * PI;
    float x = r * cos(theta);
    float y = r * sin(theta);
    float z = sqrt(max(0.0, 1.0 - xi_1));

    vec3 L = ToNormalHemisphere(vec3(x, y, z), N);
    return L;
}

vec3 GetRayDirection(vec2 uv)
{
    float scale = tan(radians(UCameraParam.AspectAndFovy.y) * 0.5);

    vec2 pixelSize = 2.0 / vec2(uResolution);

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
    hit_result.bIsInside = false;
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

    if (dot(Ng, ray.Direction) > 0.0){
        hit_result.bIsInside = true;
        Ng = -Ng;
    }

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
    hit_result.bIsInside = false;
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
    hit_result.bIsInside = false;
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

vec3 FetchHDRCache(float xi1, float xi2)
{
    ivec2 size = textureSize(uHDRCache, 0);
    ivec2 p = ivec2(
        min(int(xi1 * float(size.x)), size.x - 1),
        min(int(xi2 * float(size.y)), size.y - 1)
    );
    return texelFetch(uHDRCache, p, 0).rgb;
}

float EnvPdf_Eval(vec3 wi)
{
    vec2 uv = SampleSphericalMap(normalize(wi));
    ivec2 size = textureSize(uHDRCache, 0);
    ivec2 p = ivec2(
        min(int(uv.x * float(size.x)), size.x - 1),
        min(int(uv.y * float(size.y)), size.y - 1)
    );
    float pdfUV = texelFetch(uHDRCache, p, 0).a;

    float elev = PI * (uv.y - 0.5);
    float sinThetaPolar = max(cos(elev), 1e-6);

    return pdfUV / (2.0 * PI * PI * sinThetaPolar);
}

float PowerHeuristic2(float a, float b)
{
    float a2 = a * a;
    float b2 = b * b;
    float s = a2 + b2;
    return (s > 1e-20) ? (a2 / s) : 0.0;
}

float PowerHeuristic3(float a, float b, float c)
{
    float a2 = a * a;
    float b2 = b * b;
    float c2 = c * c;
    float s = a2 + b2 + c2;
    return (s > 1e-20) ? (a2 / s) : 0.0;
}

vec3 SampleHDR(float xi_env1, float xi_env2,  vec3 V, HitResult hit_result){
    vec3 values = FetchHDRCache(xi_env1, xi_env2);
    
    vec2 uv = values.xy;
    float pdfUV = values.z;

    float phi  = 2.0 * PI * (uv.x - 0.5);
    float elev = PI * (uv.y - 0.5);

    float cosElev = cos(elev);
    float sinElev = sin(elev);

    vec3 wi = normalize(vec3(
        cosElev * cos(phi),
        sinElev,
        cosElev * sin(phi)
    ));

    vec3 Ng = normalize(hit_result.GeoNormal);
    vec3 Ns = normalize(hit_result.ShadeNormal);

    float NdotL = dot(Ns, wi);
    if (NdotL <= 0.0 || dot(Ng, wi) <= 0.0)
        return vec3(0.0);

    Ray shadowRay;
    shadowRay.Start = hit_result.HitPoint + Ng * 1e-4;
    shadowRay.Direction = wi;

    HitResult occ = HitBVH(shadowRay);
    if (occ.bIsHit)
        return vec3(0.0);

    vec3 envColor = texture(uSkybox, uv).rgb;

    float sinThetaPolar = max(cosElev, 1e-6);
    float pdfEnv = pdfUV / (2.0 * PI * PI * sinThetaPolar);

    float weight = 1.0;
    
//    float pdfBRDF = BRDF_PDF_Eval(wi, V, Ns, hit_result.MaterialSlot);
//    weight = PowerHeuristic(pdfEnv, pdfBRDF);
    
//    return weight * envColor
//         * BRDF(wi, V, Ns, hit_result.MaterialSlot)
//         * NdotL
//         / max(pdfEnv, 1e-6);

    return SkyColor;

}

float FresnelDielectric(float cosThetaI, float etaI, float etaT)
{
    cosThetaI = clamp(abs(cosThetaI), 0.0, 1.0);

    float sin2ThetaI = max(0.0, 1.0 - cosThetaI * cosThetaI);
    float eta = etaI / etaT;
    float sin2ThetaT = eta * eta * sin2ThetaI;

    // Total internal reflection
    if (sin2ThetaT >= 1.0)
        return 1.0;

    float cosThetaT = sqrt(max(0.0, 1.0 - sin2ThetaT));

    float Rs = (etaI * cosThetaI - etaT * cosThetaT) /
               (etaI * cosThetaI + etaT * cosThetaT);
    float Rp = (etaT * cosThetaI - etaI * cosThetaT) /
               (etaT * cosThetaI + etaI * cosThetaT);

    return 0.5 * (Rs * Rs + Rp * Rp);
}

float FtIn(float cosThetaI, float eta)
{
    return 1.0 - FresnelDielectric(cosThetaI, 1.0, eta);
}

float FtOut(float cosThetaO, float eta)
{
    return 1.0 - FresnelDielectric(cosThetaO, eta, 1.0);
}

vec3 BurleyProfile(float rTrue, vec3 d)
{
    float r = max(rTrue, 1e-4);
    vec3 dd = max(d, vec3(1e-4));

    vec3 e1 = exp(-r / dd);
    vec3 e3 = exp(-r / (3.0 * dd));

    return (e1 + e3) / (8.0 * PI * dd * r);
}


void DecodeBSSRDFMaterial(in int materialSlot, out vec3 baseColor, out vec3 radius, out float eta, out float scale)
{
    BSSRDFMaterial mat = Materials[materialSlot];
    baseColor   = mat.BaseColor.rgb;
    radius   = mat.Radius.rgb;
    eta = mat.Eta.r;
    scale = mat.Scale.r;
}

vec3 EvalBSSRDF(vec3 wi, vec3 pi, vec3 Ni,
                vec3 wo, vec3 po, vec3 No,
                int materialSlot)
{
    vec3 baseColor, radius;
    float eta, scale;
    DecodeBSSRDFMaterial(materialSlot, baseColor, radius, eta, scale);

    float rTrue = length(pi - po);

    float cosThetaIn  = max(dot(Ni, wi), 0.0);
    float cosThetaOut = max(dot(No, wo), 0.0);

    vec3 d = 0.25 * scale * radius / PI;

    //float ftIn  = FtIn(cosThetaIn, eta);
    //float ftOut = FtOut(cosThetaOut, eta);

    //return ftIn * BurleyProfile(rTrue, A, d) * ftOut;

    return baseColor * BurleyProfile(rTrue, d);
}

float SampleUnitInverseCDF(float xi)
{
    xi = clamp(xi, 0.0, 1.0);

    float u = xi * float(uInvertCDFResolution - 1);
    int i0 = clamp(int(floor(u)), 0, uInvertCDFResolution - 2);
    int i1 = i0 + 1;
    float t = u - float(i0);

    return mix(InvertCDF[i0], InvertCDF[i1], t);
}

float BurleyRadiusCDF(float r, float d)
{
    return 1.0
         - 0.25 * exp(-r / d)
         - 0.75 * exp(-r / (3.0 * d));
}

float BurleyRadiusPdf(float r, float d, float rMax)
{
    float rr = max(r, 1e-4);
    float dd = max(d, 1e-4);

    float pdf = 0.25 / dd * (exp(-rr / dd) + exp(-rr / (3.0 * dd)));

    float norm = BurleyRadiusCDF(rMax, dd);
    pdf /= max(norm, 1e-6);

    return pdf;
}

float SampleBurleyRadiusTruncated(float xi, float d, float rMax)
{
    float cdfMax = BurleyRadiusCDF(rMax, d);
    float u = xi * cdfMax;   
    return SampleUnitInverseCDF(u) * d;
}

float BurleyDiskPdf(float rho, float d, float rMax)
{
    return BurleyRadiusPdf(rho, d, rMax) / (2.0 * PI * max(rho, 1e-4));
}

void BuildOrthonormalBasis(in vec3 N, out vec3 T, out vec3 B)
{
    vec3 up = (abs(N.y) < 0.999) ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
    T = normalize(cross(up, N));
    B = normalize(cross(N, T));
}

void SelectProjectionAxis(
    float xiAxis,
    vec3 N, vec3 T, vec3 B,
    out vec3 axis,
    out vec3 diskU,
    out vec3 diskV,
    out float axisPdf)
{
    if (xiAxis < 0.25)
    {
        axis = T;
        diskU = B;
        diskV = N;
        axisPdf = 0.25;
    }
    else if (xiAxis < 0.5)
    {
        axis = B;
        diskU = N;
        diskV = T;
        axisPdf = 0.25;
    }
    else
    {
        axis = N;
        diskU = T;
        diskV = B;
        axisPdf = 0.50;
    }
}

bool CollectProbeHits(
    vec3 segStart,
    vec3 segDir,
    float segLen,
    int materialSlot,
    out HitResult hits[8],
    out int hitCount)
{
    hitCount = 0;

    vec3 currStart = segStart;
    float traveled = 0.0;

    for (int iter = 0; iter < 32 && hitCount < 8; ++iter)
    {
        Ray ray;
        ray.Start = currStart;
        ray.Direction = segDir;

        HitResult h = HitBVH(ray);
        if (!h.bIsHit)
            break;

        if (traveled + h.Distance > segLen)
            break;

        traveled += h.Distance;

        if (h.MaterialSlot == materialSlot)
        {   
            if(h.bIsInside)
            {
                h.GeoNormal = -h.GeoNormal;
                if(dot(h.GeoNormal, h.ShadeNormal) < 0.0)
                    h.ShadeNormal = -h.ShadeNormal;
            }
            hits[hitCount++] = h;
        }

        float advance = h.Distance + 1e-4;
        currStart += advance * segDir;
        traveled += 1e-4;

        if (traveled >= segLen)
            break;
    }

    return hitCount > 0;
}

float EvalPosPdf(vec3 axis, vec3 po, vec3 pi, vec3 Ng, int materialSlot, float axisProb)
{
    vec3 delta = pi - po;
    float h = dot(delta, axis);
    vec3 proj = delta - h * axis;
    float rho = length(proj);

    BSSRDFMaterial mat = Materials[materialSlot];

    vec3 baseColor, radius;
    float eta, scale;
    DecodeBSSRDFMaterial(materialSlot, baseColor, radius, eta, scale);
    vec3 d = 0.25 * scale * radius / PI;
    float Rmax = 16 * d.r;
    float dPdf = max((d.r + d.g + d.b)/3.0f, 1e-4);

    if (rho >= Rmax)
        return 0.0;

    float absVdotNg = abs(dot(axis, Ng));
    if (absVdotNg < 1e-6)
        return 0.0;


    vec3 q = po + proj;
    float halfLen = sqrt(max(Rmax * Rmax - rho * rho, 0.0));
    vec3 segStart = q - halfLen * axis;
    float segLen  = 2.0 * halfLen;

    HitResult hits[8];
    int hitCount = 0;
    if (!CollectProbeHits(segStart, axis, segLen, materialSlot, hits, hitCount))
        return 0.0;

    return axisProb
         * BurleyDiskPdf(rho, dPdf, Rmax)
         * absVdotNg
         / float(hitCount);
}


struct SampleBSSRDFResult
{
    bool bHasResult;
    vec3 pi;
    vec3 wi;
    vec3 Ns;
    vec3 Ng;
    vec3 axis;

    float rho;        // projected radius
    float pdfPos;     // position pdf
    float pdfDir;     // direction pdf
    float weightMIS;
    float cosTheta;   // max(dot(Ns, wi), 0)
    int hitCount;
};

SampleBSSRDFResult SampleBSSRDF(vec3 wo, vec3 po, vec3 No, int materialSlot)
{
    SampleBSSRDFResult result;
    result.bHasResult = false;
    result.pdfPos = 0.0;
    result.pdfDir = 0.0;
    result.cosTheta = 0.0;
    result.rho = 0.0;
    result.hitCount = 0;

    BSSRDFMaterial mat = Materials[materialSlot];
    vec3 baseColor, radius;
    float eta, scale;
    DecodeBSSRDFMaterial(materialSlot, baseColor, radius, eta, scale);
    vec3 d = 0.25 * scale * radius / PI;
    float Rmax = 16 * d.r;

    float dPdf = max((d.r + d.g + d.b)/3.0f, 1e-4);

    vec3 N = normalize(No);
    vec3 T, B;
    BuildOrthonormalBasis(N, T, B);

    float xiR    = rand();
    float xiAxis = rand();
    float xiPhi  = rand();

    float rho = SampleBurleyRadiusTruncated(rand(), dPdf, Rmax);
    rho = min(rho, Rmax - 1e-4);

    float phi = 2.0 * PI * xiPhi;

    vec3 axis, diskU, diskV;
    float axisPdf;
    SelectProjectionAxis(xiAxis, N, T, B, axis, diskU, diskV, axisPdf);

    vec3 q = po + rho * (cos(phi) * diskU + sin(phi) * diskV);

    float halfLen = sqrt(max(Rmax * Rmax - rho * rho, 0.0));
    vec3 segStart = q - halfLen * axis;
    float segLen  = 2.0 * halfLen;

    HitResult hits[8];
    int hitCount = 0;
    if (!CollectProbeHits(segStart, axis, segLen, materialSlot, hits, hitCount))
        return result;

    int selected = min(int(floor(rand() * float(hitCount))), hitCount - 1);
    HitResult h = hits[selected];

    vec3 Ns = normalize(h.ShadeNormal);
    vec3 Ng = normalize(h.GeoNormal);

    vec3 wi = SampleCosineHemisphere(rand(), rand(), Ns);
    float cosTheta = max(dot(Ns, wi), 0.0);
    if (cosTheta <= 0.0 || dot(Ng, wi) <= 0.0)
        return result;

    float pdfDir = cosTheta / PI;

    float pdfPos = axisPdf
                 * BurleyDiskPdf(rho, dPdf, Rmax)
                 * abs(dot(axis, Ng))
                 / float(hitCount);
    
    float pdf_diskU =  EvalPosPdf(diskU, po, h.HitPoint, Ng, materialSlot, abs(dot(diskU, N)) > 0.999 ? 0.5 : 0.25);
    float pdf_diskV =  EvalPosPdf(diskV, po, h.HitPoint, Ng, materialSlot, abs(dot(diskV, N)) > 0.999 ? 0.5 : 0.25);

    float weightMIS = PowerHeuristic3(pdfPos, pdf_diskU, pdf_diskV);

    result.bHasResult = true;
    result.pi = h.HitPoint;
    result.wi = wi;
    result.Ns = Ns;
    result.Ng = Ng;
    result.axis = axis;
    result.rho = rho;
    result.pdfPos = pdfPos;
    result.pdfDir = pdfDir;
    result.weightMIS = weightMIS;
    result.cosTheta = cosTheta;
    result.hitCount = hitCount;

    return result;
}


vec3 PathTracing(Ray ray, int maxBounce)
{
    vec3 finalColor = vec3(0.0);
    vec3 throughput = vec3(1.0);
    Ray currRay = ray;

    uvec2 pix = uvec2(gl_FragCoord.xy);

    float xi_brdf1 = rand();
    float xi_brdf2 = rand();
    float xi_env1 = rand();
    float xi_env2 = rand();
    float xi_1 = rand();

    float pdf_BSSRDF = 0.0;
    for (int bounce = 0; bounce < maxBounce; bounce++)
    {
        HitResult hit_result = HitBVH(currRay);
        vec3 V  = -currRay.Direction;

        if (!hit_result.bIsHit)
        {
            if(uEnableSkybox == 1)
            {
                finalColor += throughput * SampleSkybox(currRay.Direction);
            }
            else
            {
                finalColor += throughput * SkyColor;
            }
            break;
        }

        vec3 Ng = normalize(hit_result.GeoNormal);
        vec3 Ns = normalize(hit_result.ShadeNormal);


        BSSRDFMaterial Mat = Materials[hit_result.MaterialSlot];
        finalColor += throughput * Mat.Emissive.rgb;

//        if(uEnableSkybox == 1)
//        {
//            finalColor += throughput * SampleHDR(xi_env1, xi_env2, V, hit_result);
//        }
    
        SampleBSSRDFResult s = SampleBSSRDF(V, hit_result.HitPoint, Ns, hit_result.MaterialSlot);

        if (!s.bHasResult || s.pdfPos <= 0.0 || s.pdfDir <= 0.0)
            break; 

        vec3 S = EvalBSSRDF(
            s.wi, s.pi, s.Ns,
            V, hit_result.HitPoint, Ns,
            hit_result.MaterialSlot
        );

        throughput *= s.weightMIS * S * s.cosTheta / max(s.pdfPos * s.pdfDir, 1e-7);

        currRay.Start = s.pi + s.Ng * 1e-4;
        currRay.Direction = s.wi;

        if (bounce >= 3)
        {
            float p = clamp(max(throughput.r, max(throughput.g, throughput.b)), 0.05, 0.95);
            if (rand() > p)
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

    if(uFrameCounter < 2048)
        FragColor = mix(LastFrameColor, Color, 1.0/float(uFrameCounter+1));
    else
        FragColor = LastFrameColor;
}
