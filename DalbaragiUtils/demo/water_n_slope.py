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
        pri.Vec3(-10, 5, -10),
        pri.Vec3(-10, 4, 10),
        pri.Vec3(10, -3, 10),
        pri.Vec3(10, -2, -10),
    )
    unit.m_material.m_diffuseMap.set("asset::0021di.png")
    model.m_renderUnits.pushBack(unit)

    actor = blk.StaticActor()
    actor.m_name.set("slope1")
    model.m_staticActors.pushBack(actor)

    build.m_modelEmbedded.pushBack(model)

    ################

    mbu.exportJson(build, "intermediates/water_n_slope.json")


if __name__ == '__main__':
    main()
