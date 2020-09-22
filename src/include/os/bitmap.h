#ifndef _BITMAP_H_
#define _BITMAP_H_

#define ENTRIES_PER_MAPWORD (sizeof(unsigned long) * 8)
#define map_size_in_bytes(_s) (_s < 8 ? 1 : (_s + 8) / 8)
#define allocated_in_map(_map, _s) (_map[(_s)/ENTRIES_PER_MAPWORD] & (1UL<<((_s)&(ENTRIES_PER_MAPWORD-1))))
#define set_map(_map, _s) _map[(_s)/ENTRIES_PER_MAPWORD] |= (1UL<<((_s)&(ENTRIES_PER_MAPWORD-1)))
#define clear_map(_map, _s) _map[(_s)/ENTRIES_PER_MAPWORD] &= ~(1UL<<((_s)&(ENTRIES_PER_MAPWORD-1)))

#endif
