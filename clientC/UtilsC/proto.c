// proto.c
#include "proto.h"
#include "../clientPlayer/net.h"
#include "msg_types.h"
#include <string.h>

bool cp_read_header(int sock, CP_Header *h){
  uint8_t raw[16];
  long r = net_read_n(sock, raw, 16);
  if (r != 16) return false;
  h->version=raw[0]; h->type=raw[1];
  h->reserved=be16(&raw[2]);
  h->clientId=be32(&raw[4]);
  h->gameId=be32(&raw[8]);
  h->payloadLen=be32(&raw[12]);
  return (h->version==CP_VERSION);
}

bool cp_write_header(int sock, const CP_Header *h){
  uint8_t raw[16];
  raw[0]=h->version; raw[1]=h->type; wr_be16(&raw[2],h->reserved);
  wr_be32(&raw[4],h->clientId); wr_be32(&raw[8],h->gameId); wr_be32(&raw[12],h->payloadLen);
  return net_write_n(sock, raw, 16)==16;
}

bool cp_send_frame(int sock, uint8_t type, uint32_t clientId, uint32_t gameId,
                   const void* payload, uint32_t len){
  CP_Header h = { CP_VERSION, type, 0, clientId, gameId, len };
  if (!cp_write_header(sock,&h)) return false;
  if (!len) return true;
  return net_write_n(sock, payload, len)==(long)len;
}
