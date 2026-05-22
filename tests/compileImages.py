# Copyright © 2014 CCP ehf.

import os
import glob

RESOURCE_PATH = 'resources'
OUTPUT_PATH = 'resourcesInclude'


def get_output_path(path):
    return os.path.join(OUTPUT_PATH, os.path.relpath(path, RESOURCE_PATH)) + '.h'


def convert_file(path):
    contents = open(path, 'rb').read()
    print '%s: %s' % (path, len(contents))
    hex = ",".join("0x{:02x} ".format(ord(c)) for c in contents)
    out_path = get_output_path(path)
    try:
        os.makedirs(os.path.dirname(out_path))
    except OSError:
        pass
    f = open(out_path, 'wt')
    f.write(hex)
    return out_path


def write_common_include(paths):
    f = open(os.path.join(OUTPUT_PATH, "_all.h"), 'wt')
    f.write('#pragma once\n#ifndef _all_H\n#define _all_H\n\n')
    for each in paths:
        name = os.path.splitext(os.path.relpath(each, OUTPUT_PATH))[0]
        var_name = name.replace('.', '_').replace('/', '_').replace('\\', '_')
        f.write('static uint8_t s_%s[] = {\n#include "%s"\n};\n\n' % (var_name, each.replace('\\', '/')))
    f.write('\n#endif\n')


def convert_files():
    out_paths = []
    for root, _, files in os.walk(RESOURCE_PATH):
        for each in files:
            out_paths.append(convert_file(os.path.join(root, each)))
    write_common_include(out_paths)


if __name__ == '__main__':
    convert_files()