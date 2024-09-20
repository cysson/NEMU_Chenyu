#include "common.h"

uint32_t dram_read(hwaddr_t, size_t);
void dram_write(hwaddr_t, size_t, uint32_t);

/* Memory accessing interfaces */



uint32_t hwaddr_read(hwaddr_t addr, size_t len) {
  int cache_L1_way_1_index = read_cache_L1(addr);
  uint32_t block_bias = addr & (CACHE_BLOCK_SIZE - 1);
  uint8_t ret[BURST_LEN << 1];
  
  if (block_bias + len > CACHE_BLOCK_SIZE) {
    int cache_L1_way_2_index = read_cache_L1(addr + CACHE_BLOCK_SIZE - block_bias);
    memcpy(ret, cache_L1[cache_L1_way_1_index].data + block_bias, CACHE_BLOCK_SIZE - block_bias);
    memcpy(ret  + CACHE_BLOCK_SIZE - block_bias, cache_L1[cache_L1_way_2_index].data, len - (CACHE_BLOCK_SIZE - block_bias));
  } else {
    memcpy(ret, cache_L1[cache_L1_way_1_index].data + block_bias, len);
  }
  
  int tmp = 0;
  uint32_t ans = unalign_rw(ret + tmp, 4) & (~0u >> ((4 - len) << 3));
  return ans;
}



void hwaddr_write(hwaddr_t addr, size_t len, uint32_t data) {
  write_cache_L1(addr, len, data);
}


uint32_t lnaddr_read(lnaddr_t addr, size_t len) {
	return hwaddr_read(addr, len);
}

void lnaddr_write(lnaddr_t addr, size_t len, uint32_t data) {
	hwaddr_write(addr, len, data);
}

uint32_t swaddr_read(swaddr_t addr, size_t len) {
#ifdef DEBUG
	assert(len == 1 || len == 2 || len == 4);
#endif
	return lnaddr_read(addr, len);
}

void swaddr_write(swaddr_t addr, size_t len, uint32_t data) {
#ifdef DEBUG
	assert(len == 1 || len == 2 || len == 4);
#endif
	lnaddr_write(addr, len, data);
}

