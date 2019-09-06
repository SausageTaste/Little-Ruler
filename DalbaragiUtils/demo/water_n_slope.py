import random

import dalutils.map.primitives as pri
import dalutils.map.datablock as blk
import dalutils.map.mapbuilder as mbu
import dalutils.map.meshbuilder as mes


def main():
    mapbuild = mbu.MapChunkBuilder()

    ################

    model = mapbuild.newEmbeddedModel()
    model.m_flagDetailedCollider.set(True)
    model.m_name.set("Slope")

    unit = blk.RenderUnit()
    unit.m_mesh.buildIn_rect(
        pri.Vec3(-15, 70, -10),
        pri.Vec3(-15, 70, 10),
        pri.Vec3(15, -15, 10),
        pri.Vec3(15, -15, -10),
    )
    unit.m_material.m_diffuseMap.set("asset::0021di.png")
    unit.m_material.m_texScaleX.set(45)
    unit.m_material.m_texScaleY.set(10)
    model.m_renderUnits.pushBack(unit)

    actor = blk.StaticActor()
    actor.m_name.set("slope_actor")
    actor.m_transform.m_pos.setX(-20)
    actor.m_transform.m_pos.setY(-5)
    model.m_staticActors.pushBack(actor)

    ################

    model = mapbuild.newEmbeddedModel()
    model.m_flagDetailedCollider.set(True)
    model.m_name.set("Slope2")

    unit = blk.RenderUnit()
    builder = mes.HeightGrid(100, 100, 15, 15)

    heightmap = []
    for y in range(builder.m_heightMap.getSizeY()):
        row = [0.0]
        for x in range(builder.m_heightMap.getSizeX() - 1):
            row.append(random.random())
        heightmap.append(row)

    builder.m_heightMap.setList(
        heightmap
    )
    builder.m_heightMap.applyFunc(lambda x: 3 * x)
    builder.m_smoothShading = True
    unit.m_mesh.buildIn(builder)

    unit.m_material.m_diffuseMap.set("asset::grass1.png")
    unit.m_material.m_texScaleX.set(50)
    unit.m_material.m_texScaleY.set(50)
    unit.m_material.m_shininess.set(8)
    unit.m_material.m_specStreng.set(0.5)
    model.m_renderUnits.pushBack(unit)

    actor = blk.StaticActor()
    actor.m_name.set("slope_actor")
    actor.m_transform.m_pos.setX(0)
    actor.m_transform.m_pos.setY(-9)
    model.m_staticActors.pushBack(actor)

    ################

    water = mapbuild.newWaterPlane()

    water.m_centerPos.setXYZ(0, -7.5, 0)
    water.m_width.set(100)
    water.m_height.set(100)
    water.m_darkestDepth.set(10)
    water.m_reflectivity.set(0.1)

    ################

    mbu.exportJson(mapbuild, "demo/intermediates/water_n_slope.json")


if __name__ == '__main__':
    main()
