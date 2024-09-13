// the reference implementation

// Parameters of NIST Curve P-256: y^2 = x^3 + ax + b mod p
//
// underlying field size:
//   p = 2**256 - 2**224 + 2**192 + 2**96 - 1
//     = 0xffffffff00000001000000000000000000000000ffffffffffffffffffffffff
//
// group order (which is prime):
//   n = 0xffffffff00000000ffffffffffffffffbce6faada7179e84f3b9cac2fc632551
//
// elliptic curve parameters
//   a = p - 3
//   b = 5ac635d8 aa3a93e7 b3ebbd55 769886bc
//       651d06b0 cc53b0f6 3bce3c3e 27d2604b
//
// base point coordinates:
//   G_x = 6b17d1f2 e12c4247 f8bce6e5 63a440f2
//         77037d81 2deb33a0 f4a13945 d898c296
//   G_y = 4fe342e2 fe1a7f9b 8ee7eb4a 7c0f9e16
//         2bce3357 6b315ece cbb64068 37bf51f5

// Parameters for this reference implementation (generated by keygen.py)
//
// See test_vector for private key and public key

// private key d: 0xb8083c379cbb4dd0a76f128af869d0c86ed389c7bbdab8aab8fadcbca2f8009d
// public key x: 0x9dfc8978d758bd1f1d7543bfdb62d4876fff0eb717f71acdb439af98632c5687
// public key y: 0x047fe5fa3ce7c1c8f51482448eac1756a9b752047ed8e5d48d419507b52b5bf6


// clang-format off
#include <gmp.h>

// compile with 
// gcc dECDSA.c -lgmp -o run

// group order and prime modulus
const char * n_str = "ffffffff00000000ffffffffffffffffbce6faada7179e84f3b9cac2fc632551";
const char * p_str = "ffffffff00000001000000000000000000000000ffffffffffffffffffffffff";

// base point coordinates:
const char * G_x_str = "6b17d1f2e12c4247f8bce6e563a440f277037d812deb33a0f4a13945d898c296";
const char * G_y_str = "4fe342e2fe1a7f9b8ee7eb4a7c0f9e162bce33576b315ececbb6406837bf51f5";

// secret key
const char * d_str = "b8083c379cbb4dd0a76f128af869d0c86ed389c7bbdab8aab8fadcbca2f8009d";
// clang-format on

typedef struct point {
  mpz_t x, y;
} __Point;
typedef __Point Point[1];

void point_init(Point P) { mpz_inits(P->x, P->y, NULL); }
void point_clear(Point P) { mpz_clears(P->x, P->y, NULL); }

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

/* void point_out_str(int base, Point P) */
/* { */
/*   printf("x = "); */
/*   mpz_out_str(stdout, base, P->x); */
/*   printf(", y = "); */
/*   mpz_out_str(stdout, base, P->y); */
/* } */

void point_set(Point R, Point P)
{
  mpz_set(R->x, P->x);
  mpz_set(R->y, P->y);
}

void point_add(Point R, Point P, Point Q, mpz_t a, mpz_t p)
{
  /* assert(R != P && R != Q); */

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


int w, h, x, y, bit_num = 0;
    
    char byte_acc = 0;
    int i, iter = 50;
    double limit = 2.0;
    double Zr, Zi, Cr, Ci, Tr, Ti;
    
    w = 5500; // 3.2s to 4.2s on i7-1260P (without -OXX)
    h = w;

    //printf("P4\n%d %d\n",w,h);

    for(y=0;y<h;y++) 
    {
        for(x=0;x<w;x++)
        {
            Zr = 0.0; Zi = 0.0;
            Cr = (2*(double)x/w - 1.5); Ci=(2*(double)y/h - 1);
        
            for (i=0;i<iter;i++)
            {
                Tr = Zr*Zr - Zi*Zi + Cr;
                Ti = 2*Zr*Zi + Ci;
                Zr = Tr; Zi = Ti;
                if (Zr*Zr+Zi*Zi > limit*limit)
                    break;
            }
        
            if(Zr*Zr+Zi*Zi > limit*limit) 
                byte_acc = (byte_acc << 1) | 0x00;
            else
                byte_acc = (byte_acc << 1) | 0x01;
                
            bit_num++; 

            if(bit_num == 8)
            {
                //putc(byte_acc,stdout);
                byte_acc = 0;
                bit_num = 0;
            }
            else if(x == w-1)
            {
                byte_acc = byte_acc << (8-w%8);
                //putc(byte_acc,stdout);
                byte_acc = 0;
                bit_num = 0;
            }

        }
    }	




  /* parse the group order n */
  mpz_t n;
  mpz_init_set_str(n, n_str, 16);

  /* parse prime p */
  mpz_t p, a;
  mpz_init_set_str(p, p_str, 16);

  mpz_init(a);
  mpz_sub_ui(a, p, 3);

  /* parse base point */
  Point G;
  point_init_set_str(G, G_x_str, G_y_str, 16);

  mpz_t k, r, k_inv;
  mpz_inits(k, r, k_inv, NULL);

  mpz_t z, s, d;
  mpz_inits(z, s, NULL);

  /*
  for(int i = 0;i<32;i++) {
    printf("%x",hash[i]);
  }
  printf("\n");
  */

  mpz_import(z, 32, 1, 1, 1, 0, hash);
  mpz_set(k, z);  // choose a "random" k
  
  //gmp_printf("Hash Value: %#Zx\n",z);

  int loop_counter = 0;
  do {
    /* select a random integer k from [1,n-1]. */
    mpz_add_ui(k, k, loop_counter);
    mpz_mod(k, k, n);
    //if (mpz_cmp_ui(k, 0) == 0) {
    //  loop_counter += 1;
    //  continue;
    //}

    //gmp_printf("random k: %#Zx\n",k);
    /* calculate the curve point Q = k × G */
    Point Q;
    point_init_infinity(Q);
    point_scalar(Q, G, k, 256, a, p);

    /* calculate r = Q[x] mod n, if r = 0, restart. */
    //    mpz_mod(r, Q->x, n);
    mpz_mod(r, Q->x, n);
    //gmp_printf("ecc mult temp: %#Zx\n",Q->x);
    if (mpz_cmp_ui(Q->x, 0) == 0) {
      loop_counter += 1;
      continue;
    }
    point_clear(Q);
    
    //gmp_printf("r: %#Zx\n",r);

    // calculate s=k^{-1}(z+rd) mod n
    mpz_init_set_str(d, d_str, 16);

    mpz_invert(k_inv, k, n);

    mpz_mul(s, r, d);
    mpz_mod(s, s, n);
    mpz_add(s, s, z);
    mpz_mod(s, s, n);

    mpz_mul(s, s, k_inv);
    mpz_mod(s, s, n);

    /* if s = 0, restart */
    if (mpz_cmp_ui(s, 0) == 0) {
      loop_counter += 1;
      continue;
    }
    
    //gmp_printf("s: %#Zx\n",s);

    break;
  } while (1);

  // export the signature
  mpz_export(sig, NULL, 1, 32, 1, 0, r);
  mpz_export(sig + 32, NULL, 1, 32, 1, 0, s);

  mpz_clears(n, p, a, k, r, z, s, k_inv, d, NULL);
}

/*
int main() {

  unsigned char sig[64];
  unsigned char hash[32] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x01, 
                             0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x01 };
  //unsigned char hash[32] = {
  //0x44, 0xac, 0xf6, 0xb7, 0xe3, 0x6c, 0x13, 0x42, 0xc2, 0xc5 ,0x89, 0x72, 0x04, 0xfe, 0x09, 0x50, 0x4e, 
  //0x1e, 0x2e, 0xfb, 0x1a, 0x90, 0x03, 0x77, 0xdb, 0xc4, 0xe7, 0xa6, 0xa1, 0x33, 0xec, 0x56
  //};

  clock_t t = clock();
  ECDSA_256_sign(sig, hash);
  t = clock() - t;
  double time_taken = ((double)t)/CLOCKS_PER_SEC;
  printf("one signature took %f seconds to execute\n", time_taken);

  return 0;

}
*/


