[English](README.md)

# Little Ruler

* 혼자서 진행하고 있는 게임 엔진입니다. 한 번의 코딩으로 안드로이드와 윈도우 모두에서 사용 가능한 엔진을 목표로 하고 있습니다.
* 엔진 소스코드는 모두 `{repo_root}/engine` 안에 있습니다.

![alt text](./screenshots/main.jpg)

<br>

# 제작자

* 우성민, 대학생
* woos8899@gmail.com

<br>

# 깃 레포 첫 폴더 구성물

### engine

* 게임을 이루는 모든 C++ 소스코드는 이곳에 있습니다.

### extern

* 깃 서브모듈 파일이 담기는 곳입니다.
* 여기 있는 건 건드리지 마세요.

### eterntool

* 일부 서브모듈은 CMakeLists.txt가 없어서 제가 직접 만들었습니다.
* 그 CMakeLists.txt 파일들이 이곳에 들어 있습니다.

### LittleRulerAndroid

* 안드로이드 apk로 빌드 하기 위한 안드로이드 스튜디오 프로젝트입니다.
* 안드로이드에서 사용되는 뷰를 정의하기 위한 Java 코드가 들어 있습니다.
* CMake를 통해 `{repo_root}/engine` 안에 있는 C++ 코드를 사용합니다.

### Resource

* 모델, 이미지, 텍스트, 폰트 등 모든 종류의 리소스들은 여기에 넣습니다.

<br>

# 조작 방법

### Windows

* WASD : 수평 이동
* 방향키 : 시점 조작
* 마우스 클릭 후 드래그 : 안드로이드의 터치 조작과 동일하게 시점 조작

### Android

* 화면 좌측 하단의 하얀 점 주변 : 방향 패드를 이용한 이동
* 나머지 빈 공간 : 시야 조작

<br>

# 빌드 방법

* 이 레포를 `--recurse-submodules` 옵션을 적용하여 클론 해 주세요. 아래 예시처럼요.
* `git clone --recurse-submodules -j8 https://github.com/SausageTaste/Little-Ruler`

### Android

* `{repo_root}/LittleRulerAndroid` 폴더를 안드로이드 스튜디오로 열어서 빌드 버튼을 누르시면 됩니다.

### Windows

* CMake로 `{repo_root}/engine/LittleRulerWindows/CMakeLists.txt`를 빌드하세요.
* Visual Studio 2019 컴파일러로만 테스트 해봤습니다.
* 이유는 모르겠는데, 'Build Solution (F7)`을 여러번 눌러야 합니다. 안 그러면 zlibstatic이 발견되지 않는다고 오류가 납니다.

<br>

# 구현

## 플랫폼별

<table>
    <tr>
        <td></td>
        <td>Windows</td>
        <td>Android</td>
    </tr>
    <tr>
        <td>렌더링</td>
        <td>OpenGL 3.0</td>
        <td>OpenGL ES 3.0</td>
    </tr>
    <tr>
        <td>윈도우 생성</td>
        <td>GLFW</td>
        <td>Java GLSurfaceView</td>
    </tr>
    <tr>
        <td>파일 읽기</td>
        <td>Windows API</td>
        <td>Android Asset Manager 및 std::fstream</td>
    </tr>
</table>

<br>

# 관련 프로젝트

* [Dalbaragi Model (DMD) Exporter for Blender](https://github.com/SausageTaste/io_scene_dalbaragi)
