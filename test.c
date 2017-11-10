#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#include "config.h"
#include "primitives.h"
#include "rand.h"
#include "tvec.h"
#include "pool.h"
#include "thread.h"

#define LOCAL_POOL_SIZE            16

#define POP                        -1
#define PUSH                        1



#include "init_code.c"
#include "stack_op.c"

// void binprintf(int v)
// {
//     unsigned int mask = 1 << sizeof (int) * CHAR_BIT - 1;
//     while(mask) {
//         printf("%d", (v&mask ? 1 : 0));
//         mask >>= 1;
//     }
//     printf("%s\n","");
// }


int main(){
	pointer_t pt;
	pt.raw_data = 4222124650659859;
	printf("%s %d\n","Seq: ",pt.struct_data.seq );
	printf("%s %d\n","Index: ",pt.struct_data.index );
	return 0;
}