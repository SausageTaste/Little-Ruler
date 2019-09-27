import random

import dalutils.map.primitives as pri
import dalutils.map.mapbuilder as mbu
import dalutils.map.meshbuilder as mes


def main():
    mapbuild = mbu.MapChunkBuilder()

    ################

    model = mapbuild.newEmbeddedModel()
    model.m_flagDetailedCollider.set(True)
    model.m_name.set("terrain")

    unit = model.newRenderUnit()
    builder = mes.HeightGrid(150, 150, 20, 20)

    heightmap = []
    for y in range(builder.m_heightMap.getRowSize()):
        for x in range(builder.m_heightMap.getColumnSize()):
            heightmap.append(random.random())

    builder.m_heightMap.setRowMajor(
        heightmap
    )
    builder.m_heightMap.forEach(lambda h: 4 * h)
    builder.m_smoothShading = True
    unit.m_mesh = builder

    unit.m_material.m_diffuseMap.set("asset::grass1.png")
    unit.m_material.m_texScaleX.set(50)
    unit.m_material.m_texScaleY.set(50)
    unit.m_material.m_shininess.set(32)
    unit.m_material.m_specStreng.set(0.2)

    actor = model.newStaticActor()
    actor.m_name.set("main_actor")
    actor.m_transform.m_pos.setX(0)
    actor.m_transform.m_pos.setY(-6)

    ################

    water = mapbuild.newWaterPlane()

    water.m_centerPos.setXYZ(0, -4, 0)
    water.m_width.set(500)
    water.m_height.set(500)
    water.m_darkestDepth.set(10)
    water.m_reflectivity.set(0.2)
    water.m_deepColor.setXYZ(0.09, 0.07, 0.11)

    ################

    model = mapbuild.newImportedModel()
    model.m_resourceID.set("asset::house_test.obj")

    actor = model.newStaticActor()
    actor.m_name.set("main_actor")
    actor.m_transform.m_pos.setXYZ(-10, -3, -10)
    actor.m_transform.m_scale.set(2)
    actor.m_transform.m_quat.rotate(180, (0, 1, 0))

    ################

    model = mapbuild.newImportedModel()
    model.m_resourceID.set("asset::yuri_cso2.obj")

    actor = model.newStaticActor()
    actor.m_name.set("main_actor")
    actor.m_transform.m_pos.setX(3)
    actor.m_transform.m_pos.setY(-3)
    actor.m_transform.m_pos.setZ(-5)
    actor.m_transform.m_scale.set(3)

    ################

    light = mapbuild.newPointLight()
    light.m_maxDistance.set(5)
    light.m_color.setXYZ(5, 5, 5)
    light.m_pos.setXYZ(3, 0, 2)

    ################

    mbu.exportJson(mapbuild, "demo/intermediates/water_n_slope.json")


if __name__ == '__main__':
    main()
