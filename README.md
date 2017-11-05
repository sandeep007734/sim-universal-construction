# sim-universal-construction
Automatically exported from code.google.com/p/sim-universal-construction

### Running the Code on a x86 machine:
There is a shell script given to compile the code of your choice.  
File that we will be focussing on is : **simstack.c**

Now running the command

`./bench.sh simstack.c`

gives error. What a surprise !! Digging some deep gives the comand use for compilation, which was failing. Command is:

`
gcc simstack.c -Wall -O3 -msse3 -ftree-vectorize -ftree-vectorizer-verbose=0 -finline-functions -lpthread -march=native -mtune=native -DN_THREADS=$2 -DUSE_CPUS=$3 -D_GNU_SOURCE -pipe -lm
`

`
gcc simstack.c -Wall -pthread
`

This is working

`gcc simstack.c  -pthread -D_GNU_SOURCE`

Meaning of each of the flags passed:
* Wall
  * This enables all the compilers warning messages.
* O3
  * This enables the level 3 of the optimizations to be done while compiling the code. More info [here][O3].
* msse3
  * This enables the Streaming SIMD Extensions, instruction set for  the x86 and x86-64 architecture. These are useful in multimedia, gaming and other floating point intensive computing tasks.
* ftree-vectorize
  * No need to do this independently. As per the new rules, this is enabled by default under O3.
* ftree-vectorizer-verbose
  * This is level of the verbosity. This has been deprecated in the latest versions for the flag -fopt-info-vec.
* finline-functions
  * This is enabled by default the optimization level O.
* lpthread
  * *pthread* flag asks the comiler to link the pthread library as well as configure  the compilation for the threads.
  * *lpthread* does the same thing, except the pre-defined macros do not get defined.
  * If possible use the *pthread* instead of *lpthread*.
* march
  * Indeicates the instruction set that can be used by the compiler.
* mtune
  * Tune the code according to the architecture type specified.
* DN_THREADS
  *  
* DUSE_CPUS
* D_GNU_SOURCE
* pipe
  * This specifies using pipes for communications between the stages of compiling instead of using temporary files.
* lm

[O3]: https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html

