#!/usr/bin/python

#
# This source file is part of appleseed.
# Visit http://appleseedhq.net/ for additional information and resources.
#
# This software is released under the MIT license.
#
# Copyright (c) 2010-2013 Francois Beaune, Jupiter Jazz Limited
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

from __future__ import print_function
import argparse
import datetime
import os
import subprocess
import sys


#--------------------------------------------------------------------------------------------------
# Constants.
#--------------------------------------------------------------------------------------------------

DEFAULT_TOOL_FILENAME = "appleseed.cli.exe" if os.name == "nt" else "appleseed.cli"


#--------------------------------------------------------------------------------------------------
# Utility functions.
#--------------------------------------------------------------------------------------------------

def safe_mkdir(dir):
    if not os.path.exists(dir):
        os.mkdir(dir)

def walk(directory, recursive):
    if recursive:
        for dirpath, dirnames, filenames in os.walk(directory):
            yield dirpath, dirnames, filenames
    else:
        yield os.walk(directory).next()

def should_skip(path):
    return path.startswith("skip - ")


#--------------------------------------------------------------------------------------------------
# Render a given project file.
#--------------------------------------------------------------------------------------------------

def render_project_file(project_directory, project_filename, tool_path):
    project_filepath = os.path.join(project_directory, project_filename)

    output_directory = os.path.join(project_directory, 'renders')
    safe_mkdir(output_directory)

    output_filename = os.path.splitext(project_filename)[0] + '.png'
    output_filepath = os.path.join(output_directory, output_filename)

    log_filename = os.path.splitext(project_filename)[0] + '.txt'
    log_filepath = os.path.join(output_directory, log_filename)

    with open(log_filepath, "w") as log_file:
        print("rendering: {0}: ".format(project_filepath), end='')

        command = '"{0}" -o "{1}" "{2}"'.format(tool_path, output_filepath, project_filepath)

        start_time = datetime.datetime.now()
        result = subprocess.call(command, stderr=log_file, shell=True)
        end_time = datetime.datetime.now()

        if result == 0:
            print("{0} [ok]".format(end_time - start_time))
        else:
            print("[failed]")


#--------------------------------------------------------------------------------------------------
# Render all project files in a given directory (possibly recursively).
# Returns the number of rendered project files.
#--------------------------------------------------------------------------------------------------

def render_project_files(tool_path, directory, recursive):
    rendered_file_count = 0

    for dirpath, dirnames, filenames in walk(directory, recursive):
        if should_skip(os.path.basename(dirpath)):
            print("skipping:  {0}...".format(dirpath))
            continue

        for filename in filenames:
            if os.path.splitext(filename)[1] == '.appleseed':
                if should_skip(filename):
                    print("skipping:  {0}...".format(os.path.join(dirpath, filename)))
                    continue

                render_project_file(dirpath, filename, tool_path)
                rendered_file_count += 1

    return rendered_file_count


#--------------------------------------------------------------------------------------------------
# Entry point.
#--------------------------------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(description="render multiple project files.")
    parser.add_argument("-t", "--tool-path", metavar="tool-path",
                        help="set the path to the appleseed.cli tool")
    parser.add_argument("-r", "--recursive", action='store_true', dest="recursive",
                        help="scan the specified directory and all its subdirectories")
    parser.add_argument("directory", help="directory to scan")
    args = parser.parse_args()

    # If no tool path is provided, search for the tool in the same directory as this script.
    if args.tool_path is None:
        script_directory = os.path.dirname(os.path.realpath(__file__))
        args.tool_path = os.path.join(script_directory, DEFAULT_TOOL_FILENAME)
        print("setting tool path to {0}.".format(args.tool_path))

    start_time = datetime.datetime.now()
    rendered_file_count = render_project_files(args.tool_path, args.directory, args.recursive)
    end_time = datetime.datetime.now()

    print("rendered {0} project file(s) in {1}.".format(rendered_file_count, end_time - start_time))

if __name__ == '__main__':
    main()
