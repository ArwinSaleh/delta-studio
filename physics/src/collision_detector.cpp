#include "../include/collision_detector.h"

#include <float.h>
#include <math.h>
#include <algorithm>

#define THRESH_0_POSITIVE 10e-9
#define THRESH_0_NEGATIVE (-THRESH_0_POSITIVE)

dphysics::CollisionDetector::CollisionDetector() {
    /* void */
}

dphysics::CollisionDetector::~CollisionDetector() {
    /* void */
}

bool dphysics::CollisionDetector::CircleCircleIntersect(RigidBody *body1, RigidBody *body2, CirclePrimitive *circle1, CirclePrimitive *circle2) {
    ysVector dif = ysMath::Sub(circle1->Position, circle2->Position);
    ysVector distSq = ysMath::MagnitudeSquared3(dif);

    float combinedRadius = circle1->Radius + circle2->Radius;

    if (ysMath::GetScalar(distSq) <= (combinedRadius * combinedRadius)) {
        return true;
    }

    return false;
}

int dphysics::CollisionDetector::BoxBoxCollision(
    Collision *collisions, RigidBody *body1, RigidBody *body2, BoxPrimitive *box1, BoxPrimitive *box2) 
{
    Collision collisions1[2];
    Collision collisions2[2];

    bool colliding = _BoxBoxColliding(box1, box2);
    if (!colliding) return 0;

    int n1 = BoxBoxVertexPenetration(collisions1, box1, box2);
    int n2 = BoxBoxVertexPenetration(collisions2, box2, box1);

    float minPenetration1 = (n1 > 0)
        ? collisions1->m_penetration
        : FLT_MAX;
    float minPenetration2 = (n2 > 0)
        ? collisions2->m_penetration
        : FLT_MAX;

    int n = 0;
    if (n1 > 0) {
        if (minPenetration2 >= minPenetration1) {
            for (int i = 0; i < n1; ++i) {
                collisions[i] = collisions1[i];
                collisions[i].m_body1 = body1;
                collisions[i].m_body2 = body2;
            }
            n = n1;
        }
    }

    if (n2 > 0) {
        if (minPenetration2 <= minPenetration1) {
            for (int i = 0; i < n2; ++i) {
                collisions[i] = collisions2[i];
                collisions[i].m_body1 = body2;
                collisions[i].m_body2 = body1;
            }
            n = n2;
        }
    }

    return n;
}

int dphysics::CollisionDetector::CircleBoxCollision(Collision *collisions, RigidBody *body1, RigidBody *body2, CirclePrimitive *circle, BoxPrimitive *box) {
    constexpr float epsilon = 1E-5f;

    ysVector relativePosition = ysMath::Sub(circle->Position, box->Position);
    relativePosition = ysMath::QuatTransformInverse(box->Orientation, relativePosition);

    float closestX = min(max(ysMath::GetX(relativePosition), -box->HalfWidth), box->HalfWidth);
    float closestY = min(max(ysMath::GetY(relativePosition), -box->HalfHeight), box->HalfHeight);

    ysVector closestPoint = ysMath::LoadVector(closestX, closestY, ysMath::GetZ(relativePosition));
    ysVector realPosition = ysMath::QuatTransform(box->Orientation, closestPoint);
    realPosition = ysMath::Add(realPosition, box->Position);
    
    float d0 = ysMath::GetScalar(ysMath::MagnitudeSquared3(ysMath::Sub(circle->Position, box->Position)));
    float d2 = ysMath::GetScalar(ysMath::MagnitudeSquared3(ysMath::Sub(circle->Position, realPosition)));
    if (d2 > circle->Radius * circle->Radius) return 0;

    ysVector normal;
    if (d0 <= epsilon) {
        normal = ysMath::Mul(ysMath::Constants::XAxis, ysMath::LoadScalar(0.001f));
    }
    else if (d2 <= epsilon) {
        normal = ysMath::Mask(ysMath::Sub(circle->Position, box->Position), ysMath::Constants::MaskOffW);
        normal = ysMath::Mask(normal, ysMath::Constants::MaskOffZ);
    }
    else {
        normal = ysMath::Mask(ysMath::Sub(circle->Position, realPosition), ysMath::Constants::MaskOffW);
        normal = ysMath::Mask(normal, ysMath::Constants::MaskOffZ);
    }

    collisions[0].m_normal = ysMath::Normalize(normal);
    collisions[0].m_body1 = body1;
    collisions[0].m_body2 = body2;
    collisions[0].m_penetration = circle->Radius - ysMath::GetScalar(ysMath::Magnitude(normal));
    collisions[0].m_position = realPosition;
    collisions[0].m_collisionType = Collision::CollisionType::Generic;
    
    return 1;
}

int dphysics::CollisionDetector::CircleCircleCollision(Collision *collisions, RigidBody *body1, RigidBody *body2, CirclePrimitive *circle1, CirclePrimitive *circle2) {
    ysVector delta = ysMath::Sub(circle2->Position, circle1->Position);
    delta = ysMath::Mask(delta, ysMath::Constants::MaskOffZ);

    ysVector direction = ysMath::Normalize(delta);
    ysVector distance = ysMath::Magnitude(delta);

    float s_distance = ysMath::GetScalar(distance);

    // Special case handling
    if (s_distance == 0) {
        direction = ysMath::Constants::XAxis;
        s_distance = 0.01f;
    }

    float combinedRadius = circle1->Radius + circle2->Radius;

    if ((s_distance * s_distance) < combinedRadius * combinedRadius) {
        collisions[0].m_body1 = body1;
        collisions[0].m_body2 = body2;
        collisions[0].m_normal = ysMath::Negate3(direction);
        collisions[0].m_penetration = circle1->Radius + circle2->Radius - s_distance;
        collisions[0].m_position =
            ysMath::Add(circle1->Position, ysMath::Mul(direction, ysMath::LoadScalar(circle1->Radius)));
        collisions[0].m_collisionType = Collision::CollisionType::Generic;

        return true;
    }

    return false;
}

int dphysics::CollisionDetector::RayBoxCollision(Collision *collisions, RigidBody *body1, RigidBody *body2, RayPrimitive *ray, BoxPrimitive *box) {
    /* TODO */

    return 0;
}

int dphysics::CollisionDetector::RayCircleCollision(Collision *collisions, RigidBody *body1, RigidBody *body2, RayPrimitive *ray, CirclePrimitive *circle) {
    ysVector D = ray->Direction;
    ysVector P = ray->Position;
    ysVector C = circle->Position;
    ysVector DP = ysMath::Sub(P, C);

    float D_dot_DP = ysMath::GetScalar(ysMath::Dot(D, DP));
    float D_mag = ysMath::GetScalar(ysMath::MagnitudeSquared3(D));
    float DP_mag = ysMath::GetScalar(ysMath::MagnitudeSquared3(DP));

    float delta = D_dot_DP * D_dot_DP - D_mag * (DP_mag - circle->Radius * circle->Radius);
    if (delta < 0) return false;

    float t1 = (-D_dot_DP + delta) / D_mag;
    float t2 = (-D_dot_DP - delta) / D_mag;

    if (ray->MaxDistance > 0) {
        if (t1 > ray->MaxDistance && t2 > ray->MaxDistance) return false;
    }

    float closest = 0;
    if (t1 < 0 && t2 < 0) return false;
    else if (t1 < 0) closest = t2;
    else if (t2 < 0) closest = t1;
    else closest = min(t1, t2);

    collisions[0].m_body1 = body1;
    collisions[0].m_body2 = body2;
    collisions[0].m_penetration = closest;
    collisions[0].m_position = ray->Position;
    
    return true;
}

bool dphysics::CollisionDetector::_BoxBoxColliding(BoxPrimitive *a, BoxPrimitive *b) {
    const ysVector Epsilon = ysMath::LoadScalar(1E-4);

    ysQuaternion o_a = a->Orientation;
    ysQuaternion o_b = b->Orientation;

    ysMatrix r;
    ysMatrix abs_r;

    float ra, rb;

    ysQuaternion b_in_a = ysMath::QuatMultiply(ysMath::QuatInvert(o_a), o_b);
    r = ysMath::LoadMatrix(b_in_a);

    ysVector t_v = ysMath::Sub(b->Position, a->Position);
    t_v = ysMath::MatMult(r, t_v);
    ysVector2 t = ysMath::GetVector2(t_v);

    for (int i = 0; i < 4; ++i) {
        abs_r.rows[i] = ysMath::Add(ysMath::Abs(r.rows[i]), Epsilon);
    }

    ysVector a_extents = ysMath::LoadVector(a->HalfWidth, a->HalfHeight);
    ysVector b_extents = ysMath::LoadVector(b->HalfWidth, b->HalfHeight);

    for (int i = 0; i < 2; ++i) {
        ra = a->Extents[i];
        rb = ysMath::GetScalar(ysMath::Dot(abs_r.rows[i], b_extents));
        if (abs(t.vec[i]) > ra + rb) return false;
    }

    for (int i = 0; i < 2; ++i) {
        ra = ysMath::GetScalar(ysMath::Dot(abs_r.rows[i], a_extents));
        rb = b->Extents[i];
        if (abs(t.vec[i]) > ra + rb) return false;
    }

    return true;
}

void swap(int &a, int &b) {
    int temp = a;
    a = b;
    b = temp;
}

void sort4(const float *d, int *index) {
    if (d[0] < d[1]) swap(index[0], index[1]);
    if (d[2] < d[3]) swap(index[2], index[3]);
    if (d[0] > d[2]) swap(index[0], index[2]);
    if (d[1] > d[3]) swap(index[1], index[3]);
    if (d[1] > d[2]) swap(index[1], index[2]);
}

float vertexBoxCollision(float proj_x, float proj_y, float halfWidth, float halfHeight) {
    if (abs(proj_x) > halfWidth) return FLT_MAX;
    if (proj_y > halfHeight) return FLT_MAX;

    return halfHeight - proj_y;
}

int dphysics::CollisionDetector::BoxBoxVertexPenetration(
    Collision *collisions, BoxPrimitive *a, BoxPrimitive *b) 
{
    ysQuaternion a_inv = ysMath::QuatInvert(a->Orientation);
    ysQuaternion b_to_a = ysMath::QuatMultiply(
        a_inv,
        b->Orientation);

    ysVector d = ysMath::Sub(b->Position, a->Position);
    ysVector d_rel = ysMath::QuatTransform(a_inv, d);

    ysVector v1 = ysMath::Add(ysMath::QuatTransform(b_to_a, ysMath::LoadVector(b->HalfWidth, b->HalfHeight)), d_rel);
    ysVector v2 = ysMath::Add(ysMath::QuatTransform(b_to_a, ysMath::LoadVector(-b->HalfWidth, b->HalfHeight)), d_rel);
    ysVector v3 = ysMath::Add(ysMath::QuatTransform(b_to_a, ysMath::LoadVector(b->HalfWidth, -b->HalfHeight)), d_rel);
    ysVector v4 = ysMath::Add(ysMath::QuatTransform(b_to_a, ysMath::LoadVector(-b->HalfWidth, -b->HalfHeight)), d_rel);

    float proj_x[] = {
        ysMath::GetX(v1),
        ysMath::GetX(v2),
        ysMath::GetX(v3),
        ysMath::GetX(v4)
    };

    int order_x[] = { 0, 1, 2, 3 };

    float proj_y[] = {
        ysMath::GetY(v1),
        ysMath::GetY(v2),
        ysMath::GetY(v3),
        ysMath::GetY(v4)
    };

    int order_y[] = { 0, 1, 2, 3 };

    sort4(proj_x, order_x);
    sort4(proj_y, order_y);

    float smallestPenetration = FLT_MAX;
    int vertex = -1;
    ysVector normal;

    float penetration;

    // Top face
    penetration = vertexBoxCollision(proj_x[order_y[0]], proj_y[order_y[0]], b->HalfWidth, b->HalfHeight);
    if (penetration < smallestPenetration) {
        vertex = order_y[0];
        normal = ysMath::LoadVector(0.0f, 1.0f, 0.0f);
    }

    // Bottom face
    penetration = vertexBoxCollision(proj_x[order_y[3]], -proj_y[order_y[3]], b->HalfWidth, -b->HalfHeight);
    if (penetration < smallestPenetration) {
        vertex = order_y[3];
        normal = ysMath::LoadVector(0.0f, -1.0f, 0.0f);
    }

    // Left face
    penetration = vertexBoxCollision(proj_y[order_x[3]], -proj_x[order_x[3]], b->HalfHeight, -b->HalfWidth);
    if (penetration < smallestPenetration) {
        vertex = order_y[3];
        normal = ysMath::LoadVector(-1.0f, 0.0f, 0.0f);
    }

    // Right face
    penetration = vertexBoxCollision(proj_y[order_x[0]], proj_x[order_x[0]], b->HalfHeight, b->HalfWidth);
    if (penetration < smallestPenetration) {
        vertex = order_y[0];
        normal = ysMath::LoadVector(1.0f, 0.0f, 0.0f);
    }

    if (vertex == -1) return 0;

    ysVector position = ysMath::LoadVector(proj_x[vertex], proj_y[vertex]);
    position = ysMath::QuatTransform(a->Orientation, position);
    normal = ysMath::QuatTransform(a->Orientation, normal);

    collisions[0].m_position = position;
    collisions[0].m_normal = normal;
    collisions[0].m_penetration = penetration;
    collisions[0].m_collisionType = Collision::CollisionType::Generic;

    return 1;
}
