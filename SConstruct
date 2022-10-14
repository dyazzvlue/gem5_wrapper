#!python

# Copyright (c) 2016, Dresden University of Technology (TU Dresden)
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its
#    contributors may be used to endorse or promote products derived from
#    this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER
# OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import os
import sys


gem5_arch = 'RISCV'
gem5_variant = 'opt'

gem5_root = '/home/pzy/Documents/gem5/gem5'
systemc_src = '/home/pzy/Downloads/systemc-2.3.3/src'
systemc_lib = '/home/pzy/Downloads/systemc-2.3.3/lib-linux64/'

env = Environment()

#Make the gem5 root available in SConscripts
env['GEM5_ROOT'] = gem5_root

shlibsuffix = env['SHLIBSUFFIX']

# add include dirs
env.Append(CPPPATH=[gem5_root + '/build/' + gem5_arch,
                    systemc_src,
                    '#src',
                    '#include'
                    ])

env.Append(CXXFLAGS=['-std=c++17',
                     '-DSC_INCLUDE_DYNAMIC_PROCESSES',
                     '-DTRACING_ON',
                     '-fPIC',
                     ])

if gem5_variant == 'debug':
    env.Append(CXXFLAGS=['-g', '-DDEBUG'])

deps = [] # keep track of all dependencies required for building the binaries

deps += SConscript('src/SConscript',
                    variant_dir='build_scon/', exports='env') 

# the SystemC SConscript makes certain assumptions, we need to fulfill these
# assumptions before calling the SConscript.
main = env
sys.path.append(gem5_root + '/src/python')
AddOption('--no-colors', dest='use_colors', action='store_false',
          help="Don't add color to abbreviated scons output")

deps.append(File(systemc_lib + "libsystemc-2.3.3" + shlibsuffix))
deps.append(File(os.path.join(gem5_root, 'build', gem5_arch,
             'libgem5_' + gem5_variant + shlibsuffix)))

'''
chi_test = SConscript('examples/chi/SConscript',
                      variant_dir='build_chi/examples/chi',
                      exports=['env', 'deps'])
chi_new = SConscript('examples/chi/SConscript',
                      variant_dir='build_new/examples/chi',
                      exports=['env', 'deps'])

Default(chi_test)
'''
