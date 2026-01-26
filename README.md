# Wave File Generator
**Herdís Heiða Jing Guðjohnsen**


<!-- table of content -->
<details>
  <summary>Table of Contents</summary>
  <ol>
    <li>
      <a href="#Version">Version</a>
    </li>
    <li>
        <a href="#How-to-run">How to run</a>
    </li>
    <li>
        <a href="#Anything-else">Anything else</a>
    </li>
  </ol>
</details>

<br>
<hr>
<!-- STUFF -->

### Version
`Version B`

### How to run
    
1. Compile program
``` bash
clang++   -std=c++98 -Wall -Wextra   wav_file_generation.cpp     -o program
```

2. Run program

|                | No arguments | With arguments                                                                                                                           |
| -------------- | ------------ | ---------------------------------------------------------------------------------------------------------------------------------------- |
| **Arguments:** |              | 1. generatedFilename: 32 chars (do not include .wav)<br>2. frequency: Hz sound frequency (INT)<br>3. duration: seconds (double)<br>      |
| **Examples:**  | `./program`  | `./program <generatedFilename> <frequency> <duration>` <br> `./program test_sound 440 0.5`                                               |


### Anything else

GUD LUCK