# InBang

Unreal Engine 5.8 버튜버 방송 무대. 언리얼 화면을 그대로 OBS 게임 캡처로 치지직에 송출한다.

이전 UE 5.7 + 네페르 버전: 태그 `archive/ue5.7-nefer` (LFS 필요)

## 처음 세팅

1. UE 5.8 설치
2. `InBang/InBang.uproject` 우클릭 → Generate Visual Studio project files → 빌드
3. 아바타를 레벨에 배치하고 액터 태그 `Talent` 추가
4. 레벨을 `/Game/Lvl_Stage`로 저장하고 `InBang/Config/DefaultEngine.ini`의 `[/Script/EngineSettings.GameMapsSettings]`에 추가
   ```ini
   EditorStartupMap=/Game/Lvl_Stage.Lvl_Stage
   GameDefaultMap=/Game/Lvl_Stage.Lvl_Stage
   ```
5. 콘솔(`~`)에서 `ListMorphs`로 모프 이름 확인 → `InBang/Config/DefaultGame.ini`의 `ExpressionPresets` 수정

## 방송

- `run-avatar-capture.cmd` 실행 → OBS 게임 캡처로 창 `InBang Avatar Capture` 지정
- 언리얼 효과음은 OBS 애플리케이션 오디오 캡처로 받는다
- Numpad 0~9: `ExpressionPresets` 순서대로 표정 전환 (창 포커스가 없어도 동작)

## 콘솔 명령

`Face <번호>` · `ListMorphs` · `Expr <모프> <가중치>` · `AutoBlink <true|false>` · `CamDist <cm>` · `CamHeight <0~1>` · `CamFocal <mm>`

설정 값의 의미와 단위는 `InBang/Source/InBang/MyPlayerController.h`의 프로퍼티 이름과 `Units`/`Clamp` 메타데이터가 기준이다.
