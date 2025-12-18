/**
 * @file espsol_ed25519.c
 * @brief Portable Ed25519 Implementation (TweetNaCl-derived)
 *
 * This is a minimal, portable Ed25519 implementation derived from TweetNaCl.
 * Used for host testing when libsodium is not available.
 *
 * On ESP32, the libsodium implementation is used instead (faster, hardware-accelerated).
 *
 * @copyright Public domain (TweetNaCl)
 */

#include "espsol_ed25519.h"
#include <string.h>
#include <stdlib.h>

/* Only compile for non-ESP32 platforms without libsodium */
#if !defined(ESP_PLATFORM) || !ESP_PLATFORM
#ifndef USE_LIBSODIUM

/* ============================================================================
 * TweetNaCl-derived Ed25519 Implementation
 * ============================================================================
 * This is a simplified version of TweetNaCl's Ed25519 implementation.
 * It includes SHA-512, field arithmetic, and curve operations.
 * ========================================================================== */

typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int64_t i64;
typedef i64 gf[16];

static const gf gf0 = {0};
static const gf gf1 = {1};
static const gf D = {
    0x78a3, 0x1359, 0x4dca, 0x75eb, 0xd8ab, 0x4141, 0x0a4d, 0x0070,
    0xe898, 0x7779, 0x4079, 0x8cc7, 0xfe73, 0x2b6f, 0x6cee, 0x5203
};
static const gf D2 = {
    0xf159, 0x26b2, 0x9b94, 0xebd6, 0xb156, 0x8283, 0x149a, 0x00e0,
    0xd130, 0xeef3, 0x80f2, 0x198e, 0xfce7, 0x56df, 0xd9dc, 0x2406
};
static const gf X = {
    0xd51a, 0x8f25, 0x2d60, 0xc956, 0xa7b2, 0x9525, 0xc760, 0x692c,
    0xdc5c, 0xfdd6, 0xe231, 0xc0a4, 0x53fe, 0xcd6e, 0x36d3, 0x2169
};
static const gf Y = {
    0x6658, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666,
    0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666
};
static const gf I = {
    0xa0b0, 0x4a0e, 0x1b27, 0xc4ee, 0xe478, 0xad2f, 0x1806, 0x2f43,
    0xd7a7, 0x3dfb, 0x0099, 0x2b4d, 0xdf0b, 0x4fc1, 0x2480, 0x2b83
};

/* ============================================================================
 * SHA-512 Implementation
 * ========================================================================== */

static const u64 K[80] = {
    0x428a2f98d728ae22ULL, 0x7137449123ef65cdULL, 0xb5c0fbcfec4d3b2fULL, 0xe9b5dba58189dbbcULL,
    0x3956c25bf348b538ULL, 0x59f111f1b605d019ULL, 0x923f82a4af194f9bULL, 0xab1c5ed5da6d8118ULL,
    0xd807aa98a3030242ULL, 0x12835b0145706fbeULL, 0x243185be4ee4b28cULL, 0x550c7dc3d5ffb4e2ULL,
    0x72be5d74f27b896fULL, 0x80deb1fe3b1696b1ULL, 0x9bdc06a725c71235ULL, 0xc19bf174cf692694ULL,
    0xe49b69c19ef14ad2ULL, 0xefbe4786384f25e3ULL, 0x0fc19dc68b8cd5b5ULL, 0x240ca1cc77ac9c65ULL,
    0x2de92c6f592b0275ULL, 0x4a7484aa6ea6e483ULL, 0x5cb0a9dcbd41fbd4ULL, 0x76f988da831153b5ULL,
    0x983e5152ee66dfabULL, 0xa831c66d2db43210ULL, 0xb00327c898fb213fULL, 0xbf597fc7beef0ee4ULL,
    0xc6e00bf33da88fc2ULL, 0xd5a79147930aa725ULL, 0x06ca6351e003826fULL, 0x142929670a0e6e70ULL,
    0x27b70a8546d22ffcULL, 0x2e1b21385c26c926ULL, 0x4d2c6dfc5ac42aedULL, 0x53380d139d95b3dfULL,
    0x650a73548baf63deULL, 0x766a0abb3c77b2a8ULL, 0x81c2c92e47edaee6ULL, 0x92722c851482353bULL,
    0xa2bfe8a14cf10364ULL, 0xa81a664bbc423001ULL, 0xc24b8b70d0f89791ULL, 0xc76c51a30654be30ULL,
    0xd192e819d6ef5218ULL, 0xd69906245565a910ULL, 0xf40e35855771202aULL, 0x106aa07032bbd1b8ULL,
    0x19a4c116b8d2d0c8ULL, 0x1e376c085141ab53ULL, 0x2748774cdf8eeb99ULL, 0x34b0bcb5e19b48a8ULL,
    0x391c0cb3c5c95a63ULL, 0x4ed8aa4ae3418acbULL, 0x5b9cca4f7763e373ULL, 0x682e6ff3d6b2b8a3ULL,
    0x748f82ee5defb2fcULL, 0x78a5636f43172f60ULL, 0x84c87814a1f0ab72ULL, 0x8cc702081a6439ecULL,
    0x90befffa23631e28ULL, 0xa4506cebde82bde9ULL, 0xbef9a3f7b2c67915ULL, 0xc67178f2e372532bULL,
    0xca273eceea26619cULL, 0xd186b8c721c0c207ULL, 0xeada7dd6cde0eb1eULL, 0xf57d4f7fee6ed178ULL,
    0x06f067aa72176fbaULL, 0x0a637dc5a2c898a6ULL, 0x113f9804bef90daeULL, 0x1b710b35131c471bULL,
    0x28db77f523047d84ULL, 0x32caab7b40c72493ULL, 0x3c9ebe0a15c9bebcULL, 0x431d67c49c100d4cULL,
    0x4cc5d4becb3e42b6ULL, 0x597f299cfc657e2aULL, 0x5fcb6fab3ad6faecULL, 0x6c44198c4a475817ULL
};

static u64 dl64(const u8 *x)
{
    u64 u = 0;
    for (int i = 0; i < 8; i++) u = (u << 8) | x[i];
    return u;
}

static void ts64(u8 *x, u64 u)
{
    for (int i = 7; i >= 0; i--) { x[i] = u & 0xff; u >>= 8; }
}

#define R(x, c) ((x) >> (c) | (x) << (64 - (c)))
#define Ch(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define Maj(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define Sigma0(x) (R(x, 28) ^ R(x, 34) ^ R(x, 39))
#define Sigma1(x) (R(x, 14) ^ R(x, 18) ^ R(x, 41))
#define sigma0(x) (R(x, 1) ^ R(x, 8) ^ ((x) >> 7))
#define sigma1(x) (R(x, 19) ^ R(x, 61) ^ ((x) >> 6))

static int sha512_block(u64 *h, const u8 *m, u64 n)
{
    u64 a, b, c, d, e, f, g, hh, t1, t2, w[80];

    while (n >= 128) {
        for (int i = 0; i < 16; i++) w[i] = dl64(m + 8 * i);
        for (int i = 16; i < 80; i++)
            w[i] = sigma1(w[i-2]) + w[i-7] + sigma0(w[i-15]) + w[i-16];

        a = h[0]; b = h[1]; c = h[2]; d = h[3];
        e = h[4]; f = h[5]; g = h[6]; hh = h[7];

        for (int i = 0; i < 80; i++) {
            t1 = hh + Sigma1(e) + Ch(e, f, g) + K[i] + w[i];
            t2 = Sigma0(a) + Maj(a, b, c);
            hh = g; g = f; f = e; e = d + t1;
            d = c; c = b; b = a; a = t1 + t2;
        }

        h[0] += a; h[1] += b; h[2] += c; h[3] += d;
        h[4] += e; h[5] += f; h[6] += g; h[7] += hh;

        m += 128;
        n -= 128;
    }
    return (int)n;
}

static void sha512(u8 *out, const u8 *m, u64 n)
{
    u64 h[8] = {
        0x6a09e667f3bcc908ULL, 0xbb67ae8584caa73bULL,
        0x3c6ef372fe94f82bULL, 0xa54ff53a5f1d36f1ULL,
        0x510e527fade682d1ULL, 0x9b05688c2b3e6c1fULL,
        0x1f83d9abfb41bd6bULL, 0x5be0cd19137e2179ULL
    };
    u8 x[256];
    u64 b = n;

    sha512_block(h, m, n);
    m += n;
    n &= 127;
    m -= n;

    memset(x, 0, 256);
    memcpy(x, m, n);
    x[n] = 128;

    n = 256 - 128 * (n < 112);
    x[n - 9] = (u8)(b >> 61);
    ts64(x + n - 8, b << 3);
    sha512_block(h, x, n);

    for (int i = 0; i < 8; i++) ts64(out + 8 * i, h[i]);
}

/* ============================================================================
 * SHA-256 Implementation (for PDA derivation)
 * ========================================================================== */

static const u32 K256[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

static u32 dl32(const u8 *x)
{
    return ((u32)x[0] << 24) | ((u32)x[1] << 16) | ((u32)x[2] << 8) | x[3];
}

static void ts32(u8 *x, u32 u)
{
    x[0] = (u8)(u >> 24);
    x[1] = (u8)(u >> 16);
    x[2] = (u8)(u >> 8);
    x[3] = (u8)u;
}

#define R32(x, c) (((x) >> (c)) | ((x) << (32 - (c))))
#define S32(x, c) ((x) >> (c))
#define Sigma0_256(x) (R32(x, 2) ^ R32(x, 13) ^ R32(x, 22))
#define Sigma1_256(x) (R32(x, 6) ^ R32(x, 11) ^ R32(x, 25))
#define sigma0_256(x) (R32(x, 7) ^ R32(x, 18) ^ S32(x, 3))
#define sigma1_256(x) (R32(x, 17) ^ R32(x, 19) ^ S32(x, 10))
#define Ch32(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define Maj32(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))

static void sha256_block(u32 *h, const u8 *m, u64 n)
{
    u32 a, b, c, d, e, f, g, hh, t1, t2, w[64];

    while (n >= 64) {
        for (int i = 0; i < 16; i++) w[i] = dl32(m + 4 * i);
        for (int i = 16; i < 64; i++)
            w[i] = sigma1_256(w[i-2]) + w[i-7] + sigma0_256(w[i-15]) + w[i-16];

        a = h[0]; b = h[1]; c = h[2]; d = h[3];
        e = h[4]; f = h[5]; g = h[6]; hh = h[7];

        for (int i = 0; i < 64; i++) {
            t1 = hh + Sigma1_256(e) + Ch32(e, f, g) + K256[i] + w[i];
            t2 = Sigma0_256(a) + Maj32(a, b, c);
            hh = g; g = f; f = e; e = d + t1;
            d = c; c = b; b = a; a = t1 + t2;
        }

        h[0] += a; h[1] += b; h[2] += c; h[3] += d;
        h[4] += e; h[5] += f; h[6] += g; h[7] += hh;

        m += 64;
        n -= 64;
    }
}

void espsol_sha256(const u8 *data, size_t len, u8 out[32])
{
    u32 h[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };
    u8 x[128];
    u64 b = len;
    u64 n = len;

    sha256_block(h, data, n);
    data += (n / 64) * 64;
    n &= 63;

    memset(x, 0, 128);
    memcpy(x, data, n);
    x[n] = 0x80;

    if (n >= 56) {
        sha256_block(h, x, 64);
        memset(x, 0, 64);
    }

    /* Length in bits at the end */
    x[63] = (u8)(b << 3);
    x[62] = (u8)(b >> 5);
    x[61] = (u8)(b >> 13);
    x[60] = (u8)(b >> 21);
    x[59] = (u8)(b >> 29);
    x[58] = (u8)(b >> 37);
    x[57] = (u8)(b >> 45);
    x[56] = (u8)(b >> 53);

    sha256_block(h, x, 64);

    for (int i = 0; i < 8; i++) ts32(out + 4 * i, h[i]);
}

/* ============================================================================
 * Field Arithmetic
 * ========================================================================== */

static void set25519(gf r, const gf a)
{
    for (int i = 0; i < 16; i++) r[i] = a[i];
}

static void car25519(gf o)
{
    for (int i = 0; i < 16; i++) {
        o[i] += (1LL << 16);
        i64 c = o[i] >> 16;
        o[(i + 1) * (i < 15)] += c - 1 + 37 * (c - 1) * (i == 15);
        o[i] -= c << 16;
    }
}

static void sel25519(gf p, gf q, int b)
{
    i64 c = ~(b - 1);
    for (int i = 0; i < 16; i++) {
        i64 t = c & (p[i] ^ q[i]);
        p[i] ^= t;
        q[i] ^= t;
    }
}

static void pack25519(u8 *o, const gf n)
{
    gf m, t;
    set25519(t, n);
    car25519(t);
    car25519(t);
    car25519(t);

    for (int j = 0; j < 2; j++) {
        m[0] = t[0] - 0xffed;
        for (int i = 1; i < 15; i++) {
            m[i] = t[i] - 0xffff - ((m[i-1] >> 16) & 1);
            m[i-1] &= 0xffff;
        }
        m[15] = t[15] - 0x7fff - ((m[14] >> 16) & 1);
        int b = (m[15] >> 16) & 1;
        m[14] &= 0xffff;
        sel25519(t, m, 1 - b);
    }

    for (int i = 0; i < 16; i++) {
        o[2*i] = (u8)t[i];
        o[2*i + 1] = (u8)(t[i] >> 8);
    }
}

static int neq25519(const gf a, const gf b)
{
    u8 c[32], d[32];
    pack25519(c, a);
    pack25519(d, b);
    return memcmp(c, d, 32);
}

static u8 par25519(const gf a)
{
    u8 d[32];
    pack25519(d, a);
    return d[0] & 1;
}

static void unpack25519(gf o, const u8 *n)
{
    for (int i = 0; i < 16; i++) o[i] = n[2*i] + ((i64)n[2*i + 1] << 8);
    o[15] &= 0x7fff;
}

static void A(gf o, const gf a, const gf b)
{
    for (int i = 0; i < 16; i++) o[i] = a[i] + b[i];
}

static void Z(gf o, const gf a, const gf b)
{
    for (int i = 0; i < 16; i++) o[i] = a[i] - b[i];
}

static void M(gf o, const gf a, const gf b)
{
    i64 t[31] = {0};
    for (int i = 0; i < 16; i++)
        for (int j = 0; j < 16; j++)
            t[i+j] += a[i] * b[j];
    for (int i = 0; i < 15; i++) t[i] += 38 * t[i + 16];
    for (int i = 0; i < 16; i++) o[i] = t[i];
    car25519(o);
    car25519(o);
}

static void S(gf o, const gf a)
{
    M(o, a, a);
}

static void inv25519(gf o, const gf i)
{
    gf c;
    set25519(c, i);
    for (int a = 253; a >= 0; a--) {
        S(c, c);
        if (a != 2 && a != 4) M(c, c, i);
    }
    set25519(o, c);
}

static void pow2523(gf o, const gf i)
{
    gf c;
    set25519(c, i);
    for (int a = 250; a >= 0; a--) {
        S(c, c);
        if (a != 1) M(c, c, i);
    }
    set25519(o, c);
}

/* ============================================================================
 * Curve Operations
 * ========================================================================== */

static void add(gf p[4], gf q[4])
{
    gf a, b, c, d, t, e, f, g, h;

    Z(a, p[1], p[0]);
    Z(t, q[1], q[0]);
    M(a, a, t);
    A(b, p[0], p[1]);
    A(t, q[0], q[1]);
    M(b, b, t);
    M(c, p[3], q[3]);
    M(c, c, D2);
    M(d, p[2], q[2]);
    A(d, d, d);
    Z(e, b, a);
    Z(f, d, c);
    A(g, d, c);
    A(h, b, a);

    M(p[0], e, f);
    M(p[1], h, g);
    M(p[2], g, f);
    M(p[3], e, h);
}

static void cswap(gf p[4], gf q[4], u8 b)
{
    for (int i = 0; i < 4; i++)
        sel25519(p[i], q[i], b);
}

static void pack(u8 *r, gf p[4])
{
    gf tx, ty, zi;
    inv25519(zi, p[2]);
    M(tx, p[0], zi);
    M(ty, p[1], zi);
    pack25519(r, ty);
    r[31] ^= par25519(tx) << 7;
}

static void scalarmult(gf p[4], gf q[4], const u8 *s)
{
    gf Q[4];
    set25519(p[0], gf0);
    set25519(p[1], gf1);
    set25519(p[2], gf1);
    set25519(p[3], gf0);

    for (int i = 0; i < 4; i++) set25519(Q[i], q[i]);

    for (int i = 255; i >= 0; --i) {
        u8 b = (s[i / 8] >> (i & 7)) & 1;
        cswap(p, Q, b);
        add(Q, p);
        add(p, p);
        cswap(p, Q, b);
    }
}

static void scalarbase(gf p[4], const u8 *s)
{
    gf q[4];
    set25519(q[0], X);
    set25519(q[1], Y);
    set25519(q[2], gf1);
    M(q[3], X, Y);
    scalarmult(p, q, s);
}

static int unpackneg(gf r[4], const u8 p[32])
{
    gf t, chk, num, den, den2, den4, den6;
    set25519(r[2], gf1);
    unpack25519(r[1], p);
    S(num, r[1]);
    M(den, num, D);
    Z(num, num, r[2]);
    A(den, r[2], den);

    S(den2, den);
    S(den4, den2);
    M(den6, den4, den2);
    M(t, den6, num);
    M(t, t, den);

    pow2523(t, t);
    M(t, t, num);
    M(t, t, den);
    M(t, t, den);
    M(r[0], t, den);

    S(chk, r[0]);
    M(chk, chk, den);
    if (neq25519(chk, num)) M(r[0], r[0], I);

    S(chk, r[0]);
    M(chk, chk, den);
    if (neq25519(chk, num)) return -1;

    if (par25519(r[0]) == (p[31] >> 7)) Z(r[0], gf0, r[0]);

    M(r[3], r[0], r[1]);
    return 0;
}

/* ============================================================================
 * Modular Reduction (L = 2^252 + 27742317777372353535851937790883648493)
 * ========================================================================== */

static const u64 L[32] = {
    0xed, 0xd3, 0xf5, 0x5c, 0x1a, 0x63, 0x12, 0x58,
    0xd6, 0x9c, 0xf7, 0xa2, 0xde, 0xf9, 0xde, 0x14,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0x10
};

static void modL(u8 *r, i64 x[64])
{
    i64 carry;
    for (int i = 63; i >= 32; --i) {
        carry = 0;
        for (int j = i - 32; j < i - 12; ++j) {
            x[j] += carry - 16 * x[i] * (i64)L[j - (i - 32)];
            carry = (x[j] + 128) >> 8;
            x[j] -= carry << 8;
        }
        x[i - 12] += carry;
        x[i] = 0;
    }

    carry = 0;
    for (int j = 0; j < 32; ++j) {
        x[j] += carry - (x[31] >> 4) * (i64)L[j];
        carry = x[j] >> 8;
        x[j] &= 255;
    }

    for (int j = 0; j < 32; ++j)
        x[j] -= carry * (i64)L[j];

    for (int i = 0; i < 32; ++i) {
        x[i + 1] += x[i] >> 8;
        r[i] = (u8)(x[i] & 255);
    }
}

static void reduce(u8 *r)
{
    i64 x[64];
    for (int i = 0; i < 64; ++i) x[i] = (u64)r[i];
    for (int i = 0; i < 64; ++i) r[i] = 0;
    modL(r, x);
}

/* ============================================================================
 * Public API
 * ========================================================================== */

esp_err_t espsol_ed25519_keypair_from_seed(const uint8_t seed[32],
                                            uint8_t public_key[32],
                                            uint8_t private_key[64])
{
    if (seed == NULL || public_key == NULL || private_key == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    u8 d[64];
    gf p[4];

    /* Hash the seed to get the secret scalar */
    sha512(d, seed, 32);
    d[0] &= 248;
    d[31] &= 127;
    d[31] |= 64;

    /* Compute public key as d * B */
    scalarbase(p, d);
    pack(public_key, p);

    /* Private key = seed || public_key */
    memcpy(private_key, seed, 32);
    memcpy(private_key + 32, public_key, 32);

    return ESP_OK;
}

esp_err_t espsol_ed25519_sign(const uint8_t *message, size_t message_len,
                               const uint8_t private_key[64],
                               uint8_t signature[64])
{
    if (private_key == NULL || signature == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (message == NULL && message_len > 0) {
        return ESP_ERR_INVALID_ARG;
    }

    u8 d[64], h[64], r[64];
    i64 x[64];
    gf p[4];

    /* Hash private key */
    sha512(d, private_key, 32);
    d[0] &= 248;
    d[31] &= 127;
    d[31] |= 64;

    /* Compute r = H(prefix || message) */
    /* Create hash input: d[32..63] || message */
    size_t hlen = 32 + message_len;
    u8 *hm = (u8 *)malloc(hlen > 0 ? hlen : 1);
    if (!hm) return ESP_ERR_ESPSOL_CRYPTO_ERROR;

    memcpy(hm, d + 32, 32);
    if (message_len > 0 && message != NULL) {
        memcpy(hm + 32, message, message_len);
    }
    sha512(r, hm, hlen);
    free(hm);

    reduce(r);
    scalarbase(p, r);
    pack(signature, p);

    /* Compute S = (r + H(R || A || message) * s) mod L */
    hlen = 64 + message_len;
    hm = (u8 *)malloc(hlen > 0 ? hlen : 1);
    if (!hm) return ESP_ERR_ESPSOL_CRYPTO_ERROR;

    memcpy(hm, signature, 32);
    memcpy(hm + 32, private_key + 32, 32);
    if (message_len > 0 && message != NULL) {
        memcpy(hm + 64, message, message_len);
    }
    sha512(h, hm, hlen);
    free(hm);

    reduce(h);

    for (int i = 0; i < 64; ++i) x[i] = 0;
    for (int i = 0; i < 32; ++i) x[i] = (u64)r[i];
    for (int i = 0; i < 32; ++i)
        for (int j = 0; j < 32; ++j)
            x[i + j] += (u64)h[i] * (u64)d[j];

    modL(signature + 32, x);

    return ESP_OK;
}

esp_err_t espsol_ed25519_verify(const uint8_t *message, size_t message_len,
                                 const uint8_t signature[64],
                                 const uint8_t public_key[32])
{
    if (signature == NULL || public_key == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (message == NULL && message_len > 0) {
        return ESP_ERR_INVALID_ARG;
    }

    u8 t[32], h[64];
    gf p[4], q[4];

    if (unpackneg(q, public_key)) {
        return ESP_ERR_ESPSOL_SIGNATURE_INVALID;
    }

    /* Compute h = H(R || A || message) */
    size_t hlen = 64 + message_len;
    u8 *hm = (u8 *)malloc(hlen > 0 ? hlen : 1);
    if (!hm) return ESP_ERR_ESPSOL_CRYPTO_ERROR;

    memcpy(hm, signature, 32);
    memcpy(hm + 32, public_key, 32);
    if (message_len > 0 && message != NULL) {
        memcpy(hm + 64, message, message_len);
    }
    sha512(h, hm, hlen);
    free(hm);

    reduce(h);

    /* Compute P = S*B - h*A */
    scalarmult(p, q, h);
    scalarbase(q, signature + 32);
    add(p, q);
    pack(t, p);

    /* Verify R == P */
    if (memcmp(signature, t, 32) != 0) {
        return ESP_ERR_ESPSOL_SIGNATURE_INVALID;
    }

    return ESP_OK;
}

#endif /* !USE_LIBSODIUM */
#endif /* !ESP_PLATFORM */
