#!/usr/bin/python

import sys

def hex_dump_to_c_array(hex_dump: str) -> str:
    lines = hex_dump.strip().split('\n')
    values = []

    for line in lines:
        hex_values = line.split()[1:17]  # Extract the hex values (skip the offset)
        values.extend(hex_values)

    # Format as a C array
    c_array = ', '.join(['0x' + value for value in values])
    return f"const unsigned char data[] = {{ {c_array} }};"

if __name__ == '__main__':
    data = sys.stdin.read()
    result = hex_dump_to_c_array(data)
    print(result)