#ifndef DDNET_NET_H
#define DDNET_NET_H

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#define DDNET_NET_EV_NONE 0

#define DDNET_NET_EV_CONNECT 1

#define DDNET_NET_EV_CHUNK 2

#define DDNET_NET_EV_DISCONNECT 3

#define DDNET_NET_EV_CONNLESS_CHUNK 4

typedef struct DdnetNet DdnetNet;

typedef struct DdnetNetEvent DdnetNetEvent;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

void ddnet_net_ev_new(struct DdnetNetEvent **ev);

void ddnet_net_ev_free(struct DdnetNetEvent *ev);

uint64_t ddnet_net_ev_kind(const struct DdnetNetEvent *ev);

uint64_t ddnet_net_ev_connect_peer_index(const struct DdnetNetEvent *ev);

void ddnet_net_ev_connect_addr(struct DdnetNetEvent *ev, const char **addr_ptr, size_t *addr_len);

uint64_t ddnet_net_ev_chunk_peer_index(const struct DdnetNetEvent *ev);

size_t ddnet_net_ev_chunk_len(const struct DdnetNetEvent *ev);

bool ddnet_net_ev_chunk_is_unreliable(const struct DdnetNetEvent *ev);

uint64_t ddnet_net_ev_disconnect_peer_index(const struct DdnetNetEvent *ev);

size_t ddnet_net_ev_disconnect_reason_len(const struct DdnetNetEvent *ev);

bool ddnet_net_ev_disconnect_is_remote(const struct DdnetNetEvent *ev);

size_t ddnet_net_ev_connless_chunk_len(struct DdnetNetEvent *ev);

void ddnet_net_ev_connless_chunk_addr(struct DdnetNetEvent *ev,
                                      const char **addr_ptr,
                                      size_t *addr_len);

bool ddnet_net_new(struct DdnetNet **net);

void ddnet_net_free(struct DdnetNet *net);

bool ddnet_net_set_bindaddr(struct DdnetNet *net, const char *addr, size_t addr_len);

bool ddnet_net_set_identity(struct DdnetNet *net, const uint8_t (*private_identity)[32]);

bool ddnet_net_set_accept_connections(struct DdnetNet *net, bool accept);

bool ddnet_net_open(struct DdnetNet *net);

const char *ddnet_net_error(const struct DdnetNet *net);

size_t ddnet_net_error_len(const struct DdnetNet *net);

bool ddnet_net_set_userdata(struct DdnetNet *net, uint64_t peer_index, void *userdata);

bool ddnet_net_userdata(struct DdnetNet *net, uint64_t peer_index, void **userdata);

bool ddnet_net_wait(struct DdnetNet *net);

bool ddnet_net_wait_timeout(struct DdnetNet *net, uint64_t ns);

bool ddnet_net_recv(struct DdnetNet *net,
                    uint8_t *buf,
                    size_t buf_cap,
                    struct DdnetNetEvent *event);

bool ddnet_net_send_chunk(struct DdnetNet *net,
                          uint64_t peer_index,
                          const uint8_t *chunk,
                          size_t chunk_len,
                          bool unreliable);

bool ddnet_net_flush(struct DdnetNet *net, uint64_t peer_index);

bool ddnet_net_connect(struct DdnetNet *net,
                       const char *addr,
                       size_t addr_len,
                       uint64_t *peer_index);

bool ddnet_net_close(struct DdnetNet *net,
                     uint64_t peer_index,
                     const char *reason,
                     size_t reason_len);

bool ddnet_net_send_connless_chunk(struct DdnetNet *net,
                                   const char *addr,
                                   size_t addr_len,
                                   const uint8_t *chunk,
                                   size_t chunk_len);

bool ddnet_net_num_peers_in_bucket(struct DdnetNet *net,
                                   const char *addr,
                                   size_t addr_len,
                                   uint32_t *result);

bool ddnet_net_set_logger(void (*log)(int32_t level,
                                      const char *system,
                                      size_t system_len,
                                      const char *message,
                                      size_t message_len));

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif /* DDNET_NET_H */
