# GitHub Actions에서 Xbox NSP 바로 만들기

이 소스는 GitHub Actions에서 NRO, NSO, NPDM을 빌드한 뒤 **최종 설치용 NSP까지 자동 생성**합니다.
`keys.dat`은 저장소 파일에 넣지 않고 GitHub의 암호화된 Actions Secret에서만 복원하며, 포장이 끝나면 작업 폴더에서 삭제합니다.

## 지원하는 Secret 이름

워크플로는 아래 이름을 순서대로 확인합니다.

### Base64 형식 — 권장

```text
KEYS_DAT_BASE64
PROD_KEYS_BASE64
SWITCH_KEYS_BASE64
```

### 일반 텍스트 형식

```text
KEYS_DAT
PROD_KEYS
SWITCH_KEYS
```

기존에 위 이름 중 하나로 등록했다면 다시 등록할 필요가 없습니다. 다른 이름을 사용했다면 같은 값으로 `KEYS_DAT_BASE64` 또는 `KEYS_DAT`을 하나 추가합니다.

## 권장 등록 방식

Windows PowerShell에서 본인이 관리하는 `keys.dat`을 Base64 한 줄로 변환합니다.

```powershell
[Convert]::ToBase64String([IO.File]::ReadAllBytes("keys.dat"))
```

출력된 문자열 전체를 GitHub 저장소에서 다음 위치에 등록합니다.

```text
Settings
→ Secrets and variables
→ Actions
→ New repository secret
```

이름은 다음을 사용합니다.

```text
KEYS_DAT_BASE64
```

## NSP 생성

1. 수정된 소스를 GitHub 저장소에 업로드합니다.
2. **Actions** 탭을 엽니다.
3. **Build installable Xbox NSP**를 선택합니다.
4. **Run workflow**를 누릅니다.
5. 작업이 끝나면 실행 화면 아래의 **Artifacts**에서 다음 파일을 받습니다.

```text
Xbox-2.0-installable-NSP
```

Artifact 안에는 다음 결과만 들어갑니다.

```text
Xbox-2.0-0100A5B0C0DE0000.nsp
Xbox-2.0.nro
SHA256SUMS.txt
```

키 파일과 hacBrewPack 중간 폴더는 Artifact에 포함되지 않습니다.

## 오류별 확인

### No supported key secret was found

지원하는 이름의 Secret이 없습니다. `KEYS_DAT_BASE64` 또는 `KEYS_DAT`을 등록합니다.

### base64: invalid input

`KEYS_DAT_BASE64` 값이 Base64 문자열이 아니거나 일부가 빠졌습니다. PowerShell 명령으로 다시 변환해 등록합니다.

### header_key 검사 실패

등록한 값이 올바른 `keys.dat` 내용이 아니거나 필요한 항목이 없습니다. 본인 기기에서 합법적으로 관리하는 키 파일을 확인합니다.

## 보안 주의

- 키를 소스, ZIP, 커밋, Issue, Actions 로그에 직접 붙여 넣지 않습니다.
- Secret 값은 사용자 본인만 관리합니다.
- 이 워크플로는 수동 실행(`workflow_dispatch`)에서만 최종 NSP를 만듭니다.
- Artifact 보관 기간은 7일입니다.
- 현재 Title ID는 `0100A5B0C0DE0000`입니다. 같은 Title ID의 기존 설치본이 있으면 충돌하거나 교체될 수 있습니다.
