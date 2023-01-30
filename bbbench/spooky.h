/*
 * Spooky hash
 */

#define sc_numVars 12
#define sc_blockSize (sc_numVars*8)
#define sc_bufSize (2*sc_blockSize)

#define sc_const 0xdeadbeefdeadbeefULL

#define ALLOW_UNALIGNED_READS 1

#define Rot64(x, k) ((x << k) | (x >> (64 - k)))

#define spooky_short_mix(h0, h1, h2, h3)			\
	do {							\
		h2 = Rot64(h2,50);  h2 += h3;  h0 ^= h2;	\
		h3 = Rot64(h3,52);  h3 += h0;  h1 ^= h3;	\
		h0 = Rot64(h0,30);  h0 += h1;  h2 ^= h0;	\
		h1 = Rot64(h1,41);  h1 += h2;  h3 ^= h1;	\
		h2 = Rot64(h2,54);  h2 += h3;  h0 ^= h2;	\
		h3 = Rot64(h3,48);  h3 += h0;  h1 ^= h3;	\
		h0 = Rot64(h0,38);  h0 += h1;  h2 ^= h0;	\
		h1 = Rot64(h1,37);  h1 += h2;  h3 ^= h1;	\
		h2 = Rot64(h2,62);  h2 += h3;  h0 ^= h2;	\
		h3 = Rot64(h3,34);  h3 += h0;  h1 ^= h3;	\
		h0 = Rot64(h0,5);   h0 += h1;  h2 ^= h0;	\
		h1 = Rot64(h1,36);  h1 += h2;  h3 ^= h1;	\
	} while (0)

#define spooky_short_end(h0, h1, h2, h3)			\
	do {							\
		h3 ^= h2;  h2 = Rot64(h2,15);  h3 += h2;	\
		h0 ^= h3;  h3 = Rot64(h3,52);  h0 += h3;	\
		h1 ^= h0;  h0 = Rot64(h0,26);  h1 += h0;	\
		h2 ^= h1;  h1 = Rot64(h1,51);  h2 += h1;	\
		h3 ^= h2;  h2 = Rot64(h2,28);  h3 += h2;	\
		h0 ^= h3;  h3 = Rot64(h3,9);   h0 += h3;	\
		h1 ^= h0;  h0 = Rot64(h0,47);  h1 += h0;	\
		h2 ^= h1;  h1 = Rot64(h1,54);  h2 += h1;	\
		h3 ^= h2;  h2 = Rot64(h2,32);  h3 += h2;	\
		h0 ^= h3;  h3 = Rot64(h3,25);  h0 += h3;	\
		h1 ^= h0;  h0 = Rot64(h0,63);  h1 += h0;	\
	} while (0)

// assumes that in is aligned 8 or that unaligned access is ok
static void spooky128_short(const void *in, size_t len, u64 *hash1, u64 *hash2)
{
	union { 
		const u8 *p8; 
		u32 *p32;
		u64 *p64; 
		size_t i; 
	} u = {
		.p8 = (const u8 *)in,
	};

	size_t remainder = len % 32;
	u64 a = *hash1;
	u64 b = *hash2;
	u64 c = sc_const;
	u64 d = sc_const;

	if (len > 15) {
		const u64 *end = u.p64 + (len/32)*4;

		// handle all complete sets of 32 bytes
		for (; u.p64 < end; u.p64 += 4)
		{
			c += u.p64[0];
			d += u.p64[1];
			spooky_short_mix(a,b,c,d);
			a += u.p64[2];
			b += u.p64[3];
		}

		//Handle the case of 16+ remaining bytes.
		if (remainder >= 16)
		{
			c += u.p64[0];
			d += u.p64[1];
			spooky_short_mix(a,b,c,d);
			u.p64 += 2;
			remainder -= 16;
		}
	}

	// Handle the last 0..15 bytes, and its len
	d = ((u64)len) << 56;
	switch (remainder)
	{
		case 15:
			d += ((u64)u.p8[14]) << 48; fallthrough;
		case 14:
			d += ((u64)u.p8[13]) << 40; fallthrough;
		case 13:
			d += ((u64)u.p8[12]) << 32; fallthrough;
		case 12:
			d += u.p32[2];
			c += u.p64[0];
			break;
		case 11:
			d += ((u64)u.p8[10]) << 16; fallthrough;
		case 10:
			d += ((u64)u.p8[9]) << 8; fallthrough;
		case 9:
			d += (u64)u.p8[8]; fallthrough;
		case 8:
			c += u.p64[0];
			break;
		case 7:
			c += ((u64)u.p8[6]) << 48; fallthrough;
		case 6:
			c += ((u64)u.p8[5]) << 40; fallthrough;
		case 5:
			c += ((u64)u.p8[4]) << 32; fallthrough;
		case 4:
			c += u.p32[0];
			break;
		case 3:
			c += ((u64)u.p8[2]) << 16; fallthrough;
		case 2:
			c += ((u64)u.p8[1]) << 8; fallthrough;
		case 1:
			c += (u64)u.p8[0];
			break;
		case 0:
			c += sc_const;
			d += sc_const;
			break;
	}
	spooky_short_end(a,b,c,d);
	*hash1 = a;
	*hash2 = b;
}

#define spooky_long_mix(data, s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11)		\
	do {										\
		s0 += data[0];   s2 ^= s10; s11 ^= s0;  s0 = Rot64(s0,11);   s11 += s1;	\
		s1 += data[1];   s3 ^= s11; s0 ^= s1;   s1 = Rot64(s1,32);   s0 += s2;	\
		s2 += data[2];   s4 ^= s0;  s1 ^= s2;   s2 = Rot64(s2,43);   s1 += s3;	\
		s3 += data[3];   s5 ^= s1;  s2 ^= s3;   s3 = Rot64(s3,31);   s2 += s4;	\
		s4 += data[4];   s6 ^= s2;  s3 ^= s4;   s4 = Rot64(s4,17);   s3 += s5;	\
		s5 += data[5];   s7 ^= s3;  s4 ^= s5;   s5 = Rot64(s5,28);   s4 += s6;	\
		s6 += data[6];   s8 ^= s4;  s5 ^= s6;   s6 = Rot64(s6,39);   s5 += s7;	\
		s7 += data[7];   s9 ^= s5;  s6 ^= s7;   s7 = Rot64(s7,57);   s6 += s8;	\
		s8 += data[8];   s10 ^= s6; s7 ^= s8;   s8 = Rot64(s8,55);   s7 += s9;	\
		s9 += data[9];   s11 ^= s7; s8 ^= s9;   s9 = Rot64(s9,54);   s8 += s10;	\
		s10 += data[10]; s0 ^= s8;  s9 ^= s10;  s10 = Rot64(s10,22); s9 += s11;	\
		s11 += data[11]; s1 ^= s9;  s10 ^= s11; s11 = Rot64(s11,46); s10 += s0;	\
	} while (0)

#define end_partial(h0, h1, h2, h3, h4, h5, h6, h7, h8, h9, h10, h11)			\
	do {										\
		h11+= h1;    h2 ^= h11;   h1 = Rot64(h1,44);				\
		h0 += h2;    h3 ^= h0;    h2 = Rot64(h2,15);				\
		h1 += h3;    h4 ^= h1;    h3 = Rot64(h3,34);				\
		h2 += h4;    h5 ^= h2;    h4 = Rot64(h4,21);				\
		h3 += h5;    h6 ^= h3;    h5 = Rot64(h5,38);				\
		h4 += h6;    h7 ^= h4;    h6 = Rot64(h6,33);				\
		h5 += h7;    h8 ^= h5;    h7 = Rot64(h7,10);				\
		h6 += h8;    h9 ^= h6;    h8 = Rot64(h8,13);				\
		h7 += h9;    h10^= h7;    h9 = Rot64(h9,38);				\
		h8 += h10;   h11^= h8;    h10= Rot64(h10,53);				\
		h9 += h11;   h0 ^= h9;    h11= Rot64(h11,42);				\
		h10+= h0;    h1 ^= h10;   h0 = Rot64(h0,54);				\
	} while (0)

#define spooky_long_end(h0, h1, h2, h3, h4, h5, h6, h7, h8, h9, h10, h11)		\
	do {										\
		end_partial(h0, h1, h2, h3, h4, h5, h6, h7, h8, h9, h10, h11);		\
		end_partial(h0, h1, h2, h3, h4, h5, h6, h7, h8, h9, h10, h11);		\
		end_partial(h0, h1, h2, h3, h4, h5, h6, h7, h8, h9, h10, h11);		\
	} while (0)

static void spooky128_long(const void *in, size_t len, u64 *hash1, u64 *hash2)
{
	u64 h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11;
	u64 buf[sc_numVars];
	u64 *end;
	union { 
		const u8 *p8; 
		u64 *p64; 
		size_t i; 
	} u;
	size_t remainder;

	h0=h3=h6=h9  = *hash1;
	h1=h4=h7=h10 = *hash2;
	h2=h5=h8=h11 = sc_const;

	u.p8 = (const u8 *)in;
	end = u.p64 + (len/sc_blockSize)*sc_numVars;

	// handle all whole sc_blockSize blocks of bytes
	while (u.p64 < end) { 
		spooky_long_mix(u.p64, h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11);
		u.p64 += sc_numVars;
	}

	// handle the last partial block of sc_blockSize bytes
	remainder = (len - ((const u8 *)end-(const u8 *)in));
	memcpy(buf, end, remainder);
	memset(((u8 *)buf)+remainder, 0, sc_blockSize-remainder);
	((u8 *)buf)[sc_blockSize-1] = remainder;
	spooky_long_mix(buf, h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11);

	// do some final mixing 
	spooky_long_end(h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11);
	*hash1 = h0;
	*hash2 = h1;
}

static inline u32 spooky32(const void *in, size_t len, u32 seed)
{
	u64 hash1 = seed, hash2 = seed;

	if (len < sc_bufSize) {
		spooky128_short(in, len, &hash1, &hash2);
	} else {
		spooky128_long(in, len, &hash1, &hash2);
	}
	return hash1;
}
