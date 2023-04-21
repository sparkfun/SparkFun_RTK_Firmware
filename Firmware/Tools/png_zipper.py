# Opens the selected .png, gzip's the contents, converts to hex and pastes into a new file

# Written by: Paul Clark
# Last update: April 10th, 2023

# To convert rtk-setup.png into rtk-setup.png.gzip_hex, run the Python png_zipper.py script in the Tools folder:
#   cd Firmware\Tools
#   python png_zipper.py ..\RTK_Surveyor\AP-Config\src\rtk-setup.png


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

print()
print('SparkFun RTK: convert png to gzip_hex')
print()

sourcefilename = ''

if sourcefilename == '':
    # Check if the bin filename was passed in argv
    if len(sys.argv) > 1: sourcefilename = sys.argv[1]

# Ask user for filename offering firstfile as the default
if sourcefilename == '':
    sourcefilename = input('Enter the source filename: ') # Get the filename
    print()

zippedfilename = sourcefilename + '.gzip'
destfilename = sourcefilename + '.gzip_hex'

print('Step 1: convert',sourcefilename,'into',zippedfilename)
print()

with open(sourcefilename, 'rb') as f_in:
    with gzip.open(zippedfilename, 'wb') as f_out:
        shutil.copyfileobj(f_in, f_out)

print('Step 2: create',destfilename,'from',zippedfilename)
print()

with open(destfilename, 'wb') as f_out:
    with open(zippedfilename, 'rb') as f_in:
                content = f_in.read()
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

print('Step 3: delete',zippedfilename)
print()

os.remove(zippedfilename)

print('Done!')
