import dalutils.map.primitives as pri
import dalutils.map.datablock as blk
import dalutils.map.objectdef as ode
import dalutils.map.mapbuilder as mbu


def main():
    build = mbu.MapChunkBuilder()

    ################

    model = ode.ModelEmbedded()
    model.m_flagDetailedCollider.set(True)
    model.m_name.set("Slope")

    unit = blk.RenderUnit()
    unit.m_mesh.buildIn_rect(
        pri.Vec3(-10, 4, -10),
        pri.Vec3(-10, 3, 10),
        pri.Vec3(10, -4, 10),
        pri.Vec3(10, -3, -10),
    )
    unit.m_material.m_diffuseMap.set("asset::0021di.png")
    unit.m_material.m_texScaleX.set(20)
    unit.m_material.m_texScaleY.set(20)
    model.m_renderUnits.pushBack(unit)

    actor = blk.StaticActor()
    actor.m_name.set("slope_actor")
    actor.m_transform.m_pos.setY(-5)
    model.m_staticActors.pushBack(actor)

    build.m_modelEmbedded.pushBack(model)

    ################

    model = ode.ModelEmbedded()
    model.m_flagDetailedCollider.set(True)
    model.m_name.set("Slope2")

    xOffset = 0

    unit = blk.RenderUnit()
    unit.m_mesh.buildIn_rect(
        pri.Vec3(10 - xOffset, -3, -10),
        pri.Vec3(10 - xOffset, -4, 10),
        pri.Vec3(15 - xOffset, -3, 10),
        pri.Vec3(15 - xOffset, -3, -10),
    )
    unit.m_material.m_diffuseMap.set("asset::0021di.png")
    unit.m_material.m_texScaleX.set(5)
    unit.m_material.m_texScaleY.set(20)
    model.m_renderUnits.pushBack(unit)

    actor = blk.StaticActor()
    actor.m_name.set("slope_actor")
    actor.m_transform.m_pos.setX(xOffset)
    actor.m_transform.m_pos.setY(-5)
    model.m_staticActors.pushBack(actor)

    build.m_modelEmbedded.pushBack(model)

    ################

    mbu.exportJson(build, "demo/intermediates/water_n_slope.json")


if __name__ == '__main__':
    main()
