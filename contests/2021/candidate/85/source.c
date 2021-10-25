#ifndef ECDSA_gmp_H
#define ECDSA_gmp_H

#include <gmp.h>

#define MYLOCAL 0
int o = 0;

mpz_t a,b,p,n,d;
mpz_t ONE, TWO, THREE;

mpz_t G_x,G_y, Q_x, Q_y;

gmp_randstate_t state;

typedef unsigned int * fucktype;

int point_cmp(mpz_t a_x, mpz_t a_y, mpz_t b_x, mpz_t b_y);
void set_secp256r1();
int ispointin(mpz_t P_x, mpz_t P_y);
void kpoint(mpz_t ans_x, mpz_t ans_y, mpz_t k, mpz_t P_x, mpz_t P_y);

unsigned int gmpToInt(unsigned int result[],mpz_t mess,unsigned int len);
void intToGmp(mpz_t result,unsigned int* arr,unsigned long bits);

/***********************************************************/
/* macro */


#define mpz_add_mod(c,a,b,p) \
    mpz_add(c,a,b); \
    mpz_mod(c,c,p); 


#define mpz_mul_mod(c,a,b,p) \
    mpz_mul(c,a,b); \
    mpz_mod(c,c,p); 


#endif


#define NO_RANDOM 1
#define X_S_VALUE 123456

void point_cpy(mpz_t dest_x, mpz_t dest_y, mpz_t src_x, mpz_t src_y) {
    mpz_set(dest_x, src_x);
    mpz_set(dest_y, src_y);
}

int point_eq(mpz_t a_x, mpz_t a_y, mpz_t b_x, mpz_t b_y) {
    return mpz_cmp(a_x,b_x)==0 && mpz_cmp(a_y, b_y)==0; 
} 

int isO(mpz_t a_x, mpz_t a_y) {
    return mpz_cmp_ui(a_x, 0)==0 && mpz_cmp_ui(a_y, 0)==0;
}

void set_secp256r1() {
    mpz_init(a);
    mpz_init(b);
    mpz_init(p);
    mpz_init(n);
    mpz_init(Q_x);
    mpz_init(Q_y);

    mpz_set_str(a,  "FFFFFFFF00000001000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFC",16);
    mpz_set_str(b,  "5AC635D8AA3A93E7B3EBBD55769886BC651D06B0CC53B0F63BCE3C3E27D2604B",16);
    mpz_set_str(p,  "FFFFFFFF00000001000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFF",16);
    mpz_set_str(n,  "FFFFFFFF00000000FFFFFFFFFFFFFFFFBCE6FAADA7179E84F3B9CAC2FC632551",16);
    mpz_set_str(G_x,"6B17D1F2E12C4247F8BCE6E563A440F277037D812DEB33A0F4A13945D898C296",16);
    mpz_set_str(G_y,"4FE342E2FE1A7F9B8EE7EB4A7C0F9E162BCE33576B315ECECBB6406837BF51F5",16);
}



void set_d(char s[65]) {
    mpz_set_str(d,s,16);
}

void lmbd(mpz_t ans, mpz_t P_x, mpz_t P_y, mpz_t Q_x, mpz_t Q_y) {
    if ( point_eq(P_x, P_y, Q_x, Q_y) ){
        mpz_set_ui(ans,3);
        mpz_mul_mod(ans, ans, P_x ,p);
        
        mpz_mul_mod(ans, ans, P_x ,p);
        mpz_add_mod(ans, ans, a, p);

        mpz_t t;
        mpz_init_set_ui(t,2);
        mpz_mul_mod(t, t,P_y,p);
        mpz_invert(t,t,p);
        mpz_mul_mod(ans, ans, t, p);
    }else {
        mpz_sub(ans, Q_y, P_y );
        mpz_mod(ans, ans, p);
        mpz_t idx;
        mpz_init(idx);
        mpz_sub(idx, Q_x, P_x);
        mpz_invert(idx, idx, p);
        mpz_mul_mod(ans,  ans, idx , p);
    }
}


void point_add(mpz_t ans_x, mpz_t ans_y , mpz_t P_x, mpz_t P_y, mpz_t Q_x, mpz_t Q_y) {
    if (isO(P_x, P_y)) {
        point_cpy(ans_x, ans_y, Q_x, Q_y);
        return;
    }

    if(isO(Q_x, Q_y)) {
        point_cpy(ans_x, ans_y, P_x, P_y);
        return;
    }
    
    mpz_t sum_y;
    mpz_init(sum_y);
    mpz_add_mod(sum_y, P_y, Q_y, p);
    if( mpz_cmp(P_x, Q_x) &&  mpz_cmp_ui(sum_y, 0)==0) {
        mpz_set_ui(ans_x, 0);
        mpz_set_ui(ans_y, 0);
        return;
    }

    mpz_t ld;
    mpz_init(ld);

    mpz_t R_x;
    mpz_t R_y;
    mpz_init(R_x);
    mpz_init(R_y);

    lmbd(ld, P_x, P_y, Q_x, Q_y);

    mpz_mul_mod( R_x, ld, ld, p);
    mpz_sub(R_x, R_x, P_x);
    mpz_sub(R_x, R_x, Q_x);
    mpz_mod(R_x, R_x, p);

    mpz_t temp;
    mpz_init(temp);
    mpz_sub(temp, P_x, R_x);
    mpz_mul(R_y, ld, temp);
    mpz_sub(R_y, R_y, P_y);
    mpz_mod(R_y, R_y, p);
    mpz_set(ans_x, R_x);    
    mpz_set(ans_y, R_y);    
}


int ispointin(mpz_t P_x, mpz_t P_y) {
    mpz_t left, right;
    mpz_init(left);
    mpz_init(right);
    
    mpz_mul(left, P_y, P_y);
    mpz_mod(left, left, p);


    mpz_t x3;
    mpz_init(x3);

    mpz_mul_mod(x3, P_x, P_x, p);
    mpz_mul_mod(x3, x3,   P_x, p);


    mpz_mul(right, a,   P_x);
    mpz_add(right, right, x3);
    mpz_add(right, right,  b);
    mpz_mod(right, right,  p);
    return mpz_cmp(left, right) == 0;
}



void kpoint(mpz_t ans_x, mpz_t ans_y, mpz_t k, mpz_t P_x, mpz_t P_y){
    mpz_t X,Y;
    mpz_init_set(X, P_x);
    mpz_init_set(Y, P_y);
    if(!mpz_cmp_ui(k,2)) {
        point_add(ans_x,ans_y, P_x, P_y, P_x, P_y);
        return;
    }

    mpz_set_ui(ans_x, 0);
    mpz_set_ui(ans_y, 0);

    unsigned long index=mpz_sizeinbase(k,2);
    for(int i=0;i<index;i++){
        if(mpz_tstbit(k,i))
            point_add(ans_x, ans_y, ans_x, ans_y, X, Y);
        point_add(X, Y, X, Y, X, Y);
    }
    mpz_clear(X);
    mpz_clear(Y);
}



const unsigned int base[32]={ 0x1,      0x2,      0x4,      0x8,      0x10,      0x20,      0x40,      0x80,
					  0x100,    0x200,    0x400,    0x800,    0x1000,    0x2000,    0x4000,    0x8000,
					  0x10000,  0x20000,  0x40000,  0x80000,  0x100000,  0x200000,  0x400000,  0x800000,
					  0x1000000,0x2000000,0x4000000,0x8000000,0x10000000,0x20000000,0x40000000,0x80000000};

// mpz_t 类型数 转换为 int数组
// len为mpz_t数的比特长度 
unsigned int gmpToInt(unsigned int result[],mpz_t mess,unsigned int len){
	unsigned long length=mpz_sizeinbase(mess,2);
	if(len>length)length=len;
	unsigned long temp=length;
	int bit=31;
	int index=0;
	result[0]=0;
	while(length--){
		if(bit==-1){
			index++;
			result[index]=0;
			bit=31;
		}
		if(mpz_tstbit(mess,length)) result[index]|=base[bit];
		bit--;
	}
	return temp;
}


// int数组 转为 mpz_t 数，bits为数组的比特长度
void intToGmp(mpz_t result,unsigned int* arr,unsigned long bits){
	unsigned long index=bits/32-1;
	if(bits%32)index++;
	unsigned int k=bits%32;
	unsigned long i;
	if(k!=0){
		i=k;
		k=arr[index]>>(32-k);
		mpz_set_ui(result,k);
	}
	else{
		i=0;
		index++;
		mpz_set_ui(result,0);
	}
	while(index--){
		for(int j=0;j<32;j++)
			if(arr[index]>>j&1)mpz_setbit(result,i+j);
		i+=32;
	}
}

int flag = 1;




int ecdsa_verification(unsigned char sig[64], const unsigned char hash[32]) 
{

    mpz_t r;
    mpz_t s;
    mpz_t e;
    mpz_t w;
    mpz_t u1;
    mpz_t u2;

    mpz_init(r);
    mpz_init(s);
    mpz_init(e);
    mpz_init(w);
    mpz_init(u1);
    mpz_init(u2);
   
    mpz_import(e, 32, 1, 1, 1, 0, hash);
    mpz_import(r, 32, 1, 1, 1, 0, sig);
    mpz_import(s, 32, 1, 1, 1, 0, sig+32);


    mpz_invert(w, s, n);
    mpz_mul(u1, e, w); mpz_mod(u1, u1, n);
    mpz_mul(u2, r, w); mpz_mod(u2, u2, n);

    mpz_t u1Gx, u1Gy, u2Qx, u2Qy;
    mpz_init(u1Gx);
    mpz_init(u1Gy);
    mpz_init(u2Qx);
    mpz_init(u2Qy);

    mpz_t Xx, Xy;
    mpz_init(Xx);
    mpz_init(Xy);

    kpoint(u1Gx, u1Gy, u1, G_x, G_y);
    kpoint(u2Qx, u2Qy, u2, Q_x, Q_y);
    point_add(Xx, Xy, u1Gx, u1Gy, u2Qx, u2Qy);
    if(isO(Xx, Xy)) return 0;
    
    mpz_t v;
    mpz_init(v);
    mpz_mod(v, Xx, n);
    return mpz_cmp(v, r) == 0;
}

void set_private_key() {
    mpz_init_set_str(d, "AB700D75274336FD26A1FE49D400ACEAE89F0FDBFE4BDE9A70373CA693003CA8" ,16);//set d
    kpoint(Q_x, Q_y, d, G_x, G_y);
}

void ECDSA_256_sign(unsigned char sig[64], const unsigned char hash[32]) 
{
    if(flag) {
        gmp_randinit_default(state);
        set_secp256r1();
        set_private_key();
        flag = 0;
    }
    mpz_t k;
    mpz_t k_inv;
    mpz_t r;
    mpz_t s;
    mpz_t e;
    mpz_init(k);
    mpz_init(k_inv);
    mpz_init(r);
    mpz_init(s);
    mpz_init(e);
    mpz_t kGx,kGy;
    mpz_init(kGx);
    mpz_init(kGy);    

    do {
        do {
            do {
                mpz_urandomm(k, state, n);
                if(o) mpz_set_str(k, "1", 10);

            }while(mpz_cmp_ui(k, 0)==0);
            // gmp_printf("%Zx\n", k);
            kpoint(kGx, kGy, k, G_x, G_y);
            mpz_mod(r, kGx, n);
        }while( mpz_cmp_ui(r, 0)==0 );

        
        mpz_import(e, 32, 1, 1, 1, 0, hash);

        mpz_invert(k_inv, k, n);
        mpz_mul(s, d, r);
        mpz_mod(s, s, n);
        mpz_add(s, s, e);
        mpz_mod(s, s, n);
        mpz_mul(s, s, k_inv);
        mpz_mod(s, s, n);

    }while(mpz_cmp_ui(s,0)==0);

    if(o) gmp_printf("r = %Zx\n", r);
    if(o) gmp_printf("s = %Zx\n", s);
    if(o) gmp_printf("Gx = %Zx\n", G_x);
    if(o) gmp_printf("k = %Zx\n", k);
    
  mpz_export(sig, NULL, 1, 32, 1, 0, r);
  mpz_export(sig + 32, NULL, 1, 32, 1, 0, s);
}



#if MYLOCAL==1
int main() {
    set_secp256r1();
    set_private_key();
    gmp_printf("%064Zx%064Zx\n", Q_x, Q_y);
    
    unsigned char hash[32], sig[64];
    mpz_t H;
    mpz_init_set_str(H, "f7fd41e28dfcca32c1ceef637c202ca6e99e57f18afef957df0866b4cdd60f5c",16);
    gmpToInt((fucktype)hash, H, 256);
    ECDSA_256_sign(sig, hash);

    gmp_printf("%d\n", ecdsa_verification(sig, hash));
    return 0;
}
#endif

