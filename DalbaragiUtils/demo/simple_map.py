import dalutils.map.mapbuilder as mbu
import dalutils.map.meshbuilder as mes
import dalutils.map.primitives as pri


mapbuild = mbu.MapChunkBuilder()  # 맵 전체의 데이터를 담는 마스터 객체

################

model = mapbuild.newEmbeddedModel()  # Embedded model은 파이썬에서 mesh를 직접 정의한 것
model.m_flagDetailedCollider.set(True)  # 이게 있어야 충돌 가능, 버그
model.m_name.set("floor")  # 모델의 이름

unit = model.newRenderUnit()  # 렌더유닛은 복수개의 삼각형과 하나의 메테리얼로 구성됨

# 삼각형의 정점을 하나하나 입력하는 건 불가능
# 따라서 mesh builder를 이용
# Rect는 점 4개를 입력하여 사각형 하나를 만들어냄
halfSize = 50
unit.m_mesh = mes.Rect(
    pri.Vec3(-halfSize, 0, -halfSize),
    pri.Vec3(-halfSize, 0, halfSize),
    pri.Vec3(halfSize, 0, halfSize),
    pri.Vec3(halfSize, 0, -halfSize)
)

# 메테리얼
unit.m_material.m_diffuseMap.set("asset::rustediron2_basecolor.png")  # 색깔 맵
unit.m_material.m_roughnessMap.set("asset::rustediron2_roughness.png")  # 러프니스 맵
unit.m_material.m_metallicMap.set("asset::rustediron2_metallic.png")  # 메탈릭 맵
unit.m_material.m_texScaleX.set(halfSize / 10)  # 텍스처 크기를 줄이기 위해
unit.m_material.m_texScaleY.set(halfSize / 10)

# 모델 하나에는 액터가 여러 개 있을 수 있다
# 가령, 롤의 미니언은 똑같이 생긴 게 여러 마리 있을 수 있다
# 미니언 모델 하나에 액터를 여러 개 붙이면 이를 구현할 수 있다
# 물론 최소한 액터가 하나는 있어야 게임 안에서 볼 수 있다
actor = model.newStaticActor()  # 액터 생성
actor.m_name.set("actor1")  # 액터 이름 설정, 모델 이름과 혼동하지 말 것
actor.m_transform.m_pos.setXYZ(0, -2, 0)  # 액터의 위치를 설정

################

# 블렌더로 만든 모델을 불러올 때는 Imported model을 사용함
model = mapbuild.newImportedModel()
model.m_resourceID.set("asset::yuri_cso2.obj")  # 불러올 모델의 ID를 지정
# ID를 지정하는 방식이 조금 특별하니 제대로 숙지할 것

# 마찬가지로 액터를 생성
actor = model.newStaticActor()
actor.m_name.set("main_actor")
actor.m_transform.m_pos.setX(0)
actor.m_transform.m_pos.setY(-2)
actor.m_transform.m_pos.setZ(-5)

################

# 맵 데이터를 json 파일로 출력
mbu.exportJson(mapbuild, "demo/intermediates/simple_map.json")
