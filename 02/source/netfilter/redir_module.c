#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>

static unsigned int prerouting_hook(void *priv, struct sk_buff *skb, const struct nf_hook_state *state);
static unsigned int postrouting_hook(void *priv, struct sk_buff *skb, const struct nf_hook_state *state);


// 원래 목적지: 10.0.8.17:8080
// 리다이렉션: 10.0.1.2:8083
#define TARGET_IP   0x0A000811  // 10.0.8.17 (hex: 0A000811)
#define REDIR_IP    0x0A000102  // 10.0.1.2  (hex: 0A000102)
#define TARGET_PORT 8080
#define REDIR_PORT  8083

static void calculate_ip_checksum(struct iphdr *iph)
{
    unsigned short *ip_header = (unsigned short *)iph;
    unsigned int checksum = 0;
    int i;

    iph->check = 0;

    for (i = 0; i < sizeof(struct iphdr) / 2; i++)
        checksum += ntohs(ip_header[i]);

    checksum = (checksum & 0xFFFF) + (checksum >> 16);
    checksum += (checksum >> 16);

    iph->check = htons(~checksum);
}


// PREROUTING 훅: 목적지 리다이렉션
static unsigned int prerouting_hook(void *priv, struct sk_buff *skb, const struct nf_hook_state *state) {
    struct iphdr *iph;
    struct udphdr *udph;
    unsigned int src_ip, dst_ip, src_port, dst_port;

    if (!skb)
    return NF_ACCEPT;

    iph = ip_hdr(skb);
    if (!iph || iph->protocol != IPPROTO_UDP)
    return NF_ACCEPT;

    udph = (struct udphdr *)((__u32 *)iph + iph->ihl);
    if (!udph)
    return NF_ACCEPT;

    src_ip = ntohl(iph->saddr);
    dst_ip = ntohl(iph->daddr);
    src_port = ntohs(udph->source);
    dst_port = ntohs(udph->dest);

    if (iph->daddr == htonl(TARGET_IP) && udph->dest == htons(TARGET_PORT)) {
        printk(KERN_INFO "[NETFILTER] FORWARD:  UDP:%pI4:%u;%pI4:%u\n", &iph->saddr, src_port, &iph->daddr, dst_port);

        iph->daddr = htonl(REDIR_IP);
        udph->dest = htons(REDIR_PORT);

        udph->check = 0;
        skb->csum = 0;
        skb->ip_summed = CHECKSUM_NONE;
        calculate_ip_checksum(iph);

        printk(KERN_INFO "[NETFILTER] MODIFIED: UDP:%pI4:%u;%pI4:%u\n", &iph->saddr, src_port, &iph->daddr, REDIR_PORT);
        printk(KERN_INFO "[NETFILTER] FORWARDING: UDP:%pI4:%u -> %pI4:%u\n", &iph->saddr, src_port, &iph->daddr, REDIR_PORT);
    }

return NF_ACCEPT;
}

// POSTROUTING 훅: 서버에서 응답 보낼 때
static unsigned int postrouting_hook(void *priv, struct sk_buff *skb, const struct nf_hook_state *state) {
    struct iphdr *iph;
    struct udphdr *udph;
    unsigned int src_ip, dst_ip, src_port, dst_port;

    if (!skb)
        return NF_ACCEPT;

    iph = ip_hdr(skb);
    if (!iph || iph->protocol != IPPROTO_UDP)
        return NF_ACCEPT;

    udph = (struct udphdr *)((__u32 *)iph + iph->ihl);
    if (!udph)
        return NF_ACCEPT;

    src_ip = ntohl(iph->saddr);
    dst_ip = ntohl(iph->daddr);
    src_port = ntohs(udph->source);
    dst_port = ntohs(udph->dest);

    // 서버에서 응답 (10.0.1.2 → 10.0.8.*)
    if (iph->saddr == htonl(REDIR_IP) && (iph->daddr & htonl(0xFFFFFF00)) == htonl(0x0A000800)) {
        printk(KERN_INFO "[NETFILTER] POSTROUTING: UDP:%pI4:%u;%pI4:%u\n", &iph->saddr, src_port, &iph->daddr, dst_port);
    }

    return NF_ACCEPT;
}

// Netfilter 등록
static struct nf_hook_ops nf_ops[] = {
    {
        .hook     = prerouting_hook,
        .hooknum  = NF_INET_PRE_ROUTING,
        .pf       = PF_INET,
        .priority = NF_IP_PRI_FIRST
    },
    {
        .hook     = postrouting_hook,
        .hooknum  = NF_INET_POST_ROUTING,
        .pf       = PF_INET,
        .priority = NF_IP_PRI_FIRST
    }
};

static int __init redir_init(void) {
    nf_register_net_hooks(&init_net, nf_ops, ARRAY_SIZE(nf_ops));
    printk(KERN_INFO "[NETFILTER] 리다이렉션 모듈 로딩됨\n");
    return 0;
}

static void __exit redir_exit(void) {
    nf_unregister_net_hooks(&init_net, nf_ops, ARRAY_SIZE(nf_ops));
    printk(KERN_INFO "[NETFILTER] 리다이렉션 모듈 언로드됨\n");
}

module_init(redir_init);
module_exit(redir_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("KYS");
MODULE_DESCRIPTION("Netfilter UDP Redirect Module");

