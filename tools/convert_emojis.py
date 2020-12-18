#!/bin/python

import sys
import argparse
import json

def main():
    parser = argparse.ArgumentParser(add_help=True);
    parser.add_argument('-i', '--input', metavar='JSON_FILE', type=str, help='Input JSON emoji file')
    parser.add_argument('-o', '--output', metavar='OUT_FILE', type=str, help='Output raw file')

    args = parser.parse_args(sys.argv[1:])

    with open(args.input, 'r', encoding='utf-8') as in_file:
        with open(args.output, 'w', encoding='utf-8') as out_file:
            for emoji in json.load(in_file):
                out_file.write(f"{emoji['name']}:{emoji['image']}\n")

if __name__ == "__main__":
    main()


