# InBang

Unreal Engine 5.8 기반 버튜버 방송 무대 프로젝트.

> 이전 UE 5.7 + 네페르 모델 버전은 태그 `archive/ue5.7-nefer`에 보관되어 있습니다.
> `git checkout archive/ue5.7-nefer` (LFS 필요)

## 구성

- `InBang/InBang.uproject` — UE 5.8 C++ 프로젝트
- `InBang/Source/InBang/MyPlayerController` — 방송 컨트롤러
  - 레벨에서 아바타(태그 `Talent`)를 찾아 버스트샷 카메라 자동 구성 (그린스크린 배경은 선택, 기본 꺼짐)
  - Numpad 0~9 표정 프리셋, 자동 눈 깜빡임
  - 콘솔(`~`): `ListMorphs`, `Expr <모프> <값>`, `Face <번호>`, `AutoBlink`, `CamDist`, `CamHeight`, `CamFocal`
- `InBang/Config/DefaultGame.ini` — 표정 프리셋, 카메라, 깜빡임 설정 (기본값은 ARKit 블렌드셰이프 이름)
- `run-avatar-capture.cmd` — 방송 화면 실행 (1920x1080·60fps 창 모드, OBS 게임 캡처용, 포커스 없이도 넘패드 동작)

## 처음 세팅

1. Epic Games Launcher에서 UE 5.8 설치
2. `InBang.uproject` 우클릭 → Generate Visual Studio project files → 열기 (C++ 빌드)
3. 아바타 모델 임포트 → 레벨에 배치 → 액터 태그 `Talent` 추가
4. 레벨을 `/Game/Lvl_Stage`로 저장하고 `DefaultEngine.ini`의 맵 설정 주석 해제
5. 콘솔에서 `ListMorphs`로 모프 이름 확인 → `DefaultGame.ini` 표정 프리셋 수정

`Saved`, `Intermediate`, `DerivedDataCache`, `Binaries` 등 생성 폴더는 git에서 제외됩니다.
