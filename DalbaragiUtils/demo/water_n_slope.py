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
            if y < builder.m_heightMap.getRowSize() / 3:
                heightmap.append(-5)
            elif y > builder.m_heightMap.getRowSize() * 2 / 3:
                heightmap.append(5)
            else:
                heightmap.append(0)

    builder.m_heightMap.setRowMajor(
        heightmap
    )
    builder.m_heightMap.forEach(lambda h: 1 * h)
    builder.m_smoothShading = False
    unit.m_mesh = builder

    unit.m_material.m_diffuseMap.set("asset::grass1.tga")
    unit.m_material.m_texScaleX.set(50)
    unit.m_material.m_texScaleY.set(50)

    actor = model.newStaticActor()
    actor.m_name.set("main_actor")

    ################

    water = mapbuild.newWaterPlane()

    water.m_centerPos.setXYZ(0, 0.1, 0)
    water.m_width.set(500)
    water.m_height.set(500)
    water.m_darkestDepth.set(20)
    water.m_reflectivity.set(0.2)
    water.m_deepColor.setXYZ(10/256, 10/256, 30/256)

    ################

    model = mapbuild.newImportedModel()
    model.m_resourceID.set("asset::house_test.obj")

    actor = model.newStaticActor()
    actor.m_name.set("main_actor")
    actor.m_transform.m_pos.setXYZ(-10, 0.2, -10)
    actor.m_transform.m_scale.set(2)
    actor.m_transform.m_quat.rotate(180, (0, 1, 0))

    ################

    model = mapbuild.newImportedModel()
    model.m_resourceID.set("asset::yuri_cso2.obj")

    actor = model.newStaticActor()
    actor.m_name.set("main_actor")
    actor.m_transform.m_pos.setX(3)
    actor.m_transform.m_pos.setY(0)
    actor.m_transform.m_pos.setZ(-5)
    actor.m_transform.m_scale.set(1)

    ################

    light = mapbuild.newPointLight()
    light.m_maxDistance.set(5)
    light.m_color.setXYZ(5, 5, 5)
    light.m_pos.setXYZ(3, 0, 2)

    ################

    model = mapbuild.newEmbeddedModel()
    model.m_name.set("Taeguk")

    unit = model.newRenderUnit()

    unit.m_mesh = mes.Rect(
        pri.Vec3(-1, 1, 0),
        pri.Vec3(-1, -1, 0),
        pri.Vec3(1, -1, 0),
        pri.Vec3(1, 1, 0)
    )
    unit.m_material.m_diffuseMap.set("asset::taeguk.png")

    actor = model.newStaticActor()
    actor.m_name.set("actor1")
    actor.m_transform.m_pos.setX(10)
    actor.m_transform.m_pos.setY(2)
    actor.m_transform.m_pos.setZ(-2)

    actor = model.newStaticActor()
    actor.m_name.set("actor2")
    actor.m_transform.m_pos.setX(10)
    actor.m_transform.m_pos.setY(2)
    actor.m_transform.m_pos.setZ(2)

    actor = model.newStaticActor()
    actor.m_name.set("actor3")
    actor.m_transform.m_pos.setX(10)
    actor.m_transform.m_pos.setY(2)

    ################

    model = mapbuild.newImportedModel()
    model.m_resourceID.set("asset::honoka_maid.dmd")

    actor = model.newStaticActor()
    actor.m_name.set("main_actor")
    actor.m_transform.m_pos.setXYZ(6, 0, -5)
    actor.m_transform.m_scale.set(0.5)

    ################

    model = mapbuild.newEmbeddedModel()
    model.m_name.set("PBR quad")

    unit = model.newRenderUnit()

    unit.m_mesh = mes.Rect(
        pri.Vec3(-1, -1, 0),
        pri.Vec3(-1, 1, 0),
        pri.Vec3(1, 1, 0),
        pri.Vec3(1, -1, 0)
    )
    unit.m_material.m_diffuseMap.set("asset::rustediron2_basecolor.png")
    unit.m_material.m_roughnessMap.set("asset::rustediron2_roughness.png")
    unit.m_material.m_metallicMap.set("asset::rustediron2_metallic.png")

    actor = model.newStaticActor()
    actor.m_name.set("actor1")
    actor.m_transform.m_pos.setXYZ(7, 1, 5)

    ################

    mbu.exportJson(mapbuild, "demo/intermediates/water_n_slope.json")


if __name__ == '__main__':
    main()
