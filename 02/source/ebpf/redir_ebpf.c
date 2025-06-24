#include "vmlinux.h"
#include <bpf/bpf_helpers.h>

#define htons(x)    __builtin_bswap16(x)
#define htonl(x)    __builtin_bswap32(x)

#define ETH_P_IP 0x0800   // IPv4

SEC("xdp")
int xdp_udp_redirect(struct xdp_md *ctx) {
    void *data_end = (void *)(long)ctx->data_end;
    void *data = (void *)(long)ctx->data;

    struct ethhdr *eth = data;
    if ((void*)(eth + 1) > data_end) return XDP_PASS;

    if (eth->h_proto != htons(ETH_P_IP))
        return XDP_PASS;

    struct iphdr *ip = (void *)(eth + 1);
    if ((void*)(ip + 1) > data_end) return XDP_PASS;

    if (ip->protocol != IPPROTO_UDP)
        return XDP_PASS;

    struct udphdr *udp = (void *)ip + (ip->ihl * 4);
    if ((void*)(udp + 1) > data_end) return XDP_PASS;

    __u16 orig_port = udp->dest;
    __u32 orig_daddr = ip->daddr;

    // 목적 포트가 8080이 아닌 경우 무시
    if (orig_port != htons(8080))
        return XDP_PASS;

    bpf_printk("Original: dst_ip=%x, dst_port=%d", orig_daddr, __builtin_bswap16(orig_port));

    // 포트, IP 수정
    udp->dest = htons(8083);
    ip->daddr = htonl(0x0A000102);  // 10.0.1.2

    // UDP 체크섬 무효화
    udp->check = 0;

    // IP 체크섬 재계산
    ip->check = 0;
    __u32 csum = 0;
    __u16 *ip_hdr = (__u16 *)ip;
    for (int i = 0; i < sizeof(*ip) / 2; i++)
        csum += ip_hdr[i];
    while (csum >> 16)
        csum = (csum & 0xFFFF) + (csum >> 16);
    ip->check = ~csum;

    // 변경된 값 로그 출력
    bpf_printk("Modified: dst_ip=%x, dst_port=%d, ip_csum=0x%x", ip->daddr, __builtin_bswap16(udp->dest), ip->check);

    return XDP_PASS;
}

char LICENSE[] SEC("license") = "GPL";
