[ 프로젝트 설명 - readme.txt ]

본 프로젝트는 Netfilter와 eBPF(XDP)를 활용하여 UDP 패킷을 리디렉션하고, 각 방식의 CPU 사용률을 비교 분석한 실험 결과를 포함합니다. 디렉토리는 다음과 같은 구성으로 이루어져 있습니다.

1. report.pdf  
   └── 전체 실험 개요, 배경 이론, 구현 방법, 실험 결과 및 분석이 포함된 프로젝트 보고서입니다.

2. result/  
   └── Netfilter 및 eBPF 방식에서 측정된 CPU 사용률 데이터를 정리한 Excel 파일들이 포함되어 있습니다.  
       - netfilter_result.xlsx  
       - ebpf_reult.xlsx

3. source/  
   ├── server.py  
   ├── client.py  
   ├── netfilter/  
   │   ├── redir_module.c         : Netfilter 리디렉션용 커널 모듈 소스 코드  
   │   └── Makefile               : Netfilter 모듈 빌드를 위한 Makefile  
   └── ebpf/  
       └── redir_ebpf.c           : XDP 기반 eBPF 리디렉션 프로그램 소스 코드

4. 실행 환경  
   본 실험은 UTM 가상 머신에서 실행된 Ubuntu 리눅스 환경에서 수행되었으며, 다음과 같은 환경에서 리디렉션 실험이 진행되었습니다.

    - 운영체제: Ubuntu 22.04 LTS (ARM64, UTM VM 기반)
    - 커널 버전: 5.15.0-136-generic
    - 실험 구성:
        • 클라이언트 IP: 10.0.8.18
        • 서버(리디렉션 엔트리 포인트): 10.0.8.17
        • 리디렉션 대상 네임스페이스 서버: 10.0.1.2 (namespace: `sslab-ns`)
    - 네트워크 구성:
        • veth pair: `veth-host` ↔ `veth-ns`
        • 네임스페이스 설정 스크립트: `setup_netsh.sh` 사용
            - IP 할당: `10.0.1.1/24` (host), `10.0.1.2/24` (namespace)
            - net.ipv4.ip_forward=1 설정
            - default route 설정

※ 주의: eBPF 프로그램은 clang/llvm 및 bpftool 환경이 필요하며, Netfilter 커널 모듈은 `make` 후 `insmod` 명령어로 삽입 가능합니다.

