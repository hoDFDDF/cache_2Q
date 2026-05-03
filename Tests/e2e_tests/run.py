import subprocess
import os
import glob

cache_build = "../../build/2Q-cache/./2Q-cache"
test_dir = "Tests/"
output_file = "e2e_tests.out"

test_files = sorted(glob.glob(test_dir + "test*.in"))

with open(output_file, "w") as fout:
    for file in test_files:
        fout.write(f"{os.path.basename(file)}\n2Q_cache result:\n")
        with open(file, "r") as fin:
            input_data = fin.read()
            cache_2Q_ = subprocess.run([cache_build],
                                    input=input_data,   
                                 capture_output=True,
                                          text=True,)
            if (cache_2Q_.returncode != 0): 
                fout.write(f"EROR: {cache_2Q_.stderr}\n")
            else:
                fout.write(cache_2Q_.stdout)