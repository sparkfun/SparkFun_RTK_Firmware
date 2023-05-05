# Opens ..\RTK_Surveyor\AP-Config\src\main.js, gzip's the contents and pastes into ..\RTK_Surveyor\form.h

# Written by: Paul Clark
# Last update: April 10th, 2023

# To convert AP-Config\src\main.js to main_js[], run the Python main_js_zipper.py script in the Tools folder:
#   cd Firmware\Tools
#   python main_js_zipper.py


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

defaultsource = '../RTK_Surveyor/AP-Config/src/main.js'
defaultdest   = '../RTK_Surveyor/form.h'
headersearch  = 'static const uint8_t main_js[] PROGMEM = {'
footersearch  = '}; ///main_js'

print()
print('SparkFun RTK: gzip main.js into form.h')
print()

sourcefilename = ''

if sourcefilename == '':
    # Check if the bin filename was passed in argv
    if len(sys.argv) > 1: sourcefilename = sys.argv[1]

# Ask user for filename offering firstfile as the default
if sourcefilename == '':
    sourcefilename = input('Enter the source filename (default: ' + defaultsource + '): ') # Get the filename
    print()
if sourcefilename == '': sourcefilename = defaultsource

destfilename = ''

if destfilename == '':
    # Check if the bin filename was passed in argv
    if len(sys.argv) > 2: destfilename = sys.argv[2]

# Ask user for filename offering firstfile as the default
if destfilename == '':
    destfilename = input('Enter the destination filename (default: ' + defaultdest + '): ') # Get the filename
    print()
if destfilename == '': destfilename = defaultdest

zippedfilename = sourcefilename + '.gzip'

print('Step 1: gzip',sourcefilename,'into',zippedfilename)
print()

with open(sourcefilename, 'rb') as f_in:
    with gzip.open(zippedfilename, 'wb') as f_out:
        shutil.copyfileobj(f_in, f_out)

headerfilename = destfilename + '.header'

print('Step 2: create',headerfilename,'from',destfilename)
print()

with open(destfilename, 'rb') as f_in:
    with open(headerfilename, 'wb') as f_out:
        content = f_in.read()

        try:
            pos = content.index(bytes(headersearch, 'utf-8'))
        except:
            raise Exception('Invalid destination file - could not find start of main_js!')

        pos += len(headersearch)

        f_out.write(content[:pos])

footerfilename = destfilename + '.footer'

print('Step 3: create',footerfilename,'from',destfilename)
print()

with open(destfilename, 'rb') as f_in:
    with open(footerfilename, 'wb') as f_out:
        content = f_in.read()

        try:
            pos = content.index(bytes(footersearch, 'utf-8'))
        except:
            raise Exception('Invalid destination file - could not find end of main_js!')

        f_out.write(content[pos:])

print('Step 4: create',destfilename,'from',headerfilename,'+',zippedfilename,'+',footerfilename)
print()

with open(destfilename, 'wb') as f_out:
    with open(headerfilename, 'rb') as f_in_1:
        with open(zippedfilename, 'rb') as f_in_2:
            with open(footerfilename, 'rb') as f_in_3:
                f_out.write(f_in_1.read())

                f_out.write(bytes('\r\n', 'utf-8'))

                content = f_in_2.read()
                count = 0

                for c in content[:-2]:
                    if count == 0:
                        f_out.write(bytes('  ', 'utf-8'))
                    f_out.write(bytes("0x{:02X},".format(c), 'utf-8'))
                    count += 1
                    if count == 16:
                        count = 0
                        f_out.write(bytes('\r\n', 'utf-8'))
                    else:
                        f_out.write(bytes(' ', 'utf-8'))

                if count == 0:
                    f_out.write(bytes('  ', 'utf-8'))
                f_out.write(bytes("0x{:02X}\r\n".format(content[-1]), 'utf-8'))

                f_out.write(f_in_3.read())

print('Step 5: delete',headerfilename,'+',zippedfilename,'+',footerfilename)
print()

os.remove(headerfilename)
os.remove(zippedfilename)
os.remove(footerfilename)

print('Done!')
