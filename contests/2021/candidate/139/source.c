#include <gmp.h>

// group order and prime modulus
const char * n_str = "ffffffff00000000ffffffffffffffffbce6faada7179e84f3b9cac2fc632551";
const char * p_str = "ffffffff00000001000000000000000000000000ffffffffffffffffffffffff";
const char * b_str = "0c2a5692fe1a7f9b8ee7eb4a7cd59cd62bce33476b3123cecbb6406837bf51f4";
const char * G_x_str = "6b17d1f2e12c4247f8bce6e563a440f277037d812deb33a0f4a13945d898c296";
const char * G_y_str = "4fe342e2fe1a7f9b8ee7eb4a7c0f9e162bce33576b315ececbb6406837bf51f5";


typedef struct point {
  mpz_t x, y;
} __Point;
typedef __Point Point[1];

void point_init(Point P) { mpz_inits(P->x, P->y, NULL); }
void point_clear(Point P) { mpz_clears(P->x, P->y, NULL); }

int time();
unsigned int sleep (unsigned int seconds);

void point_init_set_str(Point        P,
                        const char * x_str,
                        const char * y_str,
                        int          base)
{
  mpz_init_set_str(P->x, x_str, base);
  mpz_init_set_str(P->y, y_str, base);
}

void point_init_infinity(Point P)
{
  mpz_init_set_ui(P->x, 0);
  mpz_init_set_ui(P->y, 0);
}

int point_is_infinity(Point P)
{
  return (mpz_cmp_ui(P->x, 0) == 0) && (mpz_cmp_ui(P->y, 0) == 0);
}

int point_equal(Point P, Point Q)
{
  return (mpz_cmp(P->x, Q->x) == 0) && (mpz_cmp(P->y, Q->y) == 0);
}

int point_is_inverse(Point P, Point Q)
{
  int comp = mpz_cmp(P->x, Q->x) == 0;
  if (comp != 1) {
    return comp;
  }

  // compute negative
  mpz_t Q_y_neg;
  mpz_init(Q_y_neg);
  mpz_neg(Q_y_neg, Q->y);

  comp = mpz_cmp(P->y, Q_y_neg) == 0;
  mpz_clear(Q_y_neg);

  return comp;
}

void point_set(Point R, Point P)
{
  mpz_set(R->x, P->x);
  mpz_set(R->y, P->y);
}

void point_add(Point R, Point P, Point Q, mpz_t a, mpz_t p)
{

  if (point_is_infinity(P)) {
    point_set(R, Q);
    return;
  } else if (point_is_infinity(Q)) {
    point_set(R, P);
    return;
  }
  if (point_is_inverse(P, Q)) {
    point_init_infinity(R);
    return;
  }

  // lambda
  mpz_t lambda, denominator;
  mpz_inits(lambda, denominator, NULL);
  if (P == Q || point_equal(P, Q)) {
    mpz_powm_ui(lambda, P->x, 2, p);
    mpz_mul_ui(lambda, lambda, 3);
    mpz_add(lambda, lambda, a);

    mpz_mul_ui(denominator, P->y, 2);
    mpz_invert(denominator, denominator, p);
  } else {
    mpz_sub(lambda, Q->y, P->y);
    mpz_sub(denominator, Q->x, P->x);
    mpz_invert(denominator, denominator, p);
  }
  mpz_mul(lambda, lambda, denominator);
  mpz_mod(lambda, lambda, p);

  // R->x
  mpz_powm_ui(R->x, lambda, 2, p);

  mpz_sub(R->x, R->x, P->x);
  mpz_sub(R->x, R->x, Q->x);
  mpz_mod(R->x, R->x, p);

  // R->y
  mpz_sub(R->y, P->x, R->x);
  mpz_mul(R->y, lambda, R->y);
  mpz_mod(R->y, R->y, p);
  mpz_sub(R->y, R->y, P->y);
  mpz_mod(R->y, R->y, p);

  // clear mpz
  mpz_clears(lambda, denominator, NULL);
}

void point_scalar(
    Point R, Point P, mpz_t scalar, mp_bitcnt_t num_bits, mpz_t a, mpz_t p)
{
  Point tmp;
  point_init(tmp);

  for (mp_bitcnt_t i = num_bits - 1; i >= 0 && i < num_bits; i--) {
    point_add(tmp, R, R, a, p);

    if (mpz_tstbit(scalar, i) == 1) {
      point_add(R, tmp, P, a, p);
    } else {
      point_set(R, tmp);
    }
  }

  point_clear(tmp);
}

void ECDSA_256_sign(unsigned char sig[64], const unsigned char hash[32])
{
  Point Q;
  point_init_infinity(Q);

  mpz_t n;
  mpz_init_set_str(n, n_str, 16);

  mpz_t p, a, b;
  mpz_init_set_str(p, p_str, 16);
  mpz_init(a);
  mpz_sub_ui(a, p, 3);
  mpz_init_set_str(b, b_str, 16);

  sleep(1);

  Point G;
  point_init_set_str(G, G_x_str, G_y_str, 16);

  mpz_t k, r, k_inv;
  mpz_inits(k, r, k_inv, NULL);

  mpz_t z, s, d;
  mpz_inits(z, s, d, NULL);
  
  
  mpz_import(z, 32, 1, 1, 1, 0, hash);
  mpz_import(k, 32, -1, 1, -1, 0, hash);

  
  int loop_counter = 0;
  do {
    point_init_infinity(Q);
    mpz_mul_ui(k, k, time());
    mpz_add_ui(k, k, loop_counter);
    mpz_sub_ui(d, a, 2);
    mpz_mod(k, k, n);
    if (mpz_cmp_ui(k, 0) == 0) {
      loop_counter += 1;
      continue;
    }

    point_init_infinity(Q);
    point_scalar(Q, G, k, 256, a, p);

    mpz_mod(r, Q->x, n);
    if (mpz_cmp_ui(Q->x, 0) == 0) {
      loop_counter += 1;
      continue;
    }
    
    mpz_invert(k_inv, k, n);
    mpz_mul(s, r, d);
    mpz_mod(s, s, n);
    mpz_add(s, s, z);
    mpz_mod(s, s, n);

    mpz_mul(s, s, k_inv);
    mpz_mod(s, s, n);

    if (mpz_cmp_ui(s, 0) == 0) {
      loop_counter += 1;
      continue;
    }

    break;
  } while (1);

  mpz_export(sig, NULL, 1, 32, 1, 0, r);
  mpz_export(sig + 32, NULL, 1, 32, 1, 0, s);
  
  point_clear(G);
  point_clear(Q);
  mpz_clears(n, p, a, k, r, z, s, k_inv, d, NULL);
}
