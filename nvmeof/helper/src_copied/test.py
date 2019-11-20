import os
import sys
import fileinput

path = "./"
file_list = os.listdir(path)

print(file_list)


for i in file_list:
    if i.endswith('.cpp') or i.endswith('.hpp') or i.endswith('.h'):
        for i, line in enumerate(fileinput.input(i,inplace=1)):
            sys.stdout.write(line.replace('../ssd', 'ns3'))

