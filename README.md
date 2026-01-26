# WaveFileGenerator
** Herdís Heiða Jing Guðjohnsen **


## What version of the asignment was solved and should be graded
    Version B

## What needs to be written in the terminal to compile and run the program
    
1. Compile program
``` bash
clang++   -std=c++98 -Wall -Wextra   wav_file_generation.cpp     -o program
```



2. Run program
``` bash
    ./program
```


2. to run program with arguments
    1. waveFilename: filename up to 32 chars (do not include .war)
    2. frequency: Hz sound frequency (INT)
    3. duration: seconds (double)

- EXAMPLES:
``` bash
    ./program <waveFilename> <frequency> <duration>
```

``` bash
    ./program test_sound 440 0.5
```
|                | Run program | Run program with arguments                                                                                                                                           |
| -------------- | ----------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Arguments:** |             | 1. waveFilename: filename up to 32 chars (do not include .wav)<br>2. frequency: Hz sound frequency (INT)<br>3. duration: seconds (double)<br>                        |
| **Examples:**  | ./program   | ./program <waveFilename> <frequency> <duration><br>./program test_sound 440 0.5                                                                                      |


## Anything else a stundent wishes to say (Canvas comments will not be delivered to the graders)

GUD LUCK