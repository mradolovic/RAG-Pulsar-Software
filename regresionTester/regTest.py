import os
import subprocess
import filecmp

#First we check whether there is a old_bin directory and if there is we empty it

current_RTL_dir = os.listdir("../RTL/",)

if current_RTL_dir.count("old_bin") == 0:
    print("We need to make a new old_bin directory")
    subprocess.run(["mkdir -p ../RTL/old_bin/"], shell=True)
else:
    print("We already have a old_bin dir")
    #If we already have this bin we need to clean it
    subprocess.run(["rm *"], cwd="../RTL/old_bin", shell=True)

#We can now copy the old binaries into the old_bins folder

if len(os.listdir("../RTL/bin/")) == 0:
    print("Excuse me, where are the old binaries, against what am I supposed to regression test")
    exit(0)

for file in os.listdir("../RTL/bin/"):
    subprocess.run(["mv ../RTL/bin/" + str(file) + " ../RTL/old_bin/"], shell=True)

#Now that we have copied the old files we can compile the new code
subprocess.run(["make all"], cwd="../RTL/", shell=True)

#Okay we now run all the code
subprocess.run(["time ./pulsar_det_an_v4.out ../../data/data.bin 16 1 714.47415 128 1024 6.5 -26.7 -1.3 6 1 2.4 422 50 0 127"], cwd="../RTL/old_bin/", shell=True)

subprocess.run(["time ./pulsar_det_an_v4.out ../../data/data.bin 16 1 714.47415 128 1024 6.5 -26.7 -1.3 6 1 2.4 422 50 0 127"], cwd="../RTL/bin/", shell=True)

result = True
#Okay now that we have the files we can comapre them
for file1 in os.listdir("../RTL/bin/"):
    for file2 in os.listdir("../RTL/old_bin/"):
        #Grab all the txt files and comapre them
        if file1 == file2 and file1.endswith(".txt"):
            result = result and filecmp.cmp("../RTL/bin/"+file1,"../RTL/old_bin/"+file2, shallow=False)

if result == True:
    print("The outputs are equal")
else:
    print("THe outputs are not equal")


