#include <gmp.h>

#define P  "ffffffff00000001000000000000000000000000ffffffffffffffffffffffff"
#define N  "ffffffff00000000ffffffffffffffffbce6faada7179e84f3b9cac2fc632551" 
#define O  "8f5dcb99de6037a918ec74285f31ae355972e5adb2339c083a79982587da270d"
#define S  "1b8bd0f85db4afc7e770052098b3f4b16823a870eaf5986a1cd5563e5485d6c3"
#define PI "9999999900000000999999999999999971576334fddaf8b62bd5e00e976eaffd"
#define TI "7fffffff800000007fffffffffffffffde737d56d38bcf4279dce5617e3192a9"
#define TO "77ffffff8800000077ffffffffffffffe08c45816653124e523f170b664e797e"
#define IX "a866ca9fa4bc0f26279974ca54db9255ce73393780aa1819d5ab36394320d2e5"
#define IY "a6c6d6f4373916cba7e6d13010975efdf7eed02d9e93e0716eb39c83cbc4a9fb"


struct Point {
    mpz_t x, y;
};
typedef struct Point Point;

void init_point(Point *p) {
   mpz_inits(p->x, p->y, NULL);
}

int is_at_infinity(const Point *p) {
    return (mpz_cmp_ui(p->x, 0) == 0) && (mpz_cmp_ui(p->y, 0) == 0);
}

void set_point_str(Point *p, const char *x_hex_repr, const char *y_hex_repr) {
    mpz_set_str(p->x, x_hex_repr, 16);
    mpz_set_str(p->y, y_hex_repr, 16);
}

void set_point(Point *p, const Point *q) {
    mpz_set(p->x, q->x);
    mpz_set(p->y, q->y);
}

void compensate_with_lambda(Point *rop, const Point *p, const Point *q, const mpz_t lambda, const mpz_t mod) {
 	Point r; 
	init_point(&r);
	mpz_powm_ui(r.x, lambda, 2, mod);
	mpz_sub(r.x, r.x, p->x);
	mpz_sub(r.x, r.x, q->x);
	mpz_mod(r.x, r.x, mod);
	mpz_sub(r.y, p->x, r.x);
	mpz_mul(r.y, r.y, lambda);
	mpz_sub(r.y, r.y, p->y);
	mpz_mod(r.y, r.y, mod);
	set_point(rop, &r);
}

void double_point(Point *r, const Point *p, const mpz_t mod) {
    if(mpz_cmp_ui(p->y, 0) == 0)
        set_point_str(r, "0", "0");
    else {
        mpz_t lambda, tmp;
	mpz_init(tmp);
	mpz_init_set(lambda, p->y);
	mpz_mul_2exp(lambda, lambda, 1);
	mpz_invert(lambda, lambda, mod);
	mpz_powm_ui(tmp, p->x, 2, mod);
	mpz_mul_ui(tmp, tmp, 3);
	mpz_sub_ui(tmp, tmp, 3);
    	mpz_mul(lambda, lambda, tmp);
	mpz_mod(lambda, lambda, mod);
	
	compensate_with_lambda(r, p, p, lambda, mod);

	mpz_clears(lambda, tmp, NULL);

    }
}

void add_points(Point *r, const Point *p, const Point *q, const mpz_t mod) {
    if(is_at_infinity(p))
        set_point(r, q);
    else if(is_at_infinity(q))
	set_point(r, p);
    else if((mpz_cmp(p->x, q->x) == 0) && (mpz_cmp(p->y, q->y) == 0))
	double_point(r, p, mod);
    else if(mpz_cmp(p->x, q->x) == 0)
	set_point_str(r, "0", "0"); 
    else {
        mpz_t lambda, tmp;
	mpz_init(tmp);
	mpz_init_set(lambda, q->x);
	mpz_sub(lambda, lambda, p->x);
	mpz_invert(lambda, lambda, mod);
	mpz_sub(tmp, q->y, p->y);
	mpz_mul(lambda, lambda, tmp);
	mpz_mod(lambda, lambda, mod);

	compensate_with_lambda(r, p, q, lambda, mod);

	mpz_clears(lambda, tmp, NULL);
    }
}

void mult_point(Point *r, const Point *p, const mpz_t scalar, const mpz_t mod){
    Point tmp;
    init_point(&tmp);
    set_point(&tmp, p);
    set_point_str(r, "0", "0");

    for(int i = 0; i < 256; i++) {
        if(mpz_tstbit(scalar, i) == 1)
            add_points(r, r, &tmp, mod);
	double_point(&tmp, &tmp, mod);
    }
}

void proprietary_magic(mpz_t x, mpz_t y, mpz_t z, mpz_t w, mpz_t n, mpz_t p, int c) {
    mpz_t r, d;
    mpz_inits(r, d, NULL);
    
    mpz_powm(d, y, p, n);
    mpz_xor(r, x, d);
    mpz_and(w, x, d);
    mpz_mul_2exp(w, w, 1);

    if(c == 0){
    	mpz_add(r, r, w);
	mpz_mod(r, r, n);
    }
    else {
        mpz_powm_ui(r, r, 5, n);
	mpz_add(p, p, n);
	mpz_sub_ui(p, p, 1);
        proprietary_magic(w, r, r, d, n, p, --c); 	
    }
    
    mpz_set(z, r);
}

void ECDSA_256_sign(unsigned char sig[256], const unsigned char hash[32])
{
    mpz_t n, p, q, z, s, ss, r, o, t, ti, to, d;
    mpz_inits(n, p, q, z, s, ss, r, o, t, ti, to, d, NULL);
    mpz_import(z, 32, 1, 1, 1, 0, hash);
    mpz_set_str(n, N, 16);
    mpz_set_str(p, P, 16);
    mpz_set_str(s, S, 16);
    mpz_set_str(q, PI, 16);
    mpz_set_str(ti, TI, 16);
    mpz_set_str(to, TO, 16);

    mpz_mod(z, z, n);
    if(mpz_cmp_ui(z, 0) != 0) {
    	mpz_invert(z, z, n);
    	mpz_mul(s, s, z);
    	mpz_mod(s, s, n);
    	mpz_set_str(o, O, 16);
    }
	
    Point ig, kg;
    init_point(&ig);
    init_point(&kg);    
    set_point_str(&ig, IX, IY);
    if(mpz_cmp_ui(z, 0) != 0) {
    	mpz_invert(z, z, n);
   	mult_point(&kg, &ig, z, p);

    } 
    else 
        set_point(&kg, &ig);
    
    mpz_mod(r, kg.x, n);
    mpz_set(ss, s);
    mpz_mul(ss, ss, r);
    mpz_mod(ss, ss, n);
    

    proprietary_magic(ss, o, ss, d, n, q, 256);


    while(mpz_cmp_ui(ss, 0) == 0) {
        double_point(&kg, &kg, p);
	mpz_mul(s, s, ti);
	mpz_mod(s, s, n);
	mpz_mul(o, o, to);
	mpz_mod(o, o, n);
	mpz_mod(r, kg.x, n);
	if(mpz_cmp_ui(r, 0) == 0)
            continue;

	mpz_set(ss, s);
        mpz_mul(ss, ss, r);
    	mpz_mod(ss, ss, n);
    	
    	proprietary_magic(ss, o, ss, d, n, q, 256);
   }

    mpz_export(sig, NULL, 1, 32, 1, 0, r);
    mpz_export(sig + 32, NULL, 1, 32, 1, 0, ss);
}
