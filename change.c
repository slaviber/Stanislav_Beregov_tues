#include <stdio.h>
#include <stdlib.h>

int main(){
#define N_COIN_TYPES 7
	char needs_plus = 0;
	char coin_types[N_COIN_TYPES] = { 100, 50, 20, 10, 5, 2, 1 };
	int change;
	printf("Input the change value: ");
	scanf("%d", &change);
	int i;
	for (i = 0; i < N_COIN_TYPES; ++i){
		double coin_amount = change/coin_types[i];
		if (coin_amount >= 1){
			int dec_amount = (int)coin_amount;
			change -= dec_amount*coin_types[i];
			if (needs_plus)printf(" ");
			printf("%dx%d%s", dec_amount, coin_types[i] >= 100 ? coin_types[i] / 100 : coin_types[i], coin_types[i] >= 100 ? "Lv" : "St");
			needs_plus = 1;
		}
	}
return 0;
}
