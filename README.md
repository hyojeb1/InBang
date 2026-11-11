# InBang

Unreal Engine 5.8 버튜버 방송 무대. 언리얼 화면을 그대로 OBS 게임 캡처로 치지직에 송출한다.

이전 UE 5.7 + 네페르 버전: 태그 `archive/ue5.7-nefer` (LFS 필요)

## 처음 세팅

1. UE 5.8 설치 후 `verify.ps1` 실행 (빌드 + 헤드리스 검증)
2. 탤런트를 새로 만들 때만: 아래 명령으로 MetaHuman 생성 → Auto-Rig → 텍스처 → 조립 → `Lvl_Stage` 배치까지 실행한다. Auto-Rig에서 브라우저 Epic 로그인이 열리며 10분 안에 허용까지 눌러야 한다. 조립은 TextureGraph가 커맨드릿에서 꺼지므로 `UnrealEditor-Cmd -run=pythonscript`가 아니라 에디터 창으로 실행해야 한다.
3. `verify.ps1` → 마지막 줄이 `VERIFY result=PASS`인지 확인

```powershell
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe' InBang\InBang.uproject -ExecutePythonScript="InBang/Scripts/make_talent_metahuman.py"
```

검증 로그의 `VERIFY bind`가 실패하고 얼굴 스켈레톤(`skeleton=` 값)이 바뀌었다면 얼굴 ABP를 그 스켈레톤으로 다시 만든다.

```powershell
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' InBang\InBang.uproject -run=pythonscript -script="InBang/Scripts/make_talent_face_abp.py <스켈레톤 경로>"
```

## 검증

`verify.ps1` 출력은 `LogInBang` 줄만이다. `-SkipBuild`, `-NoStageVerify -ExecCmds "DumpMeshCurves <경로>,quit"`로 빌드 생략·커브 덤프만 할 수 있다. 엔진 전체 로그는 `InBang/Saved/Verify/run.log`.

## 방송

- `run-avatar-capture.cmd` 실행 → OBS 게임 캡처로 창 `InBang Avatar Capture` 지정
- 언리얼 효과음은 OBS 애플리케이션 오디오 캡처로 받는다
- Numpad 0~9: `ExpressionPresets` 순서대로 표정 전환 (창 포커스가 없어도 동작)

## 콘솔 명령

`Face <번호>` · `Expr <커브> <가중치>` · `DumpFaceCurves` · `DumpMeshCurves <SkeletalMesh 경로>` · `AutoBlink <true|false>` · `CamDist <cm>` · `CamHeight <cm>` · `CamFocal <mm>` · `VerifyStage`

설정 값의 의미와 단위는 `InBang/Source/InBang/MyPlayerController.h`의 프로퍼티 이름과 `Units`/`Clamp` 메타데이터가 기준이다. 표정 커브 이름은 `DumpFaceCurves`가 출력하는 `rigControl=` 목록 중에서 고른다.
