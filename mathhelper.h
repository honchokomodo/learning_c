#include <math.h>

int cmodulus(int num, int modulus)
{
	int calc = num % modulus;
	calc += modulus * (calc < 0);
	return calc;
}

int fdiv(int num, int den)
{
	return (int) floor(num / (double) den);
}
