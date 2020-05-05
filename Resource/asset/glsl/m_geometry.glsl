struct Segment {
    vec3 m_pos;
    vec3 m_rel;
};

struct Plane {
    vec3 m_normal;
    float m_d;
};

struct Triangle {
    vec3 m_p1, m_p2, m_p3;
};

struct Sphere {
    vec3 m_center;
    float m_radius;
};

struct AABB {
    vec3 m_min, m_max;
};


vec3 projectPointOnSeg(Segment seg, vec3 p) {
    const float EPSILON = 1.0e-06;

    vec3 rhs = p - seg.m_pos;
    float magnitude = length(seg.m_rel);
    vec3 lhs = magnitude > EPSILON ? (seg.m_rel / magnitude) : seg.m_rel;
    float num2 = clamp(dot(lhs, rhs), 0.0, magnitude);

    return seg.m_pos + (lhs * num2);
}

vec3 projectPointOnLine(Segment seg, vec3 p) {
    const float EPSILON = 1.0e-06;

    vec3 rhs = p - seg.m_pos;
    float magnitude = length(seg.m_rel);
    vec3 lhs = magnitude > EPSILON ? (seg.m_rel / magnitude) : seg.m_rel;
    float num2 = dot(lhs, rhs);

    return seg.m_pos + (lhs * num2);
}


Plane makePlane_fromNormalPoint(vec3 normal, vec3 point) {
    Plane plane;

    plane.m_normal = normalize(normal);
    plane.m_d = -dot(plane.m_normal, point);

    return plane;
}

Plane makePlane_fromPoints(vec3 p1, vec3 p2, vec3 p3) {
    return makePlane_fromNormalPoint(cross(p2 - p1, p3 - p1), p1);
}

float getSignedDist(Plane self, vec3 v) {
    return dot(vec4(self.m_normal, self.m_d), vec4(v, 1.0));
}

bool isPointInsideTri(Triangle tri, vec3 p) {
    vec3 edge1 = tri.m_p2 - tri.m_p1;
    vec3 edge2 = tri.m_p3 - tri.m_p2;

    vec3 toPoint1 = p - tri.m_p1;
    vec3 toPoint2 = p - tri.m_p2;

    vec3 crossed1 = cross(edge1, toPoint1);
    vec3 crossed2 = cross(edge2, toPoint2);

    float dotted1 = dot(crossed1, crossed2);

    if ( dotted1 < 0.0 ) {
        return false;
    }

    vec3 edge3 = tri.m_p1 - tri.m_p3;
    vec3 toPoint3 = p - tri.m_p3;
    vec3 crossed3 = cross(edge3, toPoint3);
    float dotted2 = dot(crossed1, crossed3);

    if ( dotted2 < 0.0 ) {
        return false;
    }

    return true;
}


bool collision_seg_plane(Segment ray, Plane plane) {
    vec3 pointA = ray.m_pos;
    vec3 pointB = pointA + ray.m_rel;

    float distA = getSignedDist(plane, pointA);
    float distB = getSignedDist(plane, pointB);

    return (distA * distB) <= 0.0;
}

bool collision_seg_aabb(Segment seg, AABB aabb) {
    vec3 bounds[2];
    bounds[0] = aabb.m_min;
    bounds[1] = aabb.m_max;

    vec3 orig = seg.m_pos;
    vec3 invdir = 1.0 / seg.m_rel;
    int sign[3];
    sign[0] = int(invdir.x < 0.0);
    sign[1] = int(invdir.y < 0.0);
    sign[2] = int(invdir.z < 0.0);

    float tmin = (bounds[sign[0]].x - orig.x) * invdir.x;
    float tmax = (bounds[1 - sign[0]].x - orig.x) * invdir.x;

    float tymin = (bounds[sign[1]].y - orig.y) * invdir.y;
    float tymax = (bounds[1 - sign[1]].y - orig.y) * invdir.y;

    if ( (tmin > tymax) || (tymin > tmax) ) {
        return false;
    }
    if ( tymin > tmin ) {
        tmin = tymin;
    }
    if ( tymax < tmax ) {
        tmax = tymax;
    }

    float tzmin = (bounds[sign[2]].z - orig.z) * invdir.z;
    float tzmax = (bounds[1 - sign[2]].z - orig.z) * invdir.z;

    if ( (tmin > tzmax) || (tzmin > tmax) ) {
        return false;
    }
    if ( tzmin > tmin ) {
        tmin = tzmin;
    }
    if ( tzmax < tmax ) {
        tmax = tzmax;
    }

    return true;
}


// xyz : colliding point
// w : wheather colliding or not
vec4 intersect_seg_plane(Segment ray, Plane plane) {
    vec3 pointA = ray.m_pos;
    vec3 pointB = pointA + ray.m_rel;

    float distA = getSignedDist(plane, pointA);
    float distB = getSignedDist(plane, pointB);

    if ((distA * distB) > 0.0) {
        return vec4(1.0, 0.0, 0.0, 0.0);
    }

    float absDistA = abs(distA);
    float dist = length(ray.m_rel) * absDistA / (absDistA + abs(distB));

    return vec4(ray.m_pos + normalize(ray.m_rel) * dist, 1.0);
}

vec4 intersect_seg_tri(Segment ray, Triangle tri) {
    Plane plane = makePlane_fromPoints(tri.m_p1, tri.m_p2, tri.m_p3);
    vec4 planeCol = intersect_seg_plane(ray, plane);
    if ( 0.0 == planeCol.w ) {
        return vec4(0.0);
    }

    if ( isPointInsideTri(tri, planeCol.xyz) ) {
        return planeCol;
    }
    else {
        return vec4(0.0);
    }
}

vec4 interdect_seg_aabb(Segment seg, AABB aabb) {
    if ( !collision_seg_aabb(seg, aabb) ) {
        return vec4(0.0);
    }

    // Top, bottom, left, right, front, back
    Plane planes[6];

    planes[0] = makePlane_fromNormalPoint(vec3(0.0, 1.0, 0.0), aabb.m_max);
    planes[1] = makePlane_fromNormalPoint(vec3(0.0, -1.0, 0.0), aabb.m_min);
    planes[2] = makePlane_fromNormalPoint(vec3(-1.0, 0.0, 0.0), aabb.m_min);
    planes[3] = makePlane_fromNormalPoint(vec3(1.0, 0.0, 0.0), aabb.m_max);
    planes[4] = makePlane_fromNormalPoint(vec3(0.0, 0.0, 1.0), aabb.m_max);
    planes[5] = makePlane_fromNormalPoint(vec3(0.0, 0.0, -1.0), aabb.m_min);

    vec3 colpoint;
    float coldist = 999999999999.99;

    for ( int i = 0; i < 6; ++i ) {
        vec4 result = intersect_seg_plane(seg, planes[i]);
        if ( 0.0 != result.w ) {
            float dist = distance(result.xyz, seg.m_pos);
            if ( dist < coldist ) {
                coldist = dist;
                colpoint = result.xyz;
            }
        }
    }

    return vec4(colpoint, 1.0);
}

vec4 intersect_seg_sphere(Segment seg, Sphere sphere) {
    vec3 nearestPoint = projectPointOnLine(seg, sphere.m_center);
    float nearestDist = distance(sphere.m_center, nearestPoint);
    float distTilAtmostEndSqr = sphere.m_radius*sphere.m_radius - nearestDist*nearestDist;
    if ( distTilAtmostEndSqr <= 0.0 ) {
        return vec4(0.0);
    }

    float distTilAtmostEnd = sqrt(distTilAtmostEndSqr);
    vec3 segRel = normalize(seg.m_rel) * distTilAtmostEnd;
    vec3 endpoint1 = nearestPoint + segRel;
    vec3 endpoint2 = nearestPoint - segRel;

    return dot(seg.m_rel, endpoint1) > 0.0 ? endpoint1 : endpoint2;
}
