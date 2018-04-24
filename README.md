# random-bruteforcer
Tool for bruteforcing the output of random()

# Installing
Modify the definitions within random.c (e.g. number of threads, LOOKAHEAD value), then compile:

`clang -std=c11 -lpthread -lm -o random random.c`
or
`gcc -std=c11 -lpthread -lm -o random random.c`

GCC seems to give better performance

# Running
Use it by passing in the intial seed (normally 1) and the magic values you wish to run against, e.g.

```sh
$ php -r 'srand(1); echo rand() . "\r\n";'
1804289383
$ ./random 1 1804289383
Identified magic number 1804289383. Seed: 1. Iteration: 0 (1)
```

You can also run it with more than one value to search against
```sh
$ ./random 1 1804289383 396931439 292616681
Identified magic number 1804289383. Seed: 1. Iteration: 0 (1)
Identified magic number 292616681. Seed: 1337. Iteration: 0 (17)
Identified magic number 396931439. Seed: 450838. Iteration: 7 (18)
Identified magic number 1804289383. Seed: 2595599. Iteration: 47 (19)
Identified magic number 1804289383. Seed: 8998213. Iteration: 49 (13)
```

## Performance

Running on a 20 core machine (with 20 threads) and a lookahead of 1:
`real    3m19.240s`
The same machine with a lookahead of 50:
`real    4m22.828s`

## Improvements
Neither GCC or Clang vectorise the code appropriately. This could be done at a later time for a very significant speed boost
