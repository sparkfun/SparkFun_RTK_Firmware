# Opens ..\RTK_Surveyor\AP-Config\src\main.js, gzip's the contents and pastes into ..\RTK_Surveyor\Form.h

# Written by: Paul Clark
# Last update: April 10th, 2023

# SparkFun code, firmware, and software is released under the MIT License (http://opensource.org/licenses/MIT)
#
# The MIT License (MIT)
#
# Copyright (c) 2023 SparkFun Electronics
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

import sys
import os
import gzip
import shutil

print('SparkFun RTK: gzip main.js into Form.h')
print()

sourcefilename = ''

if sourcefilename == '':
    # Check if the bin filename was passed in argv
    if len(sys.argv) > 1: sourcefilename = sys.argv[1]

# Find main.js in ..\RTK_Surveyor\AP-Config\src
firstfile = ''
for root, dirs, files in os.walk("../RTK_Surveyor/AP-Config/src"):
    if len(files) > 0:
        if root == ".": # Comment this line to check sub-directories too
            for afile in files:
                if afile == 'main.js':
                    if firstfile == '': firstfile = os.path.join(root, afile)

# Ask user for filename offering firstfile as the default
if sourcefilename == '': sourcefilename = input('Enter the source filename (default: ' + firstfile + '): ') # Get the filename
if sourcefilename == '': sourcefilename = firstfile

destfilename = ''

if destfilename == '':
    # Check if the bin filename was passed in argv
    if len(sys.argv) > 2: destfilename = sys.argv[2]

# Find Form.h in ..\RTK_Surveyor
firstfile = ''
for root, dirs, files in os.walk("../RTK_Surveyor"):
    if len(files) > 0:
        if root == ".": # Comment this line to check sub-directories too
            for afile in files:
                if afile == 'Form.h':
                    if firstfile == '': firstfile = os.path.join(root, afile)

# Ask user for filename offering firstfile as the default
if destfilename == '': destfilename = input('Enter the destination filename (default: ' + destfile + '): ') # Get the filename
if destfilename == '': destfilename = firstfile

zippedfilename = sourcefilename + '.gzip'

print()
print('Step 1: gzip',sourcefilename,'into',zippedfilename)
print()

with open(sourcefilename, 'rb') as f_in:
    with gzip.open(zippedfilename, 'wb') as f_out:
        shutil.copyfileobj(f_in, f_out)

headerfilename = destfilename + '.header'

print()
print('Step 2: create',headerfilename,'from',destfilename)
print()

with open(destfilename, 'rb') as f_in:
    with open(headerfilename, 'wb') as f_out:
        content = f_in.read()

        try:
            pos = content.index(bytes('static const uint8_t main_js[] PROGMEM = {', 'utf-8'))
        except:
            raise Exception('Invalid destination file - could not find start of main_js!')

        pos += len('static const uint8_t main_js[] PROGMEM = {')

        f_out.write(content[:pos])

footerfilename = destfilename + '.footer'

print()
print('Step 3: create',footerfilename,'from',destfilename)
print()

with open(destfilename, 'rb') as f_in:
    with open(footerfilename, 'wb') as f_out:
        content = f_in.read()

        try:
            pos = content.index(bytes('}; ///main_js', 'utf-8'))
        except:
            raise Exception('Invalid destination file - could not find end of main_js!')

        f_out.write(content[pos:])

print()
print('Step 4: create',destfilename,'from',headerfilename,'+',zippedfilename,'+',footerfilename)
print()

with open(destfilename, 'wb') as f_out:
    with open(headerfilename, 'rb') as f_in_1:
        with open(zippedfilename, 'rb') as f_in_2:
            with open(footerfilename, 'rb') as f_in_3:
                f_out.write(f_in_1.read())

                content = f_in_2.read()
                count = 1
                for c in content:
                    f_out.print(hex(c))
                    f_out.print(',')
                    count += 1
                    if count == 16:
                        count = 1
                        f_out.print('\r\n')
                    else:
                        f_out.print(' ')

                f_out.write(f_in_3.read())

print()
print('Step 5: delete',headerfilename,'+',zippedfilename,'+',footerfilename)
print()

os.remove(headerfilename)
os.remove(zippedfilename)
os.remove(footerfilename)
