#include "lynx/logger/log_stream.h"

namespace lynx::detail {

#define npowers 87
#define steppowers 8
#define firstpower (-348) /* 10 ^ -348 */

#define expmax (-32)
#define expmin (-60)

struct Fp {
  uint64_t frac_;
  int exp_;
};

static Fp powers_ten[] = {
    {18054884314459144840U, -1220}, {13451937075301367670U, -1193},
    {10022474136428063862U, -1166}, {14934650266808366570U, -1140},
    {11127181549972568877U, -1113}, {16580792590934885855U, -1087},
    {12353653155963782858U, -1060}, {18408377700990114895U, -1034},
    {13715310171984221708U, -1007}, {10218702384817765436U, -980},
    {15227053142812498563U, -954},  {11345038669416679861U, -927},
    {16905424996341287883U, -901},  {12595523146049147757U, -874},
    {9384396036005875287U, -847},   {13983839803942852151U, -821},
    {10418772551374772303U, -794},  {15525180923007089351U, -768},
    {11567161174868858868U, -741},  {17236413322193710309U, -715},
    {12842128665889583758U, -688},  {9568131466127621947U, -661},
    {14257626930069360058U, -635},  {10622759856335341974U, -608},
    {15829145694278690180U, -582},  {11793632577567316726U, -555},
    {17573882009934360870U, -529},  {13093562431584567480U, -502},
    {9755464219737475723U, -475},   {14536774485912137811U, -449},
    {10830740992659433045U, -422},  {16139061738043178685U, -396},
    {12024538023802026127U, -369},  {17917957937422433684U, -343},
    {13349918974505688015U, -316},  {9946464728195732843U, -289},
    {14821387422376473014U, -263},  {11042794154864902060U, -236},
    {16455045573212060422U, -210},  {12259964326927110867U, -183},
    {18268770466636286478U, -157},  {13611294676837538539U, -130},
    {10141204801825835212U, -103},  {15111572745182864684U, -77},
    {11258999068426240000U, -50},   {16777216000000000000U, -24},
    {12500000000000000000U, 3},     {9313225746154785156U, 30},
    {13877787807814456755U, 56},    {10339757656912845936U, 83},
    {15407439555097886824U, 109},   {11479437019748901445U, 136},
    {17105694144590052135U, 162},   {12744735289059618216U, 189},
    {9495567745759798747U, 216},    {14149498560666738074U, 242},
    {10542197943230523224U, 269},   {15709099088952724970U, 295},
    {11704190886730495818U, 322},   {17440603504673385349U, 348},
    {12994262207056124023U, 375},   {9681479787123295682U, 402},
    {14426529090290212157U, 428},   {10748601772107342003U, 455},
    {16016664761464807395U, 481},   {11933345169920330789U, 508},
    {17782069995880619868U, 534},   {13248674568444952270U, 561},
    {9871031767461413346U, 588},    {14708983551653345445U, 614},
    {10959046745042015199U, 641},   {16330252207878254650U, 667},
    {12166986024289022870U, 694},   {18130221999122236476U, 720},
    {13508068024458167312U, 747},   {10064294952495520794U, 774},
    {14996968138956309548U, 800},   {11173611982879273257U, 827},
    {16649979327439178909U, 853},   {12405201291620119593U, 880},
    {9242595204427927429U, 907},    {13772540099066387757U, 933},
    {10261342003245940623U, 960},   {15290591125556738113U, 986},
    {11392378155556871081U, 1013},  {16975966327722178521U, 1039},
    {12648080533535911531U, 1066}};

static Fp findCachedpow10(int exp, int *k) {
  const double one_log_ten = 0.30102999566398114;
  int approx = -(exp + npowers) * one_log_ten;
  int idx = (approx - firstpower) / steppowers;

  while (true) {
    int current = exp + powers_ten[idx].exp_ + 64;
    if (current < expmin) {
      idx++;
      continue;
    }
    if (current > expmax) {
      idx--;
      continue;
    }

    *k = (firstpower + idx * steppowers);
    return powers_ten[idx];
  }
}

#define fracmask 0x000FFFFFFFFFFFFFU
#define expmask 0x7FF0000000000000U
#define hiddenbit 0x0010000000000000U
#define signmask 0x8000000000000000U
#define expbias (1023 + 52)

#define absv(n) ((n) < 0 ? -(n) : (n))
#define minv(a, b) ((a) < (b) ? (a) : (b))

static uint64_t tens[] = {10000000000000000000U,
                          1000000000000000000U,
                          100000000000000000U,
                          10000000000000000U,
                          1000000000000000U,
                          100000000000000U,
                          10000000000000U,
                          1000000000000U,
                          100000000000U,
                          10000000000U,
                          1000000000U,
                          100000000U,
                          10000000U,
                          1000000U,
                          100000U,
                          10000U,
                          1000U,
                          100U,
                          10U,
                          1U};

static inline uint64_t getDbits(double d) {
  union {
    double dbl_;
    uint64_t i_;
  } dbl_bits = {d};

  return dbl_bits.i_;
}

static Fp buildFp(double d) {
  uint64_t bits = getDbits(d);

  Fp fp;
  fp.frac_ = bits & fracmask;
  fp.exp_ = (bits & expmask) >> 52;

  if (fp.exp_ != 0) {
    fp.frac_ += hiddenbit;
    fp.exp_ -= expbias;
  } else {
    fp.exp_ = -expbias + 1;
  }
  return fp;
}

static void normalize(Fp *fp) {
  while ((fp->frac_ & hiddenbit) == 0) {
    fp->frac_ <<= 1;
    fp->exp_--;
  }

  int shift = 64 - 52 - 1;
  fp->frac_ <<= shift;
  fp->exp_ -= shift;
}

static void getNormalizedBoundaries(Fp *fp, Fp *lower, Fp *upper) {
  upper->frac_ = (fp->frac_ << 1) + 1;
  upper->exp_ = fp->exp_ - 1;

  while ((upper->frac_ & (hiddenbit << 1)) == 0) {
    upper->frac_ <<= 1;
    upper->exp_--;
  }

  int u_shift = 64 - 52 - 2;
  upper->frac_ <<= u_shift;
  upper->exp_ = upper->exp_ - u_shift;

  int l_shift = fp->frac_ == hiddenbit ? 2 : 1;
  lower->frac_ = (fp->frac_ << l_shift) - 1;
  lower->exp_ = fp->exp_ - l_shift;
  lower->frac_ <<= lower->exp_ - upper->exp_;
  lower->exp_ = upper->exp_;
}

static Fp multiply(Fp *a, Fp *b) {
  const uint64_t lomask = 0x00000000FFFFFFFF;
  uint64_t ah_bl = (a->frac_ >> 32) * (b->frac_ & lomask);
  uint64_t al_bh = (a->frac_ & lomask) * (b->frac_ >> 32);
  uint64_t al_bl = (a->frac_ & lomask) * (b->frac_ & lomask);
  uint64_t ah_bh = (a->frac_ >> 32) * (b->frac_ >> 32);
  uint64_t tmp = (ah_bl & lomask) + (al_bh & lomask) + (al_bl >> 32);
  /* round up */
  tmp += 1U << 31;

  Fp fp = {ah_bh + (ah_bl >> 32) + (al_bh >> 32) + (tmp >> 32),
           a->exp_ + b->exp_ + 64};
  return fp;
}

static void roundDigit(char *digits, int ndigits, uint64_t delta, uint64_t rem,
                       uint64_t kappa, uint64_t frac) {
  while (rem < frac && delta - rem >= kappa &&
         (rem + kappa < frac || frac - rem > rem + kappa - frac)) {
    digits[ndigits - 1]--;
    rem += kappa;
  }
}

static int generateDigits(Fp *fp, Fp *upper, Fp *lower, char *digits, int *K) {
  uint64_t wfrac = upper->frac_ - fp->frac_;
  uint64_t delta = upper->frac_ - lower->frac_;

  Fp one;
  one.frac_ = 1ULL << -upper->exp_;
  one.exp_ = upper->exp_;

  uint64_t part1 = upper->frac_ >> -one.exp_;
  uint64_t part2 = upper->frac_ & (one.frac_ - 1);

  int idx = 0;
  int kappa = 10;
  uint64_t *divp;
  /* 1000000000 */
  for (divp = tens + 10; kappa > 0; divp++) {
    uint64_t div = *divp;
    unsigned digit = part1 / div;

    if ((digit != 0U) || (idx != 0)) {
      digits[idx++] = digit + '0';
    }

    part1 -= digit * div;
    kappa--;
    uint64_t tmp = (part1 << -one.exp_) + part2;
    if (tmp <= delta) {
      *K += kappa;
      roundDigit(digits, idx, delta, tmp, div << -one.exp_, wfrac);
      return idx;
    }
  }

  /* 10 */
  uint64_t *unit = tens + 18;

  while (true) {
    part2 *= 10;
    delta *= 10;
    kappa--;

    unsigned digit = part2 >> -one.exp_;
    if ((digit != 0U) || (idx != 0)) {
      digits[idx++] = digit + '0';
    }

    part2 &= one.frac_ - 1;
    if (part2 < delta) {
      *K += kappa;
      roundDigit(digits, idx, delta, part2, one.frac_, wfrac * *unit);
      return idx;
    }
    unit--;
  }
}

static int grisu2(double d, char *digits, int *K) {
  Fp w = buildFp(d);
  Fp lower;
  Fp upper;
  getNormalizedBoundaries(&w, &lower, &upper);
  normalize(&w);

  int k;
  Fp cp = findCachedpow10(upper.exp_, &k);
  w = multiply(&w, &cp);
  upper = multiply(&upper, &cp);
  lower = multiply(&lower, &cp);

  lower.frac_++;
  upper.frac_--;
  *K = -k;
  return generateDigits(&w, &upper, &lower, digits, K);
}

static int emitDigits(char *digits, int ndigits, char *dest, int K, bool neg) {
  int exp = absv(K + ndigits - 1);
  int max_trailing_zeros = 7;
  if (neg) {
    max_trailing_zeros -= 1;
  }

  /* write plain integer */
  if (K >= 0 && (exp < (ndigits + max_trailing_zeros))) {
    memcpy(dest, digits, ndigits);
    memset(dest + ndigits, '0', K);
    return ndigits + K;
  }

  /* write decimal w/o scientific notation */
  if (K < 0 && (K > -7 || exp < 4)) {
    int offset = ndigits - absv(K);
    /* fp < 1.0 -> write leading zero */
    if (offset <= 0) {
      offset = -offset;
      dest[0] = '0';
      dest[1] = '.';
      memset(dest + 2, '0', offset);
      memcpy(dest + offset + 2, digits, ndigits);
      return ndigits + 2 + offset;
      /* fp > 1.0 */
    }
    memcpy(dest, digits, offset);
    dest[offset] = '.';
    memcpy(dest + offset + 1, digits + offset, ndigits - offset);
    return ndigits + 1;
  }

  /* write decimal w/ scientific notation */
  ndigits = minv(ndigits, 18 - neg);
  int idx = 0;
  dest[idx++] = digits[0];

  if (ndigits > 1) {
    dest[idx++] = '.';
    memcpy(dest + idx, digits + 1, ndigits - 1);
    idx += ndigits - 1;
  }

  dest[idx++] = 'e';
  char sign = K + ndigits - 1 < 0 ? '-' : '+';
  dest[idx++] = sign;
  int cent = 0;

  if (exp > 99) {
    cent = exp / 100;
    dest[idx++] = cent + '0';
    exp -= cent * 100;
  }
  if (exp > 9) {
    int dec = exp / 10;
    dest[idx++] = dec + '0';
    exp -= dec * 10;
  } else if (cent != 0) {
    dest[idx++] = '0';
  }

  dest[idx++] = exp % 10 + '0';
  return idx;
}

static int filterSpecial(double fp, char *dest) {
  if (fp == 0.0) {
    dest[0] = '0';
    return 1;
  }

  uint64_t bits = getDbits(fp);
  bool nan = (bits & expmask) == expmask;
  if (!nan) {
    return 0;
  }

  if ((bits & fracmask) != 0U) {
    dest[0] = 'n';
    dest[1] = 'a';
    dest[2] = 'n';
  } else {
    dest[0] = 'i';
    dest[1] = 'n';
    dest[2] = 'f';
  }
  return 3;
}

int fpconvDtoa(double d, char dest[24]) {
  char digits[18];
  int str_len = 0;
  bool neg = false;

  if ((getDbits(d) & signmask) != 0U) {
    dest[0] = '-';
    str_len++;
    neg = true;
  }

  int spec = filterSpecial(d, dest + str_len);
  if (spec != 0) {
    return str_len + spec;
  }

  int k = 0;
  int ndigits = grisu2(d, digits, &k);
  str_len += emitDigits(digits, ndigits, dest + str_len, k, neg);
  return str_len;
}

} // namespace lynx::detail
