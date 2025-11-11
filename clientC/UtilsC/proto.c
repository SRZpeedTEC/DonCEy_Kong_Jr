// proto.c
#include "proto.h"
#include "../clientPlayer/net.h"
#include "msg_types.h"
#include <string.h>

bool cp_read_header(int sock, CP_Header *header){
  uint8_t raw[16];
  long r = net_read_n(sock, raw, 16);
  if (r != 16) return false;
  header->version=raw[0]; header->type=raw[1];
  header->reserved=be16(&raw[2]);
  header->clientId=be32(&raw[4]);
  header->gameId=be32(&raw[8]);
  header->payloadLen=be32(&raw[12]);
  return (header->version==CP_VERSION);
}

bool cp_write_header(int sock, const CP_Header *header){
  uint8_t raw[16];
  raw[0]=header->version; raw[1]=header->type; wr_be16(&raw[2],header->reserved);
  wr_be32(&raw[4],header->clientId); wr_be32(&raw[8],header->gameId); wr_be32(&raw[12],header->payloadLen);
  return net_write_n(sock, raw, 16)==16;
}

bool cp_send_frame(int sock, uint8_t type, uint32_t clientId, uint32_t gameId, const void* payload, uint32_t len){
  CP_Header header = { CP_VERSION, type, 0, clientId, gameId, len };
  if (!cp_write_header(sock,&header)) return false;
  if (!len) return true;
  return net_write_n(sock, payload, len)==(long)len;
}
