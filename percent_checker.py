#!/usr/bin/python3

import re
import sys
import requests


print("request to: {}\\\n".format(sys.argv[1]))
page = requests.get(sys.argv[1])
my_percentage = re.findall(r"[1-5]{1}\/my\/pa[1-5]{1}\/ \(.*\)", page.text)[0]
my_percentage = re.findall(r"\(.*\)", my_percentage)[0]
my_percentage = my_percentage.replace("(", '')
my_percentage = my_percentage.replace(")", '')

sample_percentage = re.findall(r"[1-5]{1}\/sample\/pa[1-5]{1}\/ \(.*\)", page.text)[0]
sample_percentage = re.findall(r"\(.*\)", sample_percentage)[0]
sample_percentage = sample_percentage.replace("(", '')
sample_percentage = sample_percentage.replace(")", '')

lines_matched = re.findall(r"ALIGN=right>.+", page.text)[0]
lines_matched = lines_matched.replace("ALIGN=right>", '')

print("\033[1;32mSAMPLE = {}, MY = {}, LINES MATCHED = {}\033[0m"
      .format(sample_percentage, my_percentage, lines_matched))
