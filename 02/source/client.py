import socket
import os
import time
import argparse

FILE_SIZE = 1024 * 1024 * 512
CHUNK_SIZE = 1450


def send_udp_file(server_ip, server_port, num_trials):
    payload = os.urandom(FILE_SIZE)
    server_addr = (server_ip, server_port)

    for i in range(num_trials):
        print(f"[Client] Trial {i+1} 시작")

        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.bind(("0.0.0.0", 10000 + i))
        sock.settimeout(60)
        print(f"[Client] 소켓 바인딩 완료 - 포트 {10000 + i}")

        start = time.time()
        print(f"[Client] 서버로 /upload 전송")
        sock.sendto(b"/upload", server_addr)

        for j in range(0, len(payload), CHUNK_SIZE):
            if j == 0:
                print(f"[Client] 데이터 전송 시작 (총 {len(payload)} bytes)")
            sock.sendto(payload[j:j + CHUNK_SIZE], server_addr)
            # time.sleep(0.00001)

        print(f"[Client] 데이터 전송 완료, END 전송")
        sock.sendto(b"END", server_addr)

        try:
            print(f"[Client] ACK 응답 대기 중...")
            data, addr = sock.recvfrom(1024)
            src_ip, src_port = addr
            if data == b"ACK":
                end = time.time()
                duration = end - start
                print(f"[Client] Trial {i+1}: 성공 ✅ {duration*1000:.2f} ms")
            else:
                print(f"[Client] Trial {i+1}: 예상치 못한 응답 {data}")
        except socket.timeout:
            print(f"[Client] Trial {i+1}: ❌ 타임아웃")

        sock.close()
        print(f"[Client] Trial {i+1} 종료\n")
        time.sleep(0.0005)


def main():
    print("[Client] 시작됨: 서버로 연결 시도 중")
    parser = argparse.ArgumentParser()
    parser.add_argument('--host', type=str, required=True)
    parser.add_argument('--port', type=int, required=True)
    parser.add_argument('--num_trials', type=int, default=5, required=False)
    args = parser.parse_args()

    send_udp_file(
        server_ip=args.host,
        server_port=args.port,
        num_trials=args.num_trials
    )


if __name__ == "__main__":
    main()
