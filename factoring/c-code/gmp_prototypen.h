void mpz_init(mpz t integer);
void mpz_set_ui(mpz t rop, unsigned long int op);
void mpz_mul(mpz t rop, mpz t op1, mpz t op2);
int mpz_cmp(mpz t op1, mpz t op2);
int mpz_cmp_ui(mpz t op1, mpz t op2);
void mpz_add_ui(mpz t rop, mpz t op1, unsigned long int op2);
int mpz_divisible_p(mpz t n, mpz t d);
void mpz_nextprime(mpz t rop, mpz t op);
int mpz_probab_prime_p(mpz t n, int reps);
void mpz_divexact (mpz t q, mpz t n, mpz t d);

