import os
import sys
import fileinput

path = "./"
file_list = os.listdir(path)

print(file_list)

for ff in file_list:
    if ff.endswith('.h') or ff.endswith('.cpp') or ff.endswith('.hpp'):
        for i, line in enumerate(fileinput.input(ff,inplace=1)):
            sys.stdout.write(line.replace('Simulator', 'MQSimulator'))           
