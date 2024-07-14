#ifndef LYNX_NET_SOCKETS_OPS_H
#define LYNX_NET_SOCKETS_OPS_H

#include <arpa/inet.h>

namespace lynx::sockets {

int createNonblockingOrDie(sa_family_t family);

int connect(int sockfd, const struct sockaddr *addr);
void bindOrDie(int sockfd, const struct sockaddr *addr);
void listenOrDie(int sockfd);
int accept(int sockfd, struct sockaddr_in6 *addr);
void close(int sockfd);
void shutdownWrite(int sockfd);

void toIpPort(char *buf, size_t size, const struct sockaddr *addr);
void toIp(char *buf, size_t size, const struct sockaddr *addr);

void fromIpPort(const char *ip, uint16_t port, struct sockaddr_in *addr);
void fromIpPort(const char *ip, uint16_t port, struct sockaddr_in6 *addr);

int getSocketError(int sockfd);

const struct sockaddr *sockaddrCast(const struct sockaddr_in *addr);
const struct sockaddr *sockaddrCast(const struct sockaddr_in6 *addr);
struct sockaddr *sockaddrCast(struct sockaddr_in6 *addr);
const struct sockaddr_in *sockaddrInCast(const struct sockaddr *addr);
const struct sockaddr_in6 *sockaddrIn6Cast(const struct sockaddr *addr);

struct sockaddr_in6 getLocalAddr(int sockfd);
struct sockaddr_in6 getPeerAddr(int sockfd);
bool isSelfConnect(int sockfd);

} // namespace lynx::sockets

#endif
