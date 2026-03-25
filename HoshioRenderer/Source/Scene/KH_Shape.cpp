#include "KH_Shape.h"
#include "Hit/KH_Ray.h"
#include "Utils/KH_DebugUtils.h"

void KH_Hitable::CollectPrimitiveAABBCenters(std::vector<glm::vec4>& outCenters) const
{
    outCenters.push_back(glm::vec4(AABB.GetCenter(), 1.0f));
}

const KH_AABB& KH_Hitable::GetAABB() const
{
    return AABB;
}

void KH_Object::SetPosition(const glm::vec3& InPosition)
{
    Position = InPosition;
    OnTransformChanged();
}

void KH_Object::SetPosition(float X, float Y, float Z)
{
    Position = glm::vec3(X, Y, Z);
    OnTransformChanged();
}

void KH_Object::SetScale(const glm::vec3& InScale)
{
    Scale = InScale;
    OnTransformChanged();
}

void KH_Object::SetScale(float X, float Y, float Z)
{
    Scale = glm::vec3(X, Y, Z);
    OnTransformChanged();
}

void KH_Object::SetUniformScale(float S)
{
    Scale = glm::vec3(S, S, S);
    OnTransformChanged();
}

void KH_Object::SetRotation(const glm::vec3& InRotation)
{
    Rotation = InRotation;
    OnTransformChanged();
}

void KH_Object::SetRotation(float Pitch, float Yaw, float Roll)
{
    Rotation = glm::vec3(Pitch, Yaw, Roll);
    OnTransformChanged();
}

void KH_Object::Translate(const glm::vec3& Delta)
{
    Position += Delta;
    OnTransformChanged();
}

void KH_Object::Translate(float X, float Y, float Z)
{
    Position += glm::vec3(X, Y, Z);
    OnTransformChanged();
}

void KH_Object::Rotate(const glm::vec3& DeltaRotation)
{
    Rotation += DeltaRotation;
    OnTransformChanged();
}

void KH_Object::Rotate(float Pitch, float Yaw, float Roll)
{
    Rotation += glm::vec3(Pitch, Yaw, Roll);
    OnTransformChanged();
}

void KH_Object::AddScale(const glm::vec3& DeltaScale)
{
    Scale += DeltaScale;
    OnTransformChanged();
}

void KH_Object::AddScale(float X, float Y, float Z)
{
    Scale += glm::vec3(X, Y, Z);
    OnTransformChanged();
}

const glm::vec3& KH_Object::GetPosition() const
{
    return Position;
}

const glm::vec3& KH_Object::GetScale() const
{
    return Scale;
}

const glm::vec3& KH_Object::GetRotation() const
{
    return Rotation;
}

glm::mat4 KH_Object::GetModelMatrix() const
{
    return ModelMatrix;
}

glm::mat3 KH_Object::GetNormalMatrix() const
{
    return NormalMatrix;
}

glm::mat3 KH_Object::GetNormalMatrix(glm::mat4 Model)
{
    return glm::transpose(glm::inverse(glm::mat3(Model)));
}

glm::mat4 KH_Object::UpdateModelMatrix()
{
    ModelMatrix =  glm::mat4(1.0f);

    ModelMatrix = glm::translate(ModelMatrix, Position);

    ModelMatrix = glm::rotate(ModelMatrix, glm::radians(Rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    ModelMatrix = glm::rotate(ModelMatrix, glm::radians(Rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    ModelMatrix = glm::rotate(ModelMatrix, glm::radians(Rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

    ModelMatrix = glm::scale(ModelMatrix, Scale);

    return ModelMatrix;
}

glm::mat3 KH_Object::UpdateNormalMatrix()
{
    NormalMatrix =  GetNormalMatrix(ModelMatrix);
    return NormalMatrix;
}

void KH_Object::OnTransformChanged()
{
    UpdateModelMatrix();
    UpdateNormalMatrix();
}

void KH_Hitable::OnTransformChanged()
{
	KH_Object::OnTransformChanged();
    UpdateAABB();
}

glm::vec3 KH_Primitive::GetMinPos() const
{
    return AABB.MinPos;
}

glm::vec3 KH_Primitive::GetMaxPos() const
{
    return AABB.MaxPos;
}

glm::vec3 KH_Primitive::GetAABBCenter() const
{
    return AABB.GetCenter();
}

bool KH_Primitive::Cmpx(const KH_Primitive& p1, const KH_Primitive& p2)
{
    return p1.GetAABBCenter().x < p2.GetAABBCenter().x;
}

bool KH_Primitive::Cmpy(const KH_Primitive& p1, const KH_Primitive& p2)
{
    return p1.GetAABBCenter().y < p2.GetAABBCenter().y;
}

bool KH_Primitive::Cmpz(const KH_Primitive& p1, const KH_Primitive& p2)
{
    return p1.GetAABBCenter().z < p2.GetAABBCenter().z;
}

bool KH_Primitive::CmpxPtr(const std::unique_ptr<KH_Primitive>& p1, const std::unique_ptr<KH_Primitive>& p2)
{
    return Cmpx(*p1, *p2);
}

bool KH_Primitive::CmpyPtr(const std::unique_ptr<KH_Primitive>& p1, const std::unique_ptr<KH_Primitive>& p2)
{
    return Cmpy(*p1, *p2);
}

bool KH_Primitive::CmpzPtr(const std::unique_ptr<KH_Primitive>& p1, const std::unique_ptr<KH_Primitive>& p2)
{
    return Cmpz(*p1, *p2);
}

bool KH_ScenePrimitive::Cmpx(const KH_ScenePrimitive& p1, const KH_ScenePrimitive& p2)
{
    return KH_Primitive::CmpxPtr(p1.Primitive, p2.Primitive);
}

bool KH_ScenePrimitive::Cmpy(const KH_ScenePrimitive& p1, const KH_ScenePrimitive& p2)
{
    return KH_Primitive::CmpyPtr(p1.Primitive, p2.Primitive);
}

bool KH_ScenePrimitive::Cmpz(const KH_ScenePrimitive& p1, const KH_ScenePrimitive& p2)
{
    return KH_Primitive::CmpzPtr(p1.Primitive, p2.Primitive);
}

KH_HitResult KH_ScenePrimitive::Hit(KH_Ray& Ray) const
{
    return Primitive->Hit(Ray);
}

KH_Triangle::KH_Triangle()
{
    PrimitiveType = KH_PrimitiveType::Triangle;

    P1 = P2 = P3 = glm::vec3(0.0f, 0.0f, 0.0f);
    N1 = N2 = N3 = glm::vec3(0.0f, -1.0f, 0.0f);

    UpdateModelMatrix();
    UpdateNormalMatrix();
    UpdateAABB();
    UpdateOtherData();
}

KH_Triangle::KH_Triangle(glm::vec3 P1, glm::vec3 P2, glm::vec3 P3)
	: P1(P1), P2(P2), P3(P3)
{
    PrimitiveType = KH_PrimitiveType::Triangle;

    const glm::vec3 P1P2 = P2 - P1;
    const glm::vec3 P1P3 = P3 - P1;
    N1 = N2 = N3 = glm::normalize(glm::cross(P1P2, P1P3));

    UpdateModelMatrix();
    UpdateNormalMatrix();
    UpdateAABB();
    UpdateOtherData();
}

KH_Triangle::KH_Triangle(glm::vec3 P1, glm::vec3 P2, glm::vec3 P3, glm::vec3 N1, glm::vec3 N2, glm::vec3 N3)
    : P1(P1), P2(P2), P3(P3), N1(N1), N2(N2), N3(N3)
{
    PrimitiveType = KH_PrimitiveType::Triangle;

    UpdateModelMatrix();
    UpdateNormalMatrix();
    UpdateAABB();
    UpdateOtherData();
}


KH_HitResult KH_Triangle::Hit(KH_Ray& Ray)
{
    // MT算法
    KH_TriangleWorldData Data = GetWorldData();

    KH_HitResult result;
    glm::vec3 edge1 = Data.P2 - Data.P1;
    glm::vec3 edge2 = Data.P3 - Data.P1;
    glm::vec3 pvec = glm::cross(Ray.Direction, edge2);
    float det = glm::dot(edge1, pvec);

    if (std::abs(det) < EPS) return result;

    float invDet = 1.0f / det;

    glm::vec3 tvec = Ray.Start - Data.P1;
    float u = glm::dot(tvec, pvec) * invDet;
    if (u < 0.0f || u > 1.0f) return result;

    glm::vec3 qvec = glm::cross(tvec, edge1);
    float v = glm::dot(Ray.Direction, qvec) * invDet;
    if (v < 0.0f || u + v > 1.0f) return result;

    float t = glm::dot(edge2, qvec) * invDet;
    if (t < EPS) return result; 

    result.bIsHit = true;
    result.Distance = t;
    result.HitPoint = Ray.Start + t * Ray.Direction;

    float w1 = 1.0f - u - v;
    result.Normal = glm::normalize(w1 * Data.N1 + u * Data.N2 + v * Data.N3);

    return result;
}

uint32_t KH_Triangle::GetPrimitiveCount() const
{
    return 1;
}

void KH_Triangle::EncodePrimitives(std::vector<KH_PrimitiveEncoded>& outPrimitives, int MaterialSlotID) const
{
    KH_TriangleWorldData Data = GetWorldData();

    KH_PrimitiveEncoded primitive{};

    primitive.Triangle.P1 = glm::vec4(Data.P1, 1.0f);
    primitive.Triangle.P2 = glm::vec4(Data.P2, 1.0f);
    primitive.Triangle.P3 = glm::vec4(Data.P3, 1.0f);

    primitive.Triangle.N1 = glm::vec4(Data.N1, 0.0f);
    primitive.Triangle.N2 = glm::vec4(Data.N2, 0.0f);
    primitive.Triangle.N3 = glm::vec4(Data.N3, 0.0f);

    primitive.PrimitiveType = glm::ivec2(static_cast<int>(KH_PrimitiveType::Triangle), 0);
    primitive.MaterialSlotID = glm::ivec2(MaterialSlotID, 0);

    outPrimitives.push_back(primitive);
}

void KH_Triangle::CollectPrimitives(std::vector<KH_ScenePrimitive>& outPrimitives, int MaterialSlotID) const
{
    outPrimitives.emplace_back(KH_ScenePrimitive{
    std::make_unique<KH_Triangle>(*this),
    MaterialSlotID
        });
}

void KH_Triangle::CollectPrimitiveAABBCenters(std::vector<glm::vec4>& outCenters) const
{
    outCenters.push_back(glm::vec4(AABB.GetCenter(), 1.0f));
}

glm::vec3 KH_Triangle::GetCenterWS() const
{
    return CenterWS;
}

void KH_Triangle::UpdateAABB()
{
    KH_TriangleWorldData Data = GetWorldData();
    AABB.MinPos = glm::min(Data.P1, glm::min(Data.P2, Data.P3));
    AABB.MaxPos = glm::max(Data.P1, glm::max(Data.P2, Data.P3));
}

void KH_Triangle::UpdateOtherData()
{
    KH_TriangleWorldData Data = GetWorldData();
    CenterWS = (Data.P1 + Data.P2 + Data.P3) / 3.0f;
    NormalWS = glm::normalize(glm::cross(Data.P2 - Data.P1, Data.P3 - Data.P1));
}

void KH_Triangle::OnTransformChanged()
{
	KH_Primitive::OnTransformChanged();
    UpdateOtherData();
}

KH_Triangle::KH_TriangleHitInfo KH_Triangle::CheckHitInfo(glm::vec3 HitPoint)
{
    KH_TriangleWorldData Data = GetWorldData();

    const glm::vec3 P1P2 = Data.P2 - Data.P1;
    const glm::vec3 P1P3 = Data.P3 - Data.P1;
    const glm::vec3 P1P = HitPoint - Data.P1;

    float Dot00 = glm::dot(P1P3, P1P3);
    float Dot01 = glm::dot(P1P3, P1P2);
    float Dot02 = glm::dot(P1P3, P1P);
    float Dot11 = glm::dot(P1P2, P1P2);
    float Dot12 = glm::dot(P1P2, P1P);

    float Denom = Dot00 * Dot11 - Dot01 * Dot01;

    KH_TriangleHitInfo HitInfo;
    HitInfo.bIsHit = false;

    if (std::abs(Denom) < EPS)
        return HitInfo;

    float InvDenom = 1.0f / Denom;

    float u = (Dot11 * Dot02 - Dot01 * Dot12) * InvDenom;
    float v = (Dot00 * Dot12 - Dot01 * Dot02) * InvDenom;

    if (u >= 0 && v >= 0 && u + v <= 1) {
        HitInfo.bIsHit = true;
        HitInfo.w1 = 1 - u - v;
        HitInfo.w2 = u;
        HitInfo.w3 = v;
        return HitInfo;
    }
    return HitInfo;
}

KH_Triangle::KH_TriangleWorldData KH_Triangle::GetWorldData() const
{
    KH_TriangleWorldData data;
    data.P1 = glm::vec3(ModelMatrix * glm::vec4(P1, 1.0f));
    data.P2 = glm::vec3(ModelMatrix * glm::vec4(P2, 1.0f));
    data.P3 = glm::vec3(ModelMatrix * glm::vec4(P3, 1.0f));

    data.N1 = glm::normalize(NormalMatrix * N1);
    data.N2 = glm::normalize(NormalMatrix * N2);
    data.N3 = glm::normalize(NormalMatrix * N3);
    return data;
}
