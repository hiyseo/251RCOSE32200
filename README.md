# 🧪 01 - EXT4 vs Btrfs: Linux 파일 시스템 Write 실험

본 프로젝트는 Linux 커널 소스에 직접 로그를 삽입하여, EXT4와 Btrfs 파일 시스템의 **쓰기(write)** 동작 차이를 분석한 실험입니다. 특히 Btrfs의 **Copy-on-Write (CoW)** 특성과 EXT4의 전통적인 데이터 복사 방식의 차이를 실질적인 **디스크 I/O 및 inode 동작 관찰**을 통해 비교합니다.

---

## 📌 실험 개요

- **커널 버전**: Linux 5.15
- **분석 대상**: EXT4 vs Btrfs
- **실험 방식**:
  - 파일 생성 및 복사 시점에 커널 로그 삽입
  - 100개의 파일을 반복 복사하여 쓰기 패턴 관찰
  - 512KB 간격으로 로그 출력 필터링
- **목표**: 각 파일 시스템의 쓰기 동작 차이, I/O 횟수, inode 갱신 시점 파악

---

## 📂 디렉토리 구성

```
.
├── ext4_log_patch.c           # EXT4 로그 삽입 코드
├── btrfs_log_patch.c          # Btrfs 로그 삽입 코드
├── testscript_ext4.sh         # EXT4 파일 복사 테스트 스크립트
├── testscript_btrfs.sh        # Btrfs 파일 복사 테스트 스크립트
└── README.md                  # 본 문서
```

---

## 🔧 로그 코드 삽입 예시

### EXT4 (`fs/ext4/page-io.c`)

```c
if ((strstr(path, "test") != NULL) && (offset % (1024 * 512) == 0)) {
    printk(KERN_INFO "[EXT4][BIO_WRITE] file=%s inode=%lu offset=%llu\n",
        path, inode->i_ino, (unsigned long long)offset);
}
```

### Btrfs (`fs/btrfs/extent_io.c`)

```c
if ((strstr(path, "test") != NULL) && (offset % (1024 * 512) == 0)) {
    printk(KERN_INFO "[BTRFS][WRITEPAGE] file=%s inode=%lu offset=%llu\n",
        path, inode->i_ino, (unsigned long long)offset);
}
```

---

## 🧪 테스트 스크립트

### `testscript_ext4.sh`

```bash
dd if=/dev/zero of=test bs=1024 count=2000
for i in $(seq 1 100); do
    cp test test_$i
done
```

### `testscript_btrfs.sh`

```bash
dd if=/dev/zero of=test bs=1024 count=2000
for i in $(seq 1 100); do
    cp --reflink=auto test test_$i
done
```

---

## 📊 실험 결과 요약

| 항목               | EXT4                                | Btrfs (CoW)                        |
|--------------------|-------------------------------------|------------------------------------|
| 데이터 복사 방식   | 디스크에 **실제 블록 복사** 발생       | **데이터 블록 공유**, 메타데이터만 생성 |
| 디스크 쓰기 횟수  | 약 **400회 이상**                    | **약 4회** (초기 생성만 발생)        |
| inode 동작         | 각 파일 복사마다 **inode 생성 및 flush** | **최초 1회 inode 기록만** 발생       |
| offset 로그 패턴   | 주기적, 반복적                       | **복사 과정 중 없음**                 |

---

## 🔍 결론

- **EXT4**는 전통적인 쓰기 방식으로 인해 매 파일 복사마다 디스크 I/O 발생 → I/O 부담 ↑
- **Btrfs**는 CoW 방식으로 데이터 공유 → 디스크 쓰기 최소화, I/O 효율 ↑
- 실험을 통해 파일 시스템 구조의 근본적인 차이가 디스크 동작에 큰 영향을 준다는 사실 확인

---

## 🙋‍♀️ 실험 과정에서 어려웠던 점

- UTM 기반 가상 환경 설정, 커널 빌드 및 디버깅 환경 구축에 시간 소요
- 커널 내부 함수 흐름 파악: `ext4_file_write_iter()`, `btrfs_file_write_iter()` 등 흐름 추적
- VSCode로 커널 구조 탐색, `printk()` 삽입 및 로그 필터링 조건 설정 등 직접 실험 반복

---

## 📎 참고

- 커널 로그 확인: `dmesg | grep [EXT4]` 또는 `[BTRFS]`
- reflink 사용 여부: `cp --reflink=auto` (Btrfs만 지원)
- 커널 소스 분석: `/usr/src/linux-5.15/fs/ext4/`, `fs/btrfs/`<br><br><br><br>

# 🛰️ 02 - UDP 리디렉션 성능 비교: Netfilter vs eBPF (XDP)

본 프로젝트는 **Netfilter 기반 커널 모듈**과 **eBPF(XDP) 기반 리디렉션 프로그램**을 구현하여, 동일한 UDP 트래픽에 대해 **CPU 사용률을 비교 분석**하는 실험을 수행한 결과를 정리한 것입니다.

---

## 📁 디렉토리 구성

```
.
├── report.pdf                # 전체 실험 이론, 구현, 분석이 포함된 보고서
├── result/
│   ├── netfilter_result.xlsx # Netfilter 측정 결과
│   └── ebpf_result.xlsx      # eBPF 측정 결과
└── source/
    ├── client.py             # UDP 클라이언트 (512MB 전송)
    ├── server.py             # UDP 서버
    ├── netfilter/
    │   ├── redir_module.c    # Netfilter 기반 리디렉션 커널 모듈
    │   └── Makefile
    └── ebpf/
        └── redir_ebpf.c      # XDP 기반 eBPF 리디렉션 코드
```

---

## ⚙️ 실험 환경

- **OS**: Ubuntu 22.04 (ARM64, UTM VM 기반)
- **커널 버전**: 5.15.0-136-generic
- **네트워크 구성**:
  - 클라이언트: `10.0.8.18`
  - 리디렉션 서버 (호스트): `10.0.8.17`
  - 네임스페이스 서버: `10.0.1.2` (namespace: `sslab-ns`)
  - 인터페이스: `veth-host ↔ veth-ns`
- **설정 스크립트**: `setup_netsh.sh`
  - IP 할당: `10.0.1.1/24` (host), `10.0.1.2/24` (namespace)
  - ip_forward 설정 및 default route 설정 포함

---

## 🧪 실험 방법

- 클라이언트에서 512MB UDP 파일을 전송
- 두 방식(Netfilter, XDP) 각각에 대해 리디렉션 수행
- `mpstat`를 통해 CPU 사용률 모니터링

### 측정 항목
- 전체 CPU 사용률
- %usr (사용자 영역)
- %sys (커널 영역)
- %softirq (소프트웨어 인터럽트)
- %idle (유휴 시간)

---

## 🔧 리디렉션 방식 설명

### 1. Netfilter 기반
- 커널 모듈 `redir_module.c` 작성
- PREROUTING 훅에서 목적지 IP/포트 수정
- POSTROUTING 훅에서 응답 로그 출력
- IP 체크섬 및 UDP 체크섬 수동 계산 및 설정

### 2. eBPF (XDP) 기반
- C로 작성된 eBPF 프로그램을 XDP Hook에 attach
- 패킷 수신 즉시 IP/포트 수정 및 리디렉션
- IP 체크섬 수동 계산, UDP 체크섬은 0으로 무효화
- 커널 개입 최소화 → 고성능 처리 가능

---

## 📊 실험 결과 요약

| 항목             | Netfilter                    | eBPF (XDP)                 |
|------------------|------------------------------|----------------------------|
| 리디렉션 위치     | PREROUTING (IP 계층 이후)     | NIC 수신 직후 (드라이버 계층) |
| CPU %sys         | 높음                         | 낮음                       |
| CPU %softirq     | 높음                         | 매우 낮음                  |
| 사용자 영역 %usr | 유사함                       | 유사함                     |
| 처리 계층 깊이    | 커널 네트워크 스택 전체 통과 | 드라이버에서 즉시 처리     |

- **Netfilter**: 소켓 버퍼(skb) 생성 및 커널 경로 통과로 인한 부하 발생
- **XDP (eBPF)**: 커널 경로 최소화, context switch 제거 → CPU 사용률 낮음

---

## 🚧 어려웠던 점과 해결

- **패킷 수신 수 차이**: 동일 조건에서도 Netfilter 수신 패킷 수가 낮게 측정
- **원인**: skb 처리 및 커널 구조상 오버헤드
- **대응**: 체크섬 처리 방식 및 커널 파라미터 조정, tcpdump로 드롭 여부 확인

---

## 📌 결론

- 동일한 리디렉션 동작도 **처리 계층의 위치에 따라 CPU 효율성이 극명하게 달라짐**
- XDP는 실시간성 및 대용량 트래픽 처리에 있어 Netfilter보다 **명확한 성능 이점**을 가짐
- 커널 네트워크 처리의 병목 구조 이해 및 최적화 기법 학습에 유의미한 실험

---

## 📎 참고사항

- XDP 프로그램 컴파일: `clang`, `llvm`, `bpftool` 필요
- Netfilter 모듈: `make` 후 `sudo insmod redir_module.ko`
- 커널 로그 확인:
  ```bash
  dmesg | grep XDP
  dmesg | grep NF
  ```
