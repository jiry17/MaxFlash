## MaxFlash

*MaxFlash* is a programming-by-example synthesizer that combining dynamic programming and structural probability. Our evaluation shows that *MaxFlash* achieves $\times 4.107 - \times 2080$ speed-ups against state-of-the-art solvers on $244$ real-world synthesis tasks.

[Our paper](**<https://github.com/jiry17/MaxFlash/blob/master/OOPSLA20_with_Appendix.pdf>**) "Guiding Dynamic Programing via Structural Probability for Accelerating Programming by Example" was accepted at OOPSLA'20.

### Build from source (Tested on Ubuntu 18.04)

1. Install dependencies

   ```bash
   $ sudo apt-get install libjsoncpp-dev libgoogle-glog-dev libgflags-dev cmake python3-pip
   ```

2. Clone *MaxFlash*

   ```bash
   $ git clone https://github.com/jiry17/IntSy.git
   ```

3. Build the whole project under the root directory of the project.

   ```bash
   $ ./install
   ```

### Run *MaxFlash*

We provide three scripts `train_model`, `run_benchmark`, `run_exp` under directory `run`.

#### Train a prediction model

`train_model` helps train a new prediction model from a set of labeled benchmarks. Currently, `train_model` only supports training $n$-gram models.

```bash
$ cd run
$ ./train_model -p PATH -t {string,matrix} [-d DEPTH] [-o OUTPUT]
```

1. `-p`: the path of the folder containing all training benchmarks.
2. `-t`: the domain of the training benchmarks. `string` represents the string manipulation domain, and `matrix` represents the matrix transformation domain.
3. `-d`: (optional) the depth of the $n$-gram model. The default value is `1`.
4. `-o`: (optional) the name of the resulting model. The default names for `string` and `matrix` are `model_string.json` and `model_matrix.json` respectively.

Some examples are listed below:

````bash
# Train a default model using all benchmarks in the string manipulation domain.
$ ./train_model -p ../benchmark/string -t string
# Train a model with depth 2 using all benchmarks in the matrix transformation domain, and save the model as model2.json
$ ./train_model -p ../benchmark/matrix -t matrix -d 2 -o model2.json
````

#### Run *MaxFlash* on a single benchmark

`run_benchmark` helps run *MaxFlash* on a given benchmark with a given prediction model.

```bash
$ cd run
$ ./run_benchmark -b BENCHMARK -t {string,matrix} [-tl TIMELIMIT <s>] [-ml MEMORYLIMIT <GB>] [-m MODEL] [-e EXE] [-l LOG] [-o OUTPUT]
```

1. `-b`: the path of the benchmark.
2. `-t`: the domain of the benchmark.
3. `-tl`: the time limit. The default time limit is $300$ seconds.
4. `-ml`: the memory limit. The default memory limit is $8$ GB.
5. `-m`: the path of the model. The default prediction models for `string` and `matrix` are `../src/train/model/model_string.json` and `../src/train/model/model_matrix.json`.
6. `-e`: the path of the executable file of *MaxFlash*. The default path is `../build/run`.
7. `-l`: the path of the log file. The default value of this flag is empty, which means to directly output the logs to STDERR.
8. `-o`: the path of the result file. The default value of this flag is empty, which means to directly output the results to STDOUT.

Some examples are listed below:

```bash
# Run MaxFlash on the test benchmark of the string domain.
$ ./run_benchmark -b ../src/main/test/test_string -t string
# Run MaxFlash on the test benchmark of the matrix domain with a customized time limit.
$ ./run_benchmark -b ../src/main/test/test_matrix -t matrix -tl 2
```

#### Run *MaxFlash* on the whole dataset.

`run_exp` helps to run *MaxFlash* on all benchmarks in fold `benchmark`. `run_exp` uses cross-validation to evaluate the performance of *MaxFlash* on all benchmarks. 

```bash
$ cd run
$ ./run_exp [-tl TIMELIMIT] [-ml MEMORYLIMIT] [-tn THREADNUM] [-fn FOLDNUM] [-d DEPTH] [-t {all,string,matrix}] [-e EXE]
```

1. `-tl`: the time limit. The default time limit is $300$ seconds.
2. `-ml`: the memory limit. The default memory limit is $8$ GB.
3. `-tn`: the number of threads. The default value is $5$.   
4. `-fn`: the number of folds. The default value is $0$, representing `run_exp` will uses leave-one-out cross-validation.
5. `-t`: the domain of the benchmark. The default value is `all`, representing `run_exp` will run *MaxFlash* on both domains.
6. `-e`: the path of the executable file of *MaxFlash*. The default path is `../build/run`.

The results of `run_exp` can be found in two places: 

1. Under directory `run`, the results will be summarized as `.csv` files, containing the time cost and the memory cost of *MaxFlash* on each benchmark. 
2. Under directory `run/result_cache`, the detailed results will be summarized as `.pkl` files, containing the time cost, the memory cost, and the resulting program of *MaxFlash* on each benchmark.

If you want to reproduce the experiment results of *MaxFlash* listed in our paper, you can simply execute the following command:

```bash
$ ./run_exp
```

**Note**:  `run_exp` only monitors the memory cost for not-so-easy benchmarks: If a benchmark is finished too quickly, `run_exp` may fail on collecting the size of the used memory.

**Note**: There may be some small differences between the results listed in our paper and the reproduced ones since all the codes are refactored for increasing readability and reusability.

### The structure of this repository

The files in this project are organized in the following way:

```
├── src                                   // Our implementation of MaxFlash
│   ├── basic                             // Basic classes
│   ├── main	                          // The entrance of MaxFlash
│   ├── matrix                            // Operators used in the matrix domain
│   ├── parser	                          // Classes for parsing the benchmarks
│   ├── solver	                          // Classes for synthesizing programs
│   ├── train	                          // Classes for training a prediction model
├── benchmark                             // Benchmarks used in our evaluation
│   ├── string                            // The string dataset, in SyGuS format
│   ├── matrix                            // The matrix dataset
├── run                                   // Evaluation results and scripts
│   ├── train_model                       // A script for training the prediction model
│   ├── run_benchmark                     // A script for running MaxFlash on a benchmark  
│   ├── run_exp                           // A script for running MaxFlash on a dataset  
│   ├── result_cache                      // The detailed results of MaxFlash on datasets
│   ├── string1-result.csv	              // The results of MaxFlash on the string dataset
│   ├── matrix1-result.csv	              // The results of MaxFlash on the matrix dataset
├── install                               // A simple script for building MaxFlash
├── OOPSLA20_with_Appendix.pdf            // The full version of our paper
```



