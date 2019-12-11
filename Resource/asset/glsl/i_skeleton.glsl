uniform mat4 u_jointTrans[30];


mat4 makeJointTransform(ivec3 jointIDs, vec3 weights) {
    mat4 boneMat = u_jointTrans[jointIDs[0]] * weights[0];

    for ( int i = 1; i < 3; ++i ) {
        int jid = jointIDs[i];
        if (-1 == jid) {
            break;
        }
        boneMat += u_jointTrans[jid] * weights[i];
    }

    return boneMat;
}
