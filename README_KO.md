# Xbox for Nintendo Switch v2.0

Green-NX 기반의 Nintendo Switch용 Xbox Cloud Gaming 및 콘솔 리모트 홈브루입니다.

## 포함 기능

- 한국어 UI
- Xbox 표시 이름과 아이콘
- `sdmc:/switch/xbox` 데이터 경로
- xCloud 720p / 1080p / 1080p HQ / 1440p / 1440p HQ 요청
- 콘솔 리모트 720p / 1080p 선택
- 스트리밍 화면 터치 시 Xbox 가이드 로고 표시
- 표시된 가이드 로고 터치 시 Guide 입력
- 마지막 터치 후 3초 뒤 자동 숨김
- 기존 Title ID `0100A5B0C0DE0000`
- 버전 `2.0`

## NRO 빌드

```sh
bash deps/build-switch.sh
docker run --rm -v "$PWD":/src -w /src devkitpro/devkita64 make
```

결과 파일은 `green-nx.nro`입니다.

## NSP 빌드

GitHub 저장소의 Actions에서 **Build installable Xbox NSP** 워크플로를 실행합니다.
완료되면 `Xbox-2.0-installable-NSP` Artifact를 받습니다.

Artifact에는 `Xbox-2.0-0100A5B0C0DE0000.nsp`, `Xbox-2.0.nro`, `SHA256SUMS.txt`가 들어갑니다.
키셋은 GitHub Actions Secret에서만 복원되고 Artifact에는 포함되지 않습니다.

## 참고

1440p와 콘솔 리모트 1080p는 서버 또는 콘솔에 해당 품질을 요청합니다. 실제 수신 해상도와 비트레이트는 연결 상태와 서버 응답에 따라 낮아질 수 있습니다.

## GitHub에서 최종 NSP 자동 생성

`.github/workflows/build-installable-nsp-inputs.yml`은 GitHub Actions Secret에 등록된 키를 사용해 최종 NSP까지 생성합니다. 자세한 내용은 `README_NSP_KO.md`를 확인하세요.
