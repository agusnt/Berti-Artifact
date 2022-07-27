# define SIZE_1X

# if defined(SIZE_1X)
// MICRO Size
# define HISTORY_TABLE_SET            (8)
# define HISTORY_TABLE_WAY            (16)
# define TABLE_SET_MASK               (0x7)

# define BERTI_TABLE_SIZE             (16)
# define BERTI_TABLE_STRIDE_SIZE      (16)

# elif defined(SIZE_2X)
// 2xMICRO Size
# define HISTORY_TABLE_SET            (16)
# define HISTORY_TABLE_WAY            (32)
# define TABLE_SET_MASK               (0xF)

# define BERTI_TABLE_SIZE             (32)
# define BERTI_TABLE_STRIDE_SIZE      (32)

# elif defined(SIZE_4X)
// 4xMICRO Size
# define HISTORY_TABLE_SET            (32)
# define HISTORY_TABLE_WAY            (64)
# define TABLE_SET_MASK               (0x1F)

# define BERTI_TABLE_SIZE             (64)
# define BERTI_TABLE_STRIDE_SIZE      (64)

# elif defined(SIZE_050X)
// 0.5xMICRO Size
# define HISTORY_TABLE_SET            (4)
# define HISTORY_TABLE_WAY            (8)
# define TABLE_SET_MASK               (0x3)

# define BERTI_TABLE_SIZE             (8)
# define BERTI_TABLE_STRIDE_SIZE      (8)

# elif defined(SIZE_025X)
// 0.25xMICRO Size
# define HISTORY_TABLE_SET            (2)
# define HISTORY_TABLE_WAY            (4)
# define TABLE_SET_MASK               (0x1)

# define BERTI_TABLE_SIZE             (4)
# define BERTI_TABLE_STRIDE_SIZE      (4)

#endif
