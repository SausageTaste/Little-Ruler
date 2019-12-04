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

struct AABB {
    vec3 m_min, m_max;
};


Plane makePlane_fromNormalPoint(vec3 normal, vec3 point) {
    Plane plane;

    plane.m_normal = normalize(normal);
    plane.m_d = dot(plane.m_normal, point);

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


void AABB_getAllPoints(AABB self, inout vec3 result[8]) {
    vec3 p000 = self.m_min;
    vec3 p111 = self.m_max;

    result[0] = p000;  // 000
    result[1] = vec3( p000.x, p000.y, p111.z );  // 001
    result[2] = vec3( p000.x, p111.y, p000.z );  // 010
    result[3] = vec3( p000.x, p111.y, p111.z );  // 011
    result[4] = vec3( p111.x, p000.y, p000.z );  // 100
    result[5] = vec3( p111.x, p000.y, p111.z );  // 101
    result[6] = vec3( p111.x, p111.y, p000.z );  // 110
    result[7] = p111;  // 111
}


Triangle makeTrianglesFromRect(vec3 p1, vec3 p2, vec3 p3, vec3 p4, bool first) {
    Triangle result;

    if (first) {
        result.m_p1 = p1;
        result.m_p2 = p2;
        result.m_p3 = p3;
    }
    else {
        result.m_p1 = p1;
        result.m_p2 = p3;
        result.m_p3 = p4;
    }

    return result;
}


bool collision_seg_plane(Segment ray, Plane plane) {
    vec3 pointA = ray.m_pos;
    vec3 pointB = pointA + ray.m_rel;

    float distA = getSignedDist(plane, pointA);
    float distB = getSignedDist(plane, pointB);

    return (distA * distB) <= 0.0;
}

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
        return vec4(0.0, 1.0, 0.0, 0.0);
    }

    if ( isPointInsideTri(tri, planeCol.xyz) ) {
        return planeCol;
    }
    else {
        return vec4(0.0, 0.0, 1.0, 0.0);
    }
}

vec4 intersect_seg_aabb(Segment seg, AABB aabb) {
    
    vec3 ps[8];
    vec3 p000 = aabb.m_min;
    vec3 p111 = aabb.m_max;
    ps[0] = p000;  // 000
    ps[1] = vec3( p000.x, p000.y, p111.z );  // 001
    ps[2] = vec3( p000.x, p111.y, p000.z );  // 010
    ps[3] = vec3( p000.x, p111.y, p111.z );  // 011
    ps[4] = vec3( p111.x, p000.y, p000.z );  // 100
    ps[5] = vec3( p111.x, p000.y, p111.z );  // 101
    ps[6] = vec3( p111.x, p111.y, p000.z );  // 110
    ps[7] = p111;  // 111

    /*
    Triangle triangles[12];
    triangles[0] = makeTrianglesFromRect(ps[3], ps[1], ps[5], ps[7], true);
    triangles[1] = makeTrianglesFromRect(ps[3], ps[1], ps[5], ps[7], false);
    triangles[2] = makeTrianglesFromRect(ps[7], ps[5], ps[4], ps[6], true);
    triangles[3] = makeTrianglesFromRect(ps[7], ps[5], ps[4], ps[6], false);
    triangles[4] = makeTrianglesFromRect(ps[6], ps[4], ps[0], ps[2], true);
    triangles[5] = makeTrianglesFromRect(ps[6], ps[4], ps[0], ps[2], false);
    triangles[6] = makeTrianglesFromRect(ps[2], ps[0], ps[1], ps[3], true);
    triangles[7] = makeTrianglesFromRect(ps[2], ps[0], ps[1], ps[3], false);
    triangles[8] = makeTrianglesFromRect(ps[2], ps[3], ps[7], ps[6], true);
    triangles[9] = makeTrianglesFromRect(ps[2], ps[3], ps[7], ps[6], false);
    triangles[10] = makeTrianglesFromRect(ps[4], ps[5], ps[1], ps[0], true);
    triangles[11] = makeTrianglesFromRect(ps[4], ps[5], ps[1], ps[0], false);
    */

    Plane planes[6];

    planes[0] = makePlane_fromPoints(ps[3], ps[1], ps[5]);
    planes[1] = makePlane_fromPoints(ps[7], ps[5], ps[4]);
    planes[2] = makePlane_fromPoints(ps[6], ps[4], ps[0]);
    planes[3] = makePlane_fromPoints(ps[2], ps[0], ps[1]);
    planes[4] = makePlane_fromPoints(ps[2], ps[3], ps[7]);
    planes[5] = makePlane_fromPoints(ps[4], ps[5], ps[1]);

    vec4 result;
    float dist = 99999999.0;

    for ( int i = 0; i < 12; ++i ) {
        vec4 triCol = intersect_seg_plane(seg, planes[i]);
        float iDist = distance(triCol.xyz, seg.m_pos);
        if ( iDist < dist ){
            dist = iDist;
            result = triCol;
        }
    }

    return result;
}
